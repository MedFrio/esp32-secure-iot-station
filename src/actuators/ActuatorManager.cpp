#include "ActuatorManager.h"

void ActuatorManager::begin() {
  ledcSetup(LEDC_LED, 5000, 8);
  ledcAttachPin(PIN_LED, LEDC_LED);

  servo.setPeriodHertz(50);
  servo.attach(PIN_SERVO, 500, 2400);

  applyLed();
  applyServo();

  Serial.println("[ACTUATORS] LED blanche GPIO33 et servo initialises");
}

bool ActuatorManager::applyCommand(JsonVariantConst json, String& error) {
  if (!json.is<JsonObjectConst>()) {
    error = "La commande doit etre un objet JSON.";
    return false;
  }

  if (json["led"].is<JsonObjectConst>()) {
    if (!updateLed(json["led"], error)) {
      return false;
    }
  }

  if (json["servo"].is<JsonObjectConst>()) {
    if (!updateServo(json["servo"], error)) {
      return false;
    }
  }

  currentState.lastCommandMs = millis();
  applyLed();
  applyServo();
  return true;
}

ActuatorState ActuatorManager::state() const {
  return currentState;
}

void ActuatorManager::writeStateTo(JsonObject target) const {
  JsonObject led = target["led"].to<JsonObject>();
  led["enabled"] = currentState.ledEnabled;
  led["brightness"] = currentState.ledBrightness;
  target["servoAngle"] = currentState.servoAngle;
  target["lastCommandMs"] = currentState.lastCommandMs;
}

void ActuatorManager::applyLed() {
  ledcWrite(LEDC_LED, currentState.ledEnabled ? currentState.ledBrightness : 0);
}

void ActuatorManager::applyServo() {
  servo.write(constrain(currentState.servoAngle, 0, 180));
}

bool ActuatorManager::updateLed(JsonVariantConst led, String& error) {
  if (led["enabled"].is<bool>()) {
    currentState.ledEnabled = led["enabled"].as<bool>();
  }

  if (led["brightness"].is<int>()) {
    int value = led["brightness"].as<int>();
    if (value < 0 || value > 255) {
      error = "led.brightness doit etre compris entre 0 et 255.";
      return false;
    }
    currentState.ledBrightness = static_cast<uint8_t>(value);
  }

  const bool hasLegacyRgb = led["r"].is<int>() || led["g"].is<int>() || led["b"].is<int>();
  if (hasLegacyRgb) {
    int r = led["r"] | 0;
    int g = led["g"] | 0;
    int b = led["b"] | 0;
    if (r < 0 || r > 255 || g < 0 || g > 255 || b < 0 || b > 255) {
      error = "led r/g/b doit etre compris entre 0 et 255.";
      return false;
    }
    currentState.ledBrightness = static_cast<uint8_t>(max(r, max(g, b)));
  }

  return true;
}

bool ActuatorManager::updateServo(JsonVariantConst servoJson, String& error) {
  if (!servoJson["angle"].is<int>()) {
    error = "servo.angle est obligatoire.";
    return false;
  }

  int angle = servoJson["angle"].as<int>();
  if (angle < 0 || angle > 180) {
    error = "servo.angle doit etre compris entre 0 et 180.";
    return false;
  }

  currentState.servoAngle = angle;
  return true;
}
