const $ = (id) => document.getElementById(id);

const token = () => localStorage.getItem('apiToken') || 'changeme-local-api';
const state = {
  ledAlert: false,
  ventOpen: false,
  normalReset: false,
  manualServo: false,
  lastManualAngle: 90,
  lastManualCommandMs: 0,
  ledEnabled: false,
  ledBrightness: 0,
  previousLedEnabled: false,
  previousLedBrightness: 0,
  steps: [false, false, false, false]
};

function headers() {
  return {
    'Content-Type': 'application/json',
    'X-API-Key': token()
  };
}

function joystickLabel(value) {
  if (value === 25) return 'Droite 25 %';
  if (value === 50) return 'Haut 50 %';
  if (value === 75) return 'Gauche 75 %';
  if (value === 100) return 'Bas 100 %';
  return 'Repos 0 %';
}

function servoAngleFromJoystick(value) {
  if (value === 25) return 140;
  if (value === 50) return 75;
  if (value === 75) return 35;
  if (value === 100) return 170;
  return 90;
}

function setMessage(text, type = 'ok') {
  $('missionMessage').textContent = text;
  $('missionMessage').className = type === 'error' ? 'message error' : type === 'ok' ? 'message ok' : 'message';
}

async function api(url, options = {}) {
  const response = await fetch(url, {
    ...options,
    headers: {
      ...headers(),
      ...(options.headers || {})
    }
  });

  if (!response.ok) {
    throw new Error(await response.text());
  }

  return response.json();
}

async function commandActuators({ ledEnabled, brightness, servoAngle }) {
  state.ledEnabled = ledEnabled;
  state.ledBrightness = brightness;

  await api('/api/actuators', {
    method: 'POST',
    body: JSON.stringify({
      led: {
        enabled: ledEnabled,
        brightness
      },
      servo: {
        angle: servoAngle
      }
    })
  });
}

async function commandServoOnly(servoAngle) {
  await commandActuators({
    ledEnabled: state.ledEnabled,
    brightness: state.ledBrightness,
    servoAngle
  });
}

async function commandManualServo(servoAngle) {
  await commandActuators({
    ledEnabled: true,
    brightness: 220,
    servoAngle
  });
}

function updateStep(index, done) {
  state.steps[index] = done;
  $(`step${index + 1}`).classList.toggle('done', done);
  const score = state.steps.filter(Boolean).length;
  $('missionScore').textContent = `Score ${score}/4`;
  $('missionScore').className = `badge ${score === 4 ? 'badge-ok' : 'badge-muted'}`;
}

function updateMissionCopy(score) {
  if (score === 0) {
    $('missionTitle').textContent = 'Ronde à effectuer';
    $('missionText').textContent = 'Le local est sous surveillance. Commence par confirmer la présence du technicien avec le bouton.';
  } else if (score === 1) {
    $('missionTitle').textContent = 'Présence confirmée';
    $('missionText').textContent = 'Le technicien est sur place. Sélectionne le niveau vigilance avec le joystick à droite.';
  } else if (score === 2) {
    $('missionTitle').textContent = 'Vigilance active';
    $('missionText').textContent = 'Déclenche le signal visuel puis ouvre le volet simulé avec le servo.';
  } else if (score === 3) {
    $('missionTitle').textContent = 'Incident maîtrisé';
    $('missionText').textContent = 'Remets le système en nominal : joystick au repos et actionneurs en état normal.';
  } else {
    $('missionTitle').textContent = 'Mission terminée';
    $('missionText').textContent = 'Le local technique est revenu en fonctionnement nominal. Les données restent publiées via MQTT.';
  }
}

async function refresh() {
  try {
    const data = await api('/api/state');
    const sensors = data.sensors;
    const actuators = data.actuators || {};
    const joystick = Number(sensors.joystickPercent);
    const manualAngle = servoAngleFromJoystick(joystick);

    if (actuators.led) {
      state.ledEnabled = Boolean(actuators.led.enabled);
      state.ledBrightness = Number(actuators.led.brightness) || 0;
    }

    $('mTemp').textContent = `${Number(sensors.temp).toFixed(1)} °C`;
    $('mHumidity').textContent = `${Number(sensors.humidity).toFixed(1)} %`;
    $('mJoystick').textContent = joystickLabel(joystick);
    $('manualDirection').textContent = joystickLabel(joystick);
    $('manualAngle').textContent = `${manualAngle} deg`;
    $('manualState').textContent = state.manualServo ? 'Actif' : 'Inactif';
    $('mButton').textContent = sensors.wireContact ? 'Appuyé' : 'Relâché';

    updateStep(0, Boolean(sensors.wireContact) || state.steps[0]);
    updateStep(1, joystick === 25 || state.steps[1]);
    updateStep(2, state.ledAlert && state.ventOpen);
    updateStep(3, joystick === 0 && state.normalReset && state.steps[2]);

    updateMissionCopy(state.steps.filter(Boolean).length);

    const now = Date.now();
    const shouldMoveServo = state.manualServo
      && (manualAngle !== state.lastManualAngle || now - state.lastManualCommandMs > 1200)
      && now - state.lastManualCommandMs > 300;

    if (shouldMoveServo) {
      state.lastManualAngle = manualAngle;
      state.lastManualCommandMs = now;
      await commandManualServo(manualAngle);
    }
  } catch (error) {
    setMessage(`Lecture impossible : ${error.message}`, 'error');
  }
}

$('alertLed').addEventListener('click', async () => {
  try {
    await commandActuators({ ledEnabled: true, brightness: 255, servoAngle: 90 });
    state.ledAlert = true;
    setMessage('Alerte LED activée.');
    refresh();
  } catch (error) {
    setMessage(`Commande refusée : ${error.message}`, 'error');
  }
});

$('openVent').addEventListener('click', async () => {
  try {
    await commandActuators({ ledEnabled: true, brightness: 255, servoAngle: 140 });
    state.ventOpen = true;
    setMessage('Volet simulé ouvert avec le servo.');
    refresh();
  } catch (error) {
    setMessage(`Commande refusée : ${error.message}`, 'error');
  }
});

$('resetActuators').addEventListener('click', async () => {
  try {
    await commandActuators({ ledEnabled: false, brightness: 0, servoAngle: 90 });
    state.normalReset = true;
    setMessage('Retour normal demandé.');
    refresh();
  } catch (error) {
    setMessage(`Commande refusée : ${error.message}`, 'error');
  }
});

$('enableManualServo').addEventListener('click', async () => {
  try {
    const data = await api('/api/state');
    const joystick = Number(data.sensors.joystickPercent);
    const manualAngle = servoAngleFromJoystick(joystick);

    state.previousLedEnabled = state.ledEnabled;
    state.previousLedBrightness = state.ledBrightness;
    state.manualServo = true;
    state.lastManualAngle = manualAngle;
    state.lastManualCommandMs = 0;
    await commandManualServo(manualAngle);
    setMessage('Pilotage manuel actif : le joystick controle le servo.');
    refresh();
  } catch (error) {
    setMessage(`Commande refusee : ${error.message}`, 'error');
  }
});

$('disableManualServo').addEventListener('click', async () => {
  try {
    state.manualServo = false;
    await commandActuators({
      ledEnabled: state.previousLedEnabled,
      brightness: state.previousLedBrightness,
      servoAngle: 90
    });
    state.lastManualAngle = 90;
    $('manualState').textContent = 'Inactif';
    $('manualAngle').textContent = '90 deg';
    setMessage('Pilotage manuel annule, servo recentre.');
  } catch (error) {
    setMessage(`Commande refusee : ${error.message}`, 'error');
  }
});

refresh();
setInterval(refresh, 1000);
