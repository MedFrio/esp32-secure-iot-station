#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>

class SecurityManager {
public:
  void begin(const String& token);
  void updateApiToken(const String& token);
  bool validateApiToken(const String& token) const;
  bool validateInboundJson(const uint8_t* payload, size_t length, JsonDocument& doc, String& error) const;
  bool validateMqttCommand(JsonVariantConst json, String& error) const;
  bool validateConfig(JsonVariantConst json, String& error) const;

private:
  String apiToken;

  static constexpr size_t MAX_JSON_PAYLOAD = 512;
  static bool isSafeString(const char* value, size_t maxLen);
};
