#include <Arduino.h>
#include "sensors/SensorManager.h"
#include "actuators/ActuatorManager.h"
#include "network/MqttClientManager.h"
#include "storage/OfflineStorage.h"
#include "storage/ConfigStorage.h"
#include "web/WebServerManager.h"
#include "security/SecurityManager.h"

static SensorManager sensorManager;
static ActuatorManager actuatorManager;
static OfflineStorage offlineStorage;
static ConfigStorage configStorage;
static SecurityManager securityManager;
static MqttClientManager mqttManager;
static WebServerManager webServerManager;

static MqttRuntimeConfig runtimeConfig;

static QueueHandle_t sensorQueue = nullptr;
static SemaphoreHandle_t stateMutex = nullptr;

static SensorSample latestSample{};
static SensorStatus latestSensorStatus{};

static void emergencyTask(void* parameter) {
  (void)parameter;

  TickType_t lastWake = xTaskGetTickCount();

  for (;;) {
    const bool emergencyActive = sensorManager.emergencyStopActive();
    actuatorManager.setEmergencyStop(emergencyActive);

    if (xSemaphoreTake(stateMutex, pdMS_TO_TICKS(5)) == pdTRUE) {
      latestSample.wireContactClosed = emergencyActive;
      xSemaphoreGive(stateMutex);
    }

    vTaskDelayUntil(&lastWake, pdMS_TO_TICKS(20));
  }
}

static void sensorTask(void* parameter) {
  (void)parameter;

  TickType_t lastWake = xTaskGetTickCount();

  for (;;) {
    SensorSample sample{};
    snprintf(sample.device, sizeof(sample.device), "%s", runtimeConfig.deviceId.c_str());

    sensorManager.read(sample);

    if (xSemaphoreTake(stateMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
      latestSample = sample;
      latestSensorStatus = sensorManager.status();
      xSemaphoreGive(stateMutex);
    }

    xQueueSend(sensorQueue, &sample, 0);

    vTaskDelayUntil(&lastWake, pdMS_TO_TICKS(2000));
  }
}

static void networkTask(void* parameter) {
  (void)parameter;

  SensorSample sample{};

  for (;;) {
    mqttManager.loopOnce();

    while (xQueueReceive(sensorQueue, &sample, 0) == pdTRUE) {
      mqttManager.publishSample(sample);
    }

    vTaskDelay(pdMS_TO_TICKS(100));
  }
}

static void webTask(void* parameter) {
  (void)parameter;

  webServerManager.begin(
    &runtimeConfig,
    &latestSample,
    &latestSensorStatus,
    &stateMutex,
    &mqttManager,
    &actuatorManager,
    &offlineStorage,
    &securityManager,
    [](const MqttRuntimeConfig& config) {
      configStorage.save(config);
    }
  );

  vTaskDelete(nullptr);
}

static void actuatorTask(void* parameter) {
  (void)parameter;

  for (;;) {
    actuatorManager.update();
    vTaskDelay(pdMS_TO_TICKS(20));
  }
}

static void supervisionTask(void* parameter) {
  (void)parameter;

  TickType_t lastWake = xTaskGetTickCount();

  for (;;) {
    NetworkStatus net = mqttManager.status();

    Serial.printf(
      "[SUPERVISION] heap=%u uptime=%us wifi=%d mqtt=%d latency=%ums offline=%u\n",
      ESP.getFreeHeap(),
      millis() / 1000,
      net.wifiConnected,
      net.mqttConnected,
      net.lastPublishLatencyMs,
      offlineStorage.pendingBytes()
    );

    vTaskDelayUntil(&lastWake, pdMS_TO_TICKS(10000));
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println("[BOOT] Systeme IoT securise et autonome");

  sensorQueue = xQueueCreate(16, sizeof(SensorSample));
  stateMutex = xSemaphoreCreateMutex();

  runtimeConfig = configStorage.load();
  securityManager.begin(runtimeConfig.apiToken);

  offlineStorage.begin();
  sensorManager.begin();
  actuatorManager.begin();

  latestSensorStatus = sensorManager.status();
  snprintf(latestSample.device, sizeof(latestSample.device), "%s", runtimeConfig.deviceId.c_str());

  mqttManager.begin(runtimeConfig, &offlineStorage, &actuatorManager, &securityManager);

  xTaskCreatePinnedToCore(emergencyTask, "task_emergency", 2048, nullptr, 4, nullptr, 1);
  xTaskCreatePinnedToCore(networkTask, "task_network", 6144, nullptr, 3, nullptr, 0);
  xTaskCreatePinnedToCore(webTask, "task_web", 6144, nullptr, 3, nullptr, 1);
  xTaskCreatePinnedToCore(sensorTask, "task_sensors", 4096, nullptr, 2, nullptr, 1);
  xTaskCreatePinnedToCore(actuatorTask, "task_actuators", 3072, nullptr, 2, nullptr, 1);
  xTaskCreatePinnedToCore(supervisionTask, "task_supervision", 3072, nullptr, 1, nullptr, 0);
}

void loop() {
  // La logique metier est repartie dans les taches FreeRTOS.
  // loop() reste inactive et cede le CPU toutes les 1000 ms.
  static constexpr TickType_t LOOP_IDLE_DELAY = pdMS_TO_TICKS(1000);
  vTaskDelay(LOOP_IDLE_DELAY);
}
