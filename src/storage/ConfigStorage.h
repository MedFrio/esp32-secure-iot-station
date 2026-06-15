#pragma once

#include <Arduino.h>
#include <Preferences.h>
#include "../network/NetworkConfig.h"

class ConfigStorage {
public:
  MqttRuntimeConfig load();
  bool save(const MqttRuntimeConfig& config);

private:
  Preferences preferences;
};
