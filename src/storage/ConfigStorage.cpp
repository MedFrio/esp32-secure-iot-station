#include "ConfigStorage.h"

#if __has_include("../network/Secrets.h")
#include "../network/Secrets.h"
#else
#include "../network/Secrets.example.h"
#endif

MqttRuntimeConfig ConfigStorage::load() {
  MqttRuntimeConfig config;

  preferences.begin("iot_cfg", true);
  config.host = preferences.getString("mqttHost", DEFAULT_MQTT_HOST);
  config.port = preferences.getUShort("mqttPort", DEFAULT_MQTT_PORT);
  config.username = preferences.getString("mqttUser", DEFAULT_MQTT_USER);
  config.password = preferences.getString("mqttPass", DEFAULT_MQTT_PASSWORD);
  config.group = preferences.getString("group", DEFAULT_GROUP);
  config.deviceId = preferences.getString("deviceId", DEFAULT_DEVICE_ID);
  config.apiToken = preferences.getString("apiToken", DEFAULT_API_TOKEN);
  preferences.end();

  return config;
}

bool ConfigStorage::save(const MqttRuntimeConfig& config) {
  preferences.begin("iot_cfg", false);
  preferences.putString("mqttHost", config.host);
  preferences.putUShort("mqttPort", config.port);
  preferences.putString("mqttUser", config.username);
  preferences.putString("mqttPass", config.password);
  preferences.putString("group", config.group);
  preferences.putString("deviceId", config.deviceId);
  preferences.putString("apiToken", config.apiToken);
  preferences.end();

  return true;
}
