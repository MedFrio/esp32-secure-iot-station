#include "SecurityManager.h"

void SecurityManager::begin(const String& token) {
  updateApiToken(token);
}

void SecurityManager::updateApiToken(const String& token) {
  apiToken = token.length() >= 12 ? token : "changeme-local-api";
}

bool SecurityManager::validateApiToken(const String& token) const {
  return token.length() > 0 && token == apiToken;
}

bool SecurityManager::validateInboundJson(const uint8_t* payload, size_t length, JsonDocument& doc, String& error) const {
  if (length == 0 || length > MAX_JSON_PAYLOAD) {
    error = "Payload JSON vide ou trop volumineux.";
    return false;
  }

  DeserializationError deserializeError = deserializeJson(doc, payload, length);
  if (deserializeError) {
    error = "JSON invalide.";
    return false;
  }

  if (!doc.as<JsonVariantConst>().is<JsonObjectConst>()) {
    error = "Le JSON entrant doit etre un objet.";
    return false;
  }

  return true;
}

bool SecurityManager::validateMqttCommand(JsonVariantConst json, String& error) const {
  if (!json.is<JsonObjectConst>()) {
    error = "Commande MQTT invalide.";
    return false;
  }

  const char* token = json["apiToken"] | "";
  if (!validateApiToken(String(token))) {
    error = "Token API invalide.";
    return false;
  }

  if (json["led"].is<JsonObjectConst>()) {
    JsonVariantConst led = json["led"];
    if (led["brightness"].is<int>() && (led["brightness"].as<int>() < 0 || led["brightness"].as<int>() > 255)) {
      error = "led.brightness invalide.";
      return false;
    }
    if (led["r"].is<int>() && (led["r"].as<int>() < 0 || led["r"].as<int>() > 255)) {
      error = "led.r invalide.";
      return false;
    }
    if (led["g"].is<int>() && (led["g"].as<int>() < 0 || led["g"].as<int>() > 255)) {
      error = "led.g invalide.";
      return false;
    }
    if (led["b"].is<int>() && (led["b"].as<int>() < 0 || led["b"].as<int>() > 255)) {
      error = "led.b invalide.";
      return false;
    }
  }

  if (json["servo"].is<JsonObjectConst>()) {
    if (!json["servo"]["angle"].is<int>()) {
      error = "servo.angle manquant.";
      return false;
    }
    const int angle = json["servo"]["angle"].as<int>();
    if (angle < 0 || angle > 180) {
      error = "servo.angle invalide.";
      return false;
    }
  }

  return true;
}

bool SecurityManager::validateConfig(JsonVariantConst json, String& error) const {
  if (!json.is<JsonObjectConst>()) {
    error = "Configuration invalide.";
    return false;
  }

  if (json["host"].is<const char*>() && !isSafeString(json["host"], 96)) {
    error = "host invalide.";
    return false;
  }

  if (json["username"].is<const char*>() && !isSafeString(json["username"], 64)) {
    error = "username invalide.";
    return false;
  }

  if (json["group"].is<const char*>() && !isSafeString(json["group"], 32)) {
    error = "group invalide.";
    return false;
  }

  if (json["deviceId"].is<const char*>() && !isSafeString(json["deviceId"], 31)) {
    error = "deviceId invalide.";
    return false;
  }

  if (json["port"].is<int>()) {
    int port = json["port"].as<int>();
    if (port <= 0 || port > 65535) {
      error = "port invalide.";
      return false;
    }
  }

  if (json["apiToken"].is<const char*>()) {
    const char* token = json["apiToken"];
    if (!isSafeString(token, 80) || strlen(token) < 12) {
      error = "apiToken trop court ou invalide.";
      return false;
    }
  }

  return true;
}

bool SecurityManager::isSafeString(const char* value, size_t maxLen) {
  if (value == nullptr) {
    return false;
  }

  const size_t len = strlen(value);
  if (len == 0 || len > maxLen) {
    return false;
  }

  for (size_t i = 0; i < len; i++) {
    const char c = value[i];
    const bool allowed =
      isalnum(c) || c == '.' || c == '-' || c == '_' || c == ':' || c == '/';
    if (!allowed) {
      return false;
    }
  }

  return true;
}
