#pragma once

#include <Arduino.h>

struct MqttRuntimeConfig {
  String host;
  uint16_t port = 1883;
  String username;
  String password;
  String group;
  String deviceId;
  String apiToken;
};

struct NetworkStatus {
  bool wifiConnected = false;
  bool mqttConnected = false;
  int32_t wifiRssi = 0;
  uint32_t lastMqttConnectMs = 0;
  uint32_t lastMqttDisconnectMs = 0;
  uint32_t reconnectAttempts = 0;
  uint32_t publishedCount = 0;
  uint32_t storedOfflineCount = 0;
  uint32_t replayedCount = 0;
  uint32_t lastPublishLatencyMs = 0;
};
