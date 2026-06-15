#pragma once

#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <functional>
#include "../sensors/SensorData.h"
#include "../actuators/ActuatorManager.h"
#include "../network/MqttClientManager.h"
#include "../storage/OfflineStorage.h"
#include "../security/SecurityManager.h"

class WebServerManager {
public:
  void begin(MqttRuntimeConfig* config,
             SensorSample* latestSample,
             SensorStatus* sensorStatus,
             SemaphoreHandle_t* stateMutex,
             MqttClientManager* mqttManager,
             ActuatorManager* actuators,
             OfflineStorage* storage,
             SecurityManager* security,
             std::function<void(const MqttRuntimeConfig&)> onConfigChanged);

private:
  AsyncWebServer server{80};

  MqttRuntimeConfig* runtimeConfig = nullptr;
  SensorSample* latest = nullptr;
  SensorStatus* sensors = nullptr;
  SemaphoreHandle_t* mutex = nullptr;
  MqttClientManager* mqtt = nullptr;
  ActuatorManager* actuatorManager = nullptr;
  OfflineStorage* offline = nullptr;
  SecurityManager* securityManager = nullptr;
  std::function<void(const MqttRuntimeConfig&)> configChanged;

  bool isAuthorized(AsyncWebServerRequest* request) const;
  void sendError(AsyncWebServerRequest* request, int code, const String& message) const;
  void handleState(AsyncWebServerRequest* request);
  void handleConfig(AsyncWebServerRequest* request);
  void registerJsonHandlers();
};
