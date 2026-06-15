#include "MqttClientManager.h"
#include <WiFi.h>
#include <time.h>

#if __has_include("Secrets.h")
#include "Secrets.h"
#else
#include "Secrets.example.h"
#endif

void MqttClientManager::begin(const MqttRuntimeConfig& initialConfig,
                              OfflineStorage* storage,
                              ActuatorManager* actuators,
                              SecurityManager* security) {
  config = initialConfig;
  offlineStorage = storage;
  actuatorManager = actuators;
  securityManager = security;

  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);

  mqtt.onConnect([this](bool sessionPresent) {
    (void)sessionPresent;
    currentStatus.mqttConnected = true;
    currentStatus.lastMqttConnectMs = millis();

    Serial.printf("[MQTT] Connecte, topic data=%s\n", dataTopic().c_str());
    mqtt.subscribe(commandTopic().c_str(), 1);
    replayRequested = true;
  });

  mqtt.onDisconnect([this](AsyncMqttClientDisconnectReason reason) {
    (void)reason;
    currentStatus.mqttConnected = false;
    currentStatus.lastMqttDisconnectMs = millis();
    Serial.println("[MQTT] Deconnecte");
  });

  mqtt.onMessage([this](char* topic, char* payload, AsyncMqttClientMessageProperties properties,
                        size_t length, size_t index, size_t total) {
    (void)properties;
    this->onMqttMessage(topic, payload, properties, length, index, total);
  });

  mqtt.onPublish([this](uint16_t packetId) {
    if (packetId == lastPublishPacketId && lastPublishStartMs > 0) {
      currentStatus.lastPublishLatencyMs = millis() - lastPublishStartMs;
      lastPublishStartMs = 0;
    }
  });

  configureMqtt();
  connectWifi();
}

void MqttClientManager::loopOnce() {
  currentStatus.wifiConnected = WiFi.isConnected();
  currentStatus.wifiRssi = currentStatus.wifiConnected ? WiFi.RSSI() : 0;

  const uint32_t now = millis();

  if (!currentStatus.wifiConnected && now - lastWifiAttemptMs > WIFI_RETRY_MS) {
    connectWifi();
  }

  if (currentStatus.wifiConnected && !mqtt.connected() && now - lastMqttAttemptMs > MQTT_RETRY_MS) {
    connectMqtt();
  }

  if (mqtt.connected() && replayRequested && offlineStorage != nullptr) {
    replayRequested = false;
    uint32_t replayed = offlineStorage->replay([this](const String& payload) {
      return this->publishPayload(payload);
    });
    currentStatus.replayedCount += replayed;
  }
}

bool MqttClientManager::publishSample(const SensorSample& sample) {
  const String payload = buildTelemetryPayload(sample);

  if (!mqtt.connected()) {
    if (offlineStorage != nullptr) {
      offlineStorage->appendPayload(payload);
      currentStatus.storedOfflineCount++;
    }
    return false;
  }

  if (publishPayload(payload)) {
    currentStatus.publishedCount++;
    return true;
  }

  if (offlineStorage != nullptr) {
    offlineStorage->appendPayload(payload);
    currentStatus.storedOfflineCount++;
  }

  return false;
}

bool MqttClientManager::publishPayload(const String& payload) {
  if (!mqtt.connected()) {
    return false;
  }

  lastPublishStartMs = millis();
  lastPublishPacketId = mqtt.publish(dataTopic().c_str(), 1, false, payload.c_str(), payload.length());

  if (lastPublishPacketId == 0) {
    lastPublishStartMs = 0;
    return false;
  }

  return true;
}

void MqttClientManager::applyConfig(const MqttRuntimeConfig& newConfig) {
  config = newConfig;
  configureMqtt();

  if (mqtt.connected()) {
    mqtt.disconnect();
  }

  connectMqtt();
}

NetworkStatus MqttClientManager::status() const {
  NetworkStatus copy = currentStatus;
  copy.wifiConnected = WiFi.isConnected();
  copy.mqttConnected = mqtt.connected();
  copy.wifiRssi = copy.wifiConnected ? WiFi.RSSI() : 0;
  return copy;
}

String MqttClientManager::dataTopic() const {
  return "campus/" + config.group + "/" + config.deviceId + "/data";
}

String MqttClientManager::commandTopic() const {
  return "campus/" + config.group + "/" + config.deviceId + "/cmd";
}

void MqttClientManager::connectWifi() {
  lastWifiAttemptMs = millis();
  Serial.printf("[WIFI] Connexion a %s\n", WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
}

void MqttClientManager::connectMqtt() {
  lastMqttAttemptMs = millis();
  currentStatus.reconnectAttempts++;
  Serial.printf("[MQTT] Connexion a %s:%u\n", config.host.c_str(), config.port);
  mqtt.connect();
}

void MqttClientManager::configureMqtt() {
  mqtt.setServer(config.host.c_str(), config.port);
  mqtt.setClientId(config.deviceId.c_str());

  if (config.username.length() > 0) {
    mqtt.setCredentials(config.username.c_str(), config.password.c_str());
  }

  configTime(0, 0, "pool.ntp.org", "time.nist.gov");
}

void MqttClientManager::onMqttMessage(char* topic, char* payload,
                                      AsyncMqttClientMessageProperties properties,
                                      size_t length, size_t index, size_t total) {
  (void)topic;
  (void)properties;

  if (index != 0 || length != total) {
    Serial.println("[MQTT] Message fragmente ignore");
    return;
  }

  JsonDocument doc;
  String error;

  if (securityManager == nullptr || !securityManager->validateInboundJson(reinterpret_cast<uint8_t*>(payload), length, doc, error)) {
    Serial.printf("[SECURITY] Commande rejetee: %s\n", error.c_str());
    return;
  }

  if (!securityManager->validateMqttCommand(doc.as<JsonVariantConst>(), error)) {
    Serial.printf("[SECURITY] Commande MQTT rejetee: %s\n", error.c_str());
    return;
  }

  if (actuatorManager == nullptr || !actuatorManager->applyCommand(doc.as<JsonVariantConst>(), error)) {
    Serial.printf("[ACTUATORS] Commande rejetee: %s\n", error.c_str());
    return;
  }

  Serial.println("[MQTT] Commande actionneur appliquee");
}

String MqttClientManager::buildTelemetryPayload(const SensorSample& sample) const {
  JsonDocument doc;

  doc["device"] = config.deviceId;
  doc["ts"] = sample.ts;

  JsonObject data = doc["data"].to<JsonObject>();
  data["temp"] = serialized(String(sample.temp, 2));
  data["humidity"] = serialized(String(sample.humidity, 2));
  data["joystickPercent"] = sample.joystickPercent;
  data["joystickXRaw"] = sample.joystickXRaw;
  data["joystickYRaw"] = sample.joystickYRaw;
  data["wireContact"] = sample.wireContactClosed;

  JsonObject health = doc["health"].to<JsonObject>();
  health["heap"] = ESP.getFreeHeap();
  health["uptime"] = millis() / 1000;
  health["anomaly"] = sample.anomaly;
  health["seq"] = sample.seq;
  health["mqttLatencyMs"] = currentStatus.lastPublishLatencyMs;

  String payload;
  serializeJson(doc, payload);
  return payload;
}
