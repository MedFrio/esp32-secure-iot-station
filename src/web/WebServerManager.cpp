#include "WebServerManager.h"
#include <SPIFFS.h>
#include <AsyncJson.h>

void WebServerManager::begin(MqttRuntimeConfig* config,
                             SensorSample* latestSample,
                             SensorStatus* sensorStatus,
                             SemaphoreHandle_t* stateMutex,
                             MqttClientManager* mqttManager,
                             ActuatorManager* actuators,
                             OfflineStorage* storage,
                             SecurityManager* security,
                             std::function<void(const MqttRuntimeConfig&)> onConfigChanged) {
  runtimeConfig = config;
  latest = latestSample;
  sensors = sensorStatus;
  mutex = stateMutex;
  mqtt = mqttManager;
  actuatorManager = actuators;
  offline = storage;
  securityManager = security;
  configChanged = onConfigChanged;

  server.serveStatic("/", SPIFFS, "/").setDefaultFile("index.html");

  server.on("/api/state", HTTP_GET, [this](AsyncWebServerRequest* request) {
    if (!isAuthorized(request)) {
      sendError(request, 401, "Non autorise");
      return;
    }
    handleState(request);
  });

  server.on("/api/config", HTTP_GET, [this](AsyncWebServerRequest* request) {
    if (!isAuthorized(request)) {
      sendError(request, 401, "Non autorise");
      return;
    }
    handleConfig(request);
  });

  registerJsonHandlers();

  server.onNotFound([this](AsyncWebServerRequest* request) {
    sendError(request, 404, "Route inconnue");
  });

  server.begin();
  Serial.println("[WEB] Serveur web local demarre sur le port 80");
}

bool WebServerManager::isAuthorized(AsyncWebServerRequest* request) const {
  if (securityManager == nullptr) {
    return false;
  }

  if (request->hasHeader("X-API-Key")) {
    return securityManager->validateApiToken(request->getHeader("X-API-Key")->value());
  }

  if (request->hasParam("token")) {
    return securityManager->validateApiToken(request->getParam("token")->value());
  }

  return false;
}

void WebServerManager::sendError(AsyncWebServerRequest* request, int code, const String& message) const {
  JsonDocument doc;
  doc["error"] = message;

  String payload;
  serializeJson(doc, payload);
  request->send(code, "application/json", payload);
}

void WebServerManager::handleState(AsyncWebServerRequest* request) {
  JsonDocument doc;

  SensorSample sampleCopy{};
  SensorStatus sensorStatusCopy{};

  if (mutex != nullptr && xSemaphoreTake(*mutex, pdMS_TO_TICKS(50)) == pdTRUE) {
    sampleCopy = *latest;
    sensorStatusCopy = *sensors;
    xSemaphoreGive(*mutex);
  }

  JsonObject sensorObj = doc["sensors"].to<JsonObject>();
  sensorObj["device"] = sampleCopy.device;
  sensorObj["ts"] = sampleCopy.ts;
  sensorObj["temp"] = sampleCopy.temp;
  sensorObj["humidity"] = sampleCopy.humidity;
  sensorObj["joystickPercent"] = sampleCopy.joystickPercent;
  sensorObj["joystickXRaw"] = sampleCopy.joystickXRaw;
  sensorObj["joystickYRaw"] = sampleCopy.joystickYRaw;
  sensorObj["wireContact"] = sampleCopy.wireContactClosed;
  sensorObj["anomaly"] = sampleCopy.anomaly;
  sensorObj["dhtOk"] = sensorStatusCopy.dhtOk;
  sensorObj["rejectedCount"] = sensorStatusCopy.rejectedCount;

  JsonObject networkObj = doc["network"].to<JsonObject>();
  if (mqtt != nullptr) {
    NetworkStatus net = mqtt->status();
    networkObj["wifi"] = net.wifiConnected;
    networkObj["mqtt"] = net.mqttConnected;
    networkObj["rssi"] = net.wifiRssi;
    networkObj["published"] = net.publishedCount;
    networkObj["storedOffline"] = net.storedOfflineCount;
    networkObj["replayed"] = net.replayedCount;
    networkObj["latencyMs"] = net.lastPublishLatencyMs;
    networkObj["dataTopic"] = mqtt->dataTopic();
    networkObj["cmdTopic"] = mqtt->commandTopic();
  }

  JsonObject metrics = doc["metrics"].to<JsonObject>();
  metrics["heap"] = ESP.getFreeHeap();
  metrics["uptime"] = millis() / 1000;
  metrics["offlineBytes"] = offline != nullptr ? offline->pendingBytes() : 0;

  JsonObject act = doc["actuators"].to<JsonObject>();
  if (actuatorManager != nullptr) {
    actuatorManager->writeStateTo(act);
  }

  String payload;
  serializeJson(doc, payload);
  request->send(200, "application/json", payload);
}

void WebServerManager::handleConfig(AsyncWebServerRequest* request) {
  JsonDocument doc;

  if (runtimeConfig != nullptr) {
    doc["host"] = runtimeConfig->host;
    doc["port"] = runtimeConfig->port;
    doc["username"] = runtimeConfig->username;
    doc["group"] = runtimeConfig->group;
    doc["deviceId"] = runtimeConfig->deviceId;
  }

  String payload;
  serializeJson(doc, payload);
  request->send(200, "application/json", payload);
}

void WebServerManager::registerJsonHandlers() {
  AsyncCallbackJsonWebHandler* configHandler = new AsyncCallbackJsonWebHandler(
    "/api/config",
    [this](AsyncWebServerRequest* request, JsonVariant& json) {
      if (!isAuthorized(request)) {
        sendError(request, 401, "Non autorise");
        return;
      }

      String error;
      if (securityManager == nullptr || !securityManager->validateConfig(json.as<JsonVariantConst>(), error)) {
        sendError(request, 400, error);
        return;
      }

      if (runtimeConfig == nullptr) {
        sendError(request, 500, "Configuration indisponible");
        return;
      }

      if (json["host"].is<const char*>()) runtimeConfig->host = json["host"].as<const char*>();
      if (json["port"].is<int>()) runtimeConfig->port = json["port"].as<int>();
      if (json["username"].is<const char*>()) runtimeConfig->username = json["username"].as<const char*>();
      if (json["password"].is<const char*>()) runtimeConfig->password = json["password"].as<const char*>();
      if (json["group"].is<const char*>()) runtimeConfig->group = json["group"].as<const char*>();
      if (json["deviceId"].is<const char*>()) runtimeConfig->deviceId = json["deviceId"].as<const char*>();
      if (json["apiToken"].is<const char*>()) {
        runtimeConfig->apiToken = json["apiToken"].as<const char*>();
        securityManager->updateApiToken(runtimeConfig->apiToken);
      }

      if (configChanged) {
        configChanged(*runtimeConfig);
      }

      if (mqtt != nullptr) {
        mqtt->applyConfig(*runtimeConfig);
      }

      request->send(200, "application/json", "{\"ok\":true}");
    }
  );
  configHandler->setMethod(HTTP_POST);
  server.addHandler(configHandler);

  AsyncCallbackJsonWebHandler* actuatorHandler = new AsyncCallbackJsonWebHandler(
    "/api/actuators",
    [this](AsyncWebServerRequest* request, JsonVariant& json) {
      if (!isAuthorized(request)) {
        sendError(request, 401, "Non autorise");
        return;
      }

      String error;
      if (actuatorManager == nullptr || !actuatorManager->applyCommand(json.as<JsonVariantConst>(), error)) {
        sendError(request, 400, error);
        return;
      }

      request->send(200, "application/json", "{\"ok\":true}");
    }
  );
  actuatorHandler->setMethod(HTTP_POST);
  server.addHandler(actuatorHandler);
}
