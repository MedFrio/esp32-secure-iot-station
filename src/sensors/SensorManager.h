#pragma once

#include <Arduino.h>
#include <DHT.h>
#include "SensorData.h"

class SensorManager {
public:
  bool begin();
  bool read(SensorSample& sample);
  SensorStatus status() const;

private:
  static constexpr uint8_t PIN_DHT = 4;
  static constexpr uint8_t PIN_JOYSTICK_X = 34;
  static constexpr uint8_t PIN_JOYSTICK_Y = 35;
  static constexpr uint8_t PIN_WIRE_CONTACT = 27;
  static constexpr uint8_t DHT_TYPE = DHT22;
  static constexpr float ALPHA = 0.25f;
  static constexpr uint16_t JOYSTICK_LOW_THRESHOLD = 1400;
  static constexpr uint16_t JOYSTICK_HIGH_THRESHOLD = 2700;

  DHT dht{PIN_DHT, DHT_TYPE};
  SensorStatus currentStatus;

  float filteredTemp = NAN;
  float filteredHumidity = NAN;

  static bool isValidTemperature(float value);
  static bool isValidHumidity(float value);
  static uint32_t epochOrUptime();
  static uint8_t joystickPercentFromAxes(uint16_t xRaw, uint16_t yRaw);
  float applyEma(float previous, float value) const;
};
