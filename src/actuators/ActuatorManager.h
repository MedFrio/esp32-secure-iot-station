#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include <ESP32Servo.h>

struct ActuatorState {
  bool ledEnabled = false;
  uint8_t ledBrightness = 180;
  int servoAngle = 90;
  uint32_t lastCommandMs = 0;
};

class ActuatorManager {
public:
  void begin();
  bool applyCommand(JsonVariantConst json, String& error);
  ActuatorState state() const;
  void writeStateTo(JsonObject target) const;

private:
  static constexpr uint8_t PIN_LED = 33;
  static constexpr uint8_t PIN_SERVO = 13;
  static constexpr uint8_t LEDC_LED = 8;

  ActuatorState currentState;
  Servo servo;

  void applyLed();
  void applyServo();
  bool updateLed(JsonVariantConst led, String& error);
  bool updateServo(JsonVariantConst servoJson, String& error);
};
