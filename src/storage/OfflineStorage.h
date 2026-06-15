#pragma once

#include <Arduino.h>
#include <functional>

class OfflineStorage {
public:
  bool begin();
  bool appendPayload(const String& payload);
  size_t pendingBytes() const;
  uint32_t droppedCount() const;
  uint32_t replay(std::function<bool(const String&)> publishFn);

private:
  static constexpr const char* OFFLINE_FILE = "/offline.jsonl";
  static constexpr const char* TEMP_FILE = "/offline.tmp";
  static constexpr size_t MAX_QUEUE_BYTES = 64 * 1024;

  uint32_t dropped = 0;

  bool rotateIfNeeded();
};
