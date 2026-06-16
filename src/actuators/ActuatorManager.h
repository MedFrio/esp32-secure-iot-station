#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include <ESP32Servo.h>

struct ActuatorState {
  bool ledEnabled = false;
  uint8_t ledBrightness = 180;
  int servoAngle = 90;
  bool servoSweepEnabled = false;
  uint8_t servoSweepIntensity = 50;
  uint8_t servoSweepMode = 0;
  bool emergencyStopActive = false;
  uint32_t lastCommandMs = 0;
};

class ActuatorManager {
public:
  void begin();
  void update();
  void setEmergencyStop(bool active);
  bool applyCommand(JsonVariantConst json, String& error);
  ActuatorState state() const;
  void writeStateTo(JsonObject target) const;

private:
  static constexpr uint8_t PIN_LED = 33;
  static constexpr uint8_t PIN_SERVO = 13;
  static constexpr uint8_t LEDC_LED = 8;

  ActuatorState currentState;
  Servo servo;
  int8_t servoSweepDirection = 1;
  uint32_t lastServoSweepMs = 0;
  uint8_t servoPatternIndex = 0;

  void applyLed();
  void applyServo();
  void updateServoSweep();
  void updateServoRadar(uint8_t intensity);
  void updateServoAlert(uint8_t intensity);
  bool updateLed(JsonVariantConst led, String& error);
  bool updateServo(JsonVariantConst servoJson, String& error);
};
