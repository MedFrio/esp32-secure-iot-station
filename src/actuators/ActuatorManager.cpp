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

void ActuatorManager::update() {
  if (!currentState.servoSweepEnabled) {
    return;
  }

  const uint8_t intensity = constrain(currentState.servoSweepIntensity, 1, 100);

  if (currentState.servoSweepMode == 1) {
    updateServoRadar(intensity);
    return;
  }

  if (currentState.servoSweepMode == 2) {
    updateServoAlert(intensity);
    return;
  }

  updateServoSweep();
}

void ActuatorManager::updateServoSweep() {
  const uint32_t now = millis();
  const uint8_t intensity = constrain(currentState.servoSweepIntensity, 1, 100);
  const uint32_t intervalMs = map(intensity, 1, 100, 45, 5);
  const int step = map(intensity, 1, 100, 2, 12);

  if (now - lastServoSweepMs < intervalMs) {
    return;
  }

  lastServoSweepMs = now;
  currentState.servoAngle += servoSweepDirection * step;

  if (currentState.servoAngle >= 180) {
    currentState.servoAngle = 180;
    servoSweepDirection = -1;
  } else if (currentState.servoAngle <= 0) {
    currentState.servoAngle = 0;
    servoSweepDirection = 1;
  }

  applyServo();
}

void ActuatorManager::updateServoRadar(uint8_t intensity) {
  const uint32_t now = millis();
  const uint32_t intervalMs = map(intensity, 1, 100, 120, 18);

  if (now - lastServoSweepMs < intervalMs) {
    return;
  }

  static constexpr uint8_t radarAngles[] = {25, 55, 90, 125, 155, 125, 90, 55};
  lastServoSweepMs = now;
  currentState.servoAngle = radarAngles[servoPatternIndex];
  servoPatternIndex = (servoPatternIndex + 1) % (sizeof(radarAngles) / sizeof(radarAngles[0]));
  applyServo();
}

void ActuatorManager::updateServoAlert(uint8_t intensity) {
  const uint32_t now = millis();
  const uint32_t intervalMs = map(intensity, 1, 100, 140, 25);

  if (now - lastServoSweepMs < intervalMs) {
    return;
  }

  static constexpr uint8_t alertAngles[] = {70, 110, 75, 105, 90, 35, 145, 90};
  lastServoSweepMs = now;
  currentState.servoAngle = alertAngles[servoPatternIndex];
  servoPatternIndex = (servoPatternIndex + 1) % (sizeof(alertAngles) / sizeof(alertAngles[0]));
  applyServo();
}

bool ActuatorManager::applyCommand(JsonVariantConst json, String& error) {
  if (currentState.emergencyStopActive) {
    error = "Arret d'urgence actif.";
    return false;
  }

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

void ActuatorManager::setEmergencyStop(bool active) {
  if (currentState.emergencyStopActive == active) {
    return;
  }

  currentState.emergencyStopActive = active;
  currentState.servoSweepEnabled = false;
  currentState.servoAngle = 90;
  servoPatternIndex = 0;
  lastServoSweepMs = 0;

  if (active) {
    currentState.ledEnabled = true;
    currentState.ledBrightness = 255;
  } else {
    currentState.ledEnabled = false;
  }

  currentState.lastCommandMs = millis();
  applyLed();
  applyServo();
}

ActuatorState ActuatorManager::state() const {
  return currentState;
}

void ActuatorManager::writeStateTo(JsonObject target) const {
  JsonObject led = target["led"].to<JsonObject>();
  led["enabled"] = currentState.ledEnabled;
  led["brightness"] = currentState.ledBrightness;
  target["servoAngle"] = currentState.servoAngle;
  target["servoSweepEnabled"] = currentState.servoSweepEnabled;
  target["servoSweepIntensity"] = currentState.servoSweepIntensity;
  target["servoSweepMode"] = currentState.servoSweepMode;
  target["emergencyStop"] = currentState.emergencyStopActive;
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
  if (servoJson["sweep"].is<bool>()) {
    currentState.servoSweepEnabled = servoJson["sweep"].as<bool>();
    lastServoSweepMs = 0;
    servoPatternIndex = 0;
    if (currentState.servoSweepEnabled) {
      servoSweepDirection = 1;
    }
  }

  if (servoJson["intensity"].is<int>()) {
    int intensity = servoJson["intensity"].as<int>();
    if (intensity < 1 || intensity > 100) {
      error = "servo.intensity doit etre compris entre 1 et 100.";
      return false;
    }
    currentState.servoSweepIntensity = static_cast<uint8_t>(intensity);
  }

  if (servoJson["mode"].is<int>()) {
    int mode = servoJson["mode"].as<int>();
    if (mode < 0 || mode > 2) {
      error = "servo.mode doit etre compris entre 0 et 2.";
      return false;
    }
    currentState.servoSweepMode = static_cast<uint8_t>(mode);
    servoPatternIndex = 0;
  }

  if (servoJson["angle"].is<int>()) {
    int angle = servoJson["angle"].as<int>();
    if (angle < 0 || angle > 180) {
      error = "servo.angle doit etre compris entre 0 et 180.";
      return false;
    }

    currentState.servoAngle = angle;
    if (!servoJson["sweep"].is<bool>()) {
      currentState.servoSweepEnabled = false;
    }
    return true;
  }

  if (servoJson["sweep"].is<bool>() || servoJson["intensity"].is<int>()) {
    return true;
  }

  error = "servo.angle ou servo.sweep est obligatoire.";
  return false;
}
