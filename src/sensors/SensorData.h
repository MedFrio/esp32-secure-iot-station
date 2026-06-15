#pragma once

#include <Arduino.h>

struct SensorSample {
  char device[32];
  uint32_t ts;
  float temp;
  float humidity;
  uint8_t joystickPercent;
  uint16_t joystickXRaw;
  uint16_t joystickYRaw;
  bool wireContactClosed;
  bool anomaly;
  uint32_t seq;
};

struct SensorStatus {
  bool dhtOk = false;
  bool initialized = false;
  uint32_t lastReadMs = 0;
  uint32_t rejectedCount = 0;
  uint32_t sequence = 0;
};
