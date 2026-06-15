#include "SensorManager.h"
#include <time.h>

bool SensorManager::begin() {
  pinMode(PIN_JOYSTICK_X, INPUT);
  pinMode(PIN_JOYSTICK_Y, INPUT);
  pinMode(PIN_WIRE_CONTACT, INPUT_PULLUP);
  analogSetPinAttenuation(PIN_JOYSTICK_X, ADC_11db);
  analogSetPinAttenuation(PIN_JOYSTICK_Y, ADC_11db);

  dht.begin();
  delay(1500);

  const float initialTemp = dht.readTemperature();
  const float initialHumidity = dht.readHumidity();
  currentStatus.dhtOk = isValidTemperature(initialTemp) && isValidHumidity(initialHumidity);

  currentStatus.initialized = true;
  currentStatus.lastReadMs = millis();

  Serial.printf("[SENSORS] DHT22: %s\n", currentStatus.dhtOk ? "OK" : "NON DETECTE");
  return currentStatus.dhtOk;
}

bool SensorManager::read(SensorSample& sample) {
  currentStatus.sequence++;
  sample.seq = currentStatus.sequence;
  sample.ts = epochOrUptime();
  const uint16_t joystickX = analogRead(PIN_JOYSTICK_X);
  const uint16_t joystickY = analogRead(PIN_JOYSTICK_Y);
  sample.joystickXRaw = joystickX;
  sample.joystickYRaw = joystickY;
  sample.joystickPercent = joystickPercentFromAxes(joystickX, joystickY);
  sample.wireContactClosed = digitalRead(PIN_WIRE_CONTACT) == LOW;
  sample.anomaly = false;

  const float rawTemp = dht.readTemperature();
  const float rawHumidity = dht.readHumidity();

  const bool tempOk = isValidTemperature(rawTemp);
  const bool humOk = isValidHumidity(rawHumidity);
  currentStatus.dhtOk = tempOk && humOk;

  if (!tempOk || !humOk) {
    sample.anomaly = true;
    currentStatus.rejectedCount++;
  }

  if (tempOk) {
    filteredTemp = applyEma(filteredTemp, rawTemp);
  }

  if (humOk) {
    filteredHumidity = applyEma(filteredHumidity, rawHumidity);
  }

  sample.temp = isnan(filteredTemp) ? 0.0f : filteredTemp;
  sample.humidity = isnan(filteredHumidity) ? 0.0f : filteredHumidity;

  currentStatus.lastReadMs = millis();
  return !sample.anomaly;
}

SensorStatus SensorManager::status() const {
  return currentStatus;
}

bool SensorManager::isValidTemperature(float value) {
  return !isnan(value) && value >= -20.0f && value <= 80.0f;
}

bool SensorManager::isValidHumidity(float value) {
  return !isnan(value) && value >= 0.0f && value <= 100.0f;
}

uint32_t SensorManager::epochOrUptime() {
  time_t now = time(nullptr);
  if (now > 1700000000) {
    return static_cast<uint32_t>(now);
  }
  return millis() / 1000;
}

uint8_t SensorManager::joystickPercentFromAxes(uint16_t xRaw, uint16_t yRaw) {
  if (xRaw < JOYSTICK_LOW_THRESHOLD) {
    return 25;
  }
  if (yRaw > JOYSTICK_HIGH_THRESHOLD) {
    return 50;
  }
  if (xRaw > JOYSTICK_HIGH_THRESHOLD) {
    return 75;
  }
  if (yRaw < JOYSTICK_LOW_THRESHOLD) {
    return 100;
  }
  return 0;
}

float SensorManager::applyEma(float previous, float value) const {
  if (isnan(previous)) {
    return value;
  }
  return (ALPHA * value) + ((1.0f - ALPHA) * previous);
}
