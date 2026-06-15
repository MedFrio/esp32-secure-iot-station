#include "OfflineStorage.h"
#include <SPIFFS.h>

bool OfflineStorage::begin() {
  if (!SPIFFS.begin(true)) {
    Serial.println("[STORAGE] SPIFFS impossible a monter");
    return false;
  }

  Serial.printf("[STORAGE] SPIFFS OK, offline=%u octets\n", pendingBytes());
  return true;
}

bool OfflineStorage::appendPayload(const String& payload) {
  if (!rotateIfNeeded()) {
    return false;
  }

  File file = SPIFFS.open(OFFLINE_FILE, FILE_APPEND);
  if (!file) {
    Serial.println("[STORAGE] Impossible d'ouvrir la file offline");
    return false;
  }

  file.println(payload);
  file.close();
  return true;
}

size_t OfflineStorage::pendingBytes() const {
  if (!SPIFFS.exists(OFFLINE_FILE)) {
    return 0;
  }

  File file = SPIFFS.open(OFFLINE_FILE, FILE_READ);
  if (!file) {
    return 0;
  }

  size_t size = file.size();
  file.close();
  return size;
}

uint32_t OfflineStorage::droppedCount() const {
  return dropped;
}

uint32_t OfflineStorage::replay(std::function<bool(const String&)> publishFn) {
  if (!SPIFFS.exists(OFFLINE_FILE)) {
    return 0;
  }

  File input = SPIFFS.open(OFFLINE_FILE, FILE_READ);
  if (!input) {
    return 0;
  }

  File output = SPIFFS.open(TEMP_FILE, FILE_WRITE);
  if (!output) {
    input.close();
    return 0;
  }

  uint32_t replayed = 0;
  bool keepRest = false;

  while (input.available()) {
    String line = input.readStringUntil('\n');
    line.trim();

    if (line.length() == 0) {
      continue;
    }

    if (!keepRest && publishFn(line)) {
      replayed++;
    } else {
      keepRest = true;
      output.println(line);
    }
  }

  input.close();
  output.close();

  SPIFFS.remove(OFFLINE_FILE);

  if (SPIFFS.exists(TEMP_FILE)) {
    File temp = SPIFFS.open(TEMP_FILE, FILE_READ);
    const bool hasRemaining = temp && temp.size() > 0;
    if (temp) {
      temp.close();
    }

    if (hasRemaining) {
      SPIFFS.rename(TEMP_FILE, OFFLINE_FILE);
    } else {
      SPIFFS.remove(TEMP_FILE);
    }
  }

  if (replayed > 0) {
    Serial.printf("[STORAGE] %u messages offline retransmis\n", replayed);
  }

  return replayed;
}

bool OfflineStorage::rotateIfNeeded() {
  size_t size = pendingBytes();
  if (size < MAX_QUEUE_BYTES) {
    return true;
  }

  SPIFFS.remove(OFFLINE_FILE);
  dropped++;
  Serial.println("[STORAGE] File offline pleine, rotation effectuee");
  return true;
}
