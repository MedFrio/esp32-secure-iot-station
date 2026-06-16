const $ = (id) => document.getElementById(id);

const tokenInput = $('apiToken');
const message = $('message');
const actuatorStatus = $('actuatorStatus');
let actuatorTimer = null;
let hasSyncedActuators = false;

tokenInput.value = localStorage.getItem('apiToken') || tokenInput.value;

function apiHeaders() {
  return {
    'Content-Type': 'application/json',
    'X-API-Key': tokenInput.value
  };
}

function setBadge(element, text, state) {
  element.textContent = text;
  element.className = `badge badge-${state}`;
}

function showMessage(text, type = 'muted') {
  message.textContent = text;
  message.className = type === 'error' ? 'message error' : type === 'ok' ? 'message ok' : 'message';
}

function formatBytes(value) {
  if (value >= 1024) {
    return `${(value / 1024).toFixed(1)} Ko`;
  }
  return `${value} o`;
}

function formatUptime(seconds) {
  const minutes = Math.floor(seconds / 60);
  const hours = Math.floor(minutes / 60);
  if (hours > 0) {
    return `${hours} h ${minutes % 60} min`;
  }
  if (minutes > 0) {
    return `${minutes} min ${seconds % 60} s`;
  }
  return `${seconds} s`;
}

function joystickLabel(value) {
  if (value === 25) return 'Droite 25 %';
  if (value === 50) return 'Haut 50 %';
  if (value === 75) return 'Gauche 75 %';
  if (value === 100) return 'Bas 100 %';
  return 'Repos 0 %';
}

function updateJoystickPad(value) {
  const cells = {
    0: $('joyCenter'),
    25: $('joyRight'),
    50: $('joyUp'),
    75: $('joyLeft'),
    100: $('joyDown')
  };

  Object.values(cells).forEach((cell) => cell.classList.remove('active'));
  (cells[value] || cells[0]).classList.add('active');
}

async function fetchJson(url, options = {}) {
  const response = await fetch(url, {
    ...options,
    headers: {
      ...apiHeaders(),
      ...(options.headers || {})
    }
  });

  if (!response.ok) {
    const text = await response.text();
    throw new Error(text || response.statusText);
  }

  return response.json();
}

function syncActuatorControls(actuators) {
  if (hasSyncedActuators || !actuators) {
    return;
  }

  if (actuators.led) {
    $('ledEnabled').checked = Boolean(actuators.led.enabled);
    $('ledBrightness').value = actuators.led.brightness ?? 180;
    $('ledBrightnessValue').textContent = $('ledBrightness').value;
  }

  if (typeof actuators.servoAngle === 'number') {
    $('servo').value = actuators.servoAngle;
    $('servoValue').textContent = `${actuators.servoAngle}°`;
  }

  if (typeof actuators.servoSweepIntensity === 'number') {
    $('servoIntensity').value = actuators.servoSweepIntensity;
    $('servoIntensityValue').textContent = `${actuators.servoSweepIntensity}%`;
  }

  if (typeof actuators.servoSweepMode === 'number') {
    $('servoMode').value = String(actuators.servoSweepMode);
  }

  hasSyncedActuators = true;
}

async function refreshState() {
  try {
    const state = await fetchJson('/api/state');
    const sensors = state.sensors;
    const network = state.network;
    const metrics = state.metrics;

    $('temp').textContent = `${Number(sensors.temp).toFixed(1)} °C`;
    $('humidity').textContent = `${Number(sensors.humidity).toFixed(1)} %`;
    $('joystick').textContent = joystickLabel(Number(sensors.joystickPercent));
    updateJoystickPad(Number(sensors.joystickPercent));
    $('joystickRaw').textContent = `X=${sensors.joystickXRaw}, Y=${sensors.joystickYRaw}`;
    $('wireContact').textContent = sensors.wireContact ? 'Appuyé' : 'Relâché';
    $('sampleAge').textContent = sensors.ts ? new Date(sensors.ts * 1000).toLocaleTimeString() : '--';
    $('dhtStatus').textContent = sensors.dhtOk ? 'OK' : 'Non détecté';

    setBadge($('wifi'), network.wifi ? 'WiFi connecté' : 'WiFi hors ligne', network.wifi ? 'ok' : 'error');
    setBadge($('mqtt'), network.mqtt ? 'MQTT connecté' : 'MQTT hors ligne', network.mqtt ? 'ok' : 'warn');
    setBadge($('anomaly'), sensors.anomaly ? 'Anomalie' : 'Capteurs OK', sensors.anomaly ? 'error' : 'ok');

    $('rssi').textContent = `${network.rssi} dBm`;
    $('dataTopic').textContent = network.dataTopic || '--';
    $('cmdTopic').textContent = network.cmdTopic || '--';
    $('heap').textContent = formatBytes(metrics.heap);
    $('uptime').textContent = formatUptime(metrics.uptime);
    $('latency').textContent = `${network.latencyMs} ms`;
    $('offline').textContent = formatBytes(metrics.offlineBytes);

    syncActuatorControls(state.actuators);
  } catch (error) {
    showMessage(`Etat indisponible : ${error.message}`, 'error');
  }
}

async function loadConfig() {
  try {
    const config = await fetchJson('/api/config');
    $('mqttHost').value = config.host || '';
    $('mqttPort').value = config.port || 1883;
    $('mqttUser').value = config.username || '';
    $('group').value = config.group || '';
    $('deviceId').value = config.deviceId || '';
  } catch (error) {
    showMessage(`Configuration indisponible : ${error.message}`, 'error');
  }
}

async function sendActuators() {
  try {
    setBadge(actuatorStatus, 'Envoi...', 'warn');

    await fetchJson('/api/actuators', {
      method: 'POST',
      body: JSON.stringify({
        led: {
          enabled: $('ledEnabled').checked,
          brightness: Number($('ledBrightness').value)
        },
        servo: {
          angle: Number($('servo').value),
          sweep: false,
          intensity: Number($('servoIntensity').value),
          mode: Number($('servoMode').value)
        }
      })
    });

    setBadge(actuatorStatus, 'Appliqué', 'ok');
    showMessage('Commande actionneur envoyée.', 'ok');
    refreshState();
  } catch (error) {
    setBadge(actuatorStatus, 'Erreur', 'error');
    showMessage(`Commande refusée : ${error.message}`, 'error');
  }
}

async function sendServoSweep(enabled) {
  try {
    setBadge(actuatorStatus, enabled ? 'Rotation...' : 'Arrêt...', 'warn');
    clearTimeout(actuatorTimer);

    const servoCommand = {
      sweep: enabled,
      intensity: Number($('servoIntensity').value),
      mode: Number($('servoMode').value),
      angle: enabled ? 90 : Number($('servo').value)
    };

    await fetchJson('/api/actuators', {
      method: 'POST',
      body: JSON.stringify({
        servo: servoCommand
      })
    });

    setBadge(actuatorStatus, enabled ? 'Rotation active' : 'Rotation arrêtée', 'ok');
    showMessage(enabled ? 'Rotation continue du servo démarrée.' : 'Rotation continue annulée.', 'ok');
    refreshState();
  } catch (error) {
    setBadge(actuatorStatus, 'Erreur', 'error');
    showMessage(`Commande refusée : ${error.message}`, 'error');
  }
}

function startServoPreset(mode, intensity) {
  $('servoMode').value = String(mode);
  $('servoIntensity').value = String(intensity);
  $('servoIntensityValue').textContent = `${intensity}%`;
  sendServoSweep(true);
}

function scheduleActuatorSend() {
  clearTimeout(actuatorTimer);
  actuatorTimer = setTimeout(sendActuators, 350);
}

$('saveToken').addEventListener('click', () => {
  localStorage.setItem('apiToken', tokenInput.value);
  showMessage('Token mémorisé localement.', 'ok');
  loadConfig();
  refreshState();
});

$('ledBrightness').addEventListener('input', () => {
  $('ledBrightnessValue').textContent = $('ledBrightness').value;
  scheduleActuatorSend();
});

$('ledEnabled').addEventListener('change', scheduleActuatorSend);

$('servo').addEventListener('input', () => {
  $('servoValue').textContent = `${$('servo').value}°`;
  scheduleActuatorSend();
});

$('servoIntensity').addEventListener('input', () => {
  $('servoIntensityValue').textContent = `${$('servoIntensity').value}%`;
});

$('servoMode').addEventListener('change', () => {
  if (actuatorStatus.textContent.includes('Rotation')) {
    sendServoSweep(true);
  }
});

$('startServoSweep').addEventListener('click', () => sendServoSweep(true));
$('stopServoSweep').addEventListener('click', () => sendServoSweep(false));
$('servoPresetRadar').addEventListener('click', () => startServoPreset(1, 75));
$('servoPresetAlert').addEventListener('click', () => startServoPreset(2, 100));

$('sendActuators').addEventListener('click', sendActuators);

$('saveConfig').addEventListener('click', async () => {
  try {
    const body = {
      host: $('mqttHost').value,
      port: Number($('mqttPort').value),
      username: $('mqttUser').value,
      group: $('group').value,
      deviceId: $('deviceId').value
    };

    if ($('mqttPass').value.length > 0) {
      body.password = $('mqttPass').value;
    }

    await fetchJson('/api/config', {
      method: 'POST',
      body: JSON.stringify(body)
    });

    showMessage('Configuration MQTT sauvegardée.', 'ok');
    loadConfig();
  } catch (error) {
    showMessage(`Sauvegarde refusée : ${error.message}`, 'error');
  }
});

showMessage('Interface prête.');
loadConfig();
refreshState();
setInterval(refreshState, 1000);
