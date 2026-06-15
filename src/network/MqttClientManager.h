#pragma once

#include <Arduino.h>
#include <AsyncMqttClient.h>
#include <ArduinoJson.h>
#include "../sensors/SensorData.h"
#include "../storage/OfflineStorage.h"
#include "../actuators/ActuatorManager.h"
#include "../security/SecurityManager.h"
#include "NetworkConfig.h"

class MqttClientManager {
public:
  void begin(const MqttRuntimeConfig& initialConfig,
             OfflineStorage* storage,
             ActuatorManager* actuators,
             SecurityManager* security);

  void loopOnce();
  bool publishSample(const SensorSample& sample);
  bool publishPayload(const String& payload);
  void applyConfig(const MqttRuntimeConfig& newConfig);
  NetworkStatus status() const;
  String dataTopic() const;
  String commandTopic() const;

private:
  AsyncMqttClient mqtt;
  MqttRuntimeConfig config;
  NetworkStatus currentStatus;

  OfflineStorage* offlineStorage = nullptr;
  ActuatorManager* actuatorManager = nullptr;
  SecurityManager* securityManager = nullptr;

  uint32_t lastWifiAttemptMs = 0;
  uint32_t lastMqttAttemptMs = 0;
  uint16_t lastPublishPacketId = 0;
  uint32_t lastPublishStartMs = 0;
  bool replayRequested = false;

  static constexpr uint32_t WIFI_RETRY_MS = 5000;
  static constexpr uint32_t MQTT_RETRY_MS = 3000;

  void connectWifi();
  void connectMqtt();
  void configureMqtt();
  void onMqttMessage(char* topic, char* payload, AsyncMqttClientMessageProperties properties,
                     size_t length, size_t index, size_t total);
  String buildTelemetryPayload(const SensorSample& sample) const;
};
