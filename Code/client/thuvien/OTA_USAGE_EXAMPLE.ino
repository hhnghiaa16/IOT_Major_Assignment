/**
 * ============================================================================
 * OTA UPDATE - USAGE EXAMPLE
 * ============================================================================
 * 
 * File này hướng dẫn cách tích hợp OTA Update vào ESP32_client.ino
 * This file shows how to integrate OTA Update into ESP32_client.ino
 * 
 * Các bước tích hợp / Integration steps:
 * 1. Include OTA header
 * 2. Khởi tạo OTA trong setup()
 * 3. (Optional) Tạo task để xử lý OTA updates
 * 4. (Optional) Thêm MQTT topic để trigger OTA update
 * 
 * ============================================================================
 */

#include "wifiStation.h"
#include "settings.h"
#include "mqtt.h"
#include "gpioManager.h"
#include "OTAUpdate.h"  // ✅ BƯỚC 1: Include OTA header
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>
#include "DHT.h"

#define CLIENT_ID "066420c45a4e819437bbfbea63b83739"
#define SLAVE_VERSION  "1.0.0"  // Phiên bản hiện tại của firmware
#define OTA_SERVER_URL "http://192.168.1.100:8080/ota"  // URL của OTA server

// ======= Global References =======
WiFiStation* wifi;
MQTTProtocol* mqtt;
GPIOManager* gpio;
OTAUpdate* ota;  // ✅ BƯỚC 2: Thêm OTA reference

#define DHTPIN 5
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// ======= FreeRTOS Objects =======
QueueHandle_t deviceDataQueue;
QueueHandle_t commandQueue;

// ======= Data Structures =======
struct DeviceData {
    int pin;
    int VirtualPin;
    String value;
    unsigned long timestamp;
};

struct CommandData {
    int VirtualPin;
    String Message;
    unsigned long timestamp;
};

// ======= Task Handles =======
TaskHandle_t wifiTaskHandle = NULL;
TaskHandle_t mqttTaskHandle = NULL;
TaskHandle_t gpioTaskHandle = NULL;
TaskHandle_t readTaskHandle = NULL;
TaskHandle_t otaTaskHandle = NULL;  // ✅ Optional: Task handle cho OTA

// ======= Task Functions =======
void wifiTask(void* parameter);
void mqttTask(void* parameter);
void gpioTask(void* parameter);
void ReadDataTask(void* parameter);
void otaCheckTask(void* parameter);  // ✅ Optional: Task để check OTA updates

// ======= MQTT Callback =======
void mqttCallback(char* topic, byte* payload, unsigned int length);

// ======= OTA Callbacks =======
// ✅ BƯỚC 3: Định nghĩa các callback functions cho OTA

void onOTAStart() {
    Serial.println("🚀 [OTA] Update started!");
    // Có thể gửi notification qua MQTT
    if (mqtt && mqtt->isConnected()) {
        mqtt->publish("device/status", "OTA update started");
    }
}

void onOTAProgress(int progress) {
    Serial.printf("📊 [OTA] Progress: %d%%\n", progress);
    // Có thể cập nhật LED hoặc gửi progress qua MQTT
    
    // Ví dụ: Nhấp nháy LED theo tiến trình
    if (progress % 10 == 0) {
        gpio->writeDigital(4, !gpio->readDigital(4));
    }
}

void onOTAEnd(bool success) {
    if (success) {
        Serial.println("✅ [OTA] Update completed successfully!");
        if (mqtt && mqtt->isConnected()) {
            mqtt->publish("device/status", "OTA update successful, rebooting...");
        }
    } else {
        Serial.println("❌ [OTA] Update failed!");
        if (mqtt && mqtt->isConnected()) {
            mqtt->publish("device/status", "OTA update failed");
        }
    }
}

void onOTAError(const char* error) {
    Serial.printf("❌ [OTA] Error: %s\n", error);
    if (mqtt && mqtt->isConnected()) {
        String errorMsg = "OTA error: " + String(error);
        mqtt->publish("device/status", errorMsg.c_str());
    }
}

// ======= Setup Function =======
void setup() {
    Serial.begin(115200);
    delay(2000);
    
    Settings deviceSettings("device", true);
    deviceSettings.setString("clientId", CLIENT_ID);
    deviceSettings.setString("slave_version", SLAVE_VERSION);

    Serial.println("🚀 Starting ESP32 Multi-Thread IoT Device...");
    Serial.printf("📊 Free heap at start: %d bytes\n", ESP.getFreeHeap());
    
    // Initialize NVS
    Settings::initializeNVS();
    Settings::printNVSInfo();
    
    // Get singleton references
    wifi = &WiFiStation::getInstance();
    mqtt = &MQTTProtocol::getInstance();
    gpio = &GPIOManager::getInstance();
    ota = &OTAUpdate::getInstance();  // ✅ BƯỚC 4: Lấy OTA instance
    
    // Initialize WiFi (blocking until connected)
    wifi->begin();
    
    // Initialize MQTT
    mqtt->updateConfig("10.1.1.20", 1883, CLIENT_ID);
    mqtt->begin();
    mqtt->setCallback(mqttCallback);
    
    // Initialize GPIO
    gpio->begin();
    gpio->setOutputPin(4, true);
    gpio->saveGPIOConfig();
    dht.begin();
    
    // ✅ BƯỚC 5: Khởi tạo OTA Update
    // Tham số: (server_url, current_version, device_id, check_interval_ms)
    ota->begin(
        OTA_SERVER_URL,     // URL của OTA server
        SLAVE_VERSION,      // Phiên bản hiện tại
        CLIENT_ID,          // Device ID
        3600000            // Kiểm tra mỗi 1 giờ (3600000 ms)
    );
    
    // ✅ BƯỚC 6: Đăng ký các callback functions
    ota->setOnStartCallback(onOTAStart);
    ota->setOnProgressCallback(onOTAProgress);
    ota->setOnEndCallback(onOTAEnd);
    ota->setOnErrorCallback(onOTAError);
    
    // ✅ BƯỚC 7: In thông tin OTA
    ota->printInfo();

    // Create FreeRTOS objects
    deviceDataQueue = xQueueCreate(10, sizeof(DeviceData));
    commandQueue = xQueueCreate(10, sizeof(CommandData));
    
    if (deviceDataQueue == NULL || commandQueue == NULL ) {
        Serial.println("❌ Failed to create FreeRTOS objects!");
        return;
    }
    
    // Create tasks
    xTaskCreatePinnedToCore(wifiTask, "WiFiTask", 4096, NULL, 2, &wifiTaskHandle, 0);
    xTaskCreatePinnedToCore(mqttTask, "MQTTTask", 4096, NULL, 3, &mqttTaskHandle, 1);
    xTaskCreatePinnedToCore(gpioTask, "GPIOTask", 4096, NULL, 2, &gpioTaskHandle, 0);
    xTaskCreatePinnedToCore(ReadDataTask, "SensorTask", 4096, NULL, 1, &readTaskHandle, 1);
    
    // ✅ Optional: Tạo task riêng để xử lý OTA (nếu muốn tùy chỉnh nhiều hơn)
    // xTaskCreatePinnedToCore(otaCheckTask, "OTACheckTask", 8192, NULL, 1, &otaTaskHandle, 1);
    
    Serial.println("✅ All tasks created successfully!");
    Serial.printf("📊 Free heap after setup: %d bytes\n", ESP.getFreeHeap());
    
    // Subscribe to MQTT topics
    mqtt->registerVirtualpin(true, 4);
    mqtt->registerVirtualpin(false, 5);
    
    // ✅ BƯỚC 8: Subscribe to OTA control topic (optional)
    // mqtt->subscribe("device/" + String(CLIENT_ID) + "/ota");
    
    Serial.println("🎉 Setup completed successfully!");
}

// ======= Loop Function =======
void loop() {
    vTaskDelay(pdMS_TO_TICKS(10000));
    Serial.printf("📊 System Status - Free heap: %d bytes, Uptime: %d seconds\n", 
                  ESP.getFreeHeap(), millis() / 1000);
}

// ======= WiFi Task (Core 0) =======
void wifiTask(void* parameter) {
    Serial.println("📡 [WiFiTask] Started on Core 0");
    
    while (true) {
        wifi->loop();
        
        if (!wifi->isConnected()) {
            Serial.println("⚠️ [WiFiTask] WiFi disconnected!");
        }
        
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

// ======= MQTT Task (Core 1) =======
void mqttTask(void* parameter) {
    Serial.println("📨 [MQTTTask] Started on Core 1");
    
    while (true) {
        if (wifi->isConnected()) {
            mqtt->loop();
            
            // Process sensor data queue
            DeviceData deviceData;
            if (xQueueReceive(deviceDataQueue, &deviceData, 0) == pdTRUE) {
                mqtt->send(deviceData.VirtualPin, deviceData.value , false , false);
            }
        }
        
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

// ======= GPIO Task (Core 0) =======
void gpioTask(void* parameter) {
    Serial.println("🔌 [GPIOTask] Started on Core 0");
    
    while (true) {
        CommandData commandData;
        if (xQueueReceive(commandQueue, &commandData, 0) == pdTRUE) {
            if(commandData.Message.substring(0 , 2) == "ER") {
                Serial.printf("📤 [ERROR] SERVER ERROR: %s ", 
                    commandData.Message.c_str());
                continue;
            }
            gpio->processCommand(commandData.VirtualPin, commandData.Message , 1);
        }
        
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

// ======= Sensor Task (Core 1) =======
void ReadDataTask(void* parameter) {
    Serial.println("🌡️ [SensorTask] Started on Core 1");
    float t, h;
    
    while (true) {
        h = dht.readHumidity();
        t = dht.readTemperature();
        
        DeviceData deviceData;
        deviceData.pin = 5;
        deviceData.VirtualPin = 5;
        deviceData.value = String(t);
        deviceData.timestamp = millis();
        xQueueSend(deviceDataQueue, &deviceData, pdMS_TO_TICKS(10));
        
        vTaskDelay(pdMS_TO_TICKS(8000));
    }
}

// ✅ BƯỚC 9: Optional - Tạo task riêng để kiểm tra OTA updates theo cách custom
void otaCheckTask(void* parameter) {
    Serial.println("🔄 [OTACheckTask] Started on Core 1");
    
    while (true) {
        // Đợi 5 phút trước khi kiểm tra update lần đầu
        vTaskDelay(pdMS_TO_TICKS(300000));
        
        Serial.println("⏰ [OTACheckTask] Checking for updates...");
        
        if (wifi->isConnected()) {
            // Kiểm tra xem có version mới không
            if (ota->hasNewVersion()) {
                Serial.println("🎉 [OTACheckTask] New version available!");
                
                // Có thể gửi notification qua MQTT
                if (mqtt && mqtt->isConnected()) {
                    mqtt->publish("device/notification", "New firmware available");
                }
                
                // Tùy chọn: Tự động update hoặc đợi lệnh từ server
                // ota->performUpdate(true);  // Force update ngay
            }
        }
        
        // Kiểm tra mỗi 1 giờ
        vTaskDelay(pdMS_TO_TICKS(3600000));
    }
}

// ======= MQTT Callback =======
void mqttCallback(char* topic, byte* payload, unsigned int length) {
    String message = "";
    for (int i = 0; i < length; i++) {
        message += (char)payload[i];
    }
    
    Serial.printf("📨 [MQTT] Received: %s = %s\n", topic, message.c_str());
    
    String topicStr = String(topic);
    String type = topicStr.substring(0, topicStr.indexOf("/"));
    String clientId = topicStr.substring(topicStr.indexOf("/") + 1, topicStr.lastIndexOf("/"));
    String virtualPin = topicStr.substring(topicStr.lastIndexOf("/") + 1);
    
    // ✅ BƯỚC 10: Xử lý OTA commands từ MQTT
    if (type == "OTA" || virtualPin == "ota") {
        Serial.println("🔄 [MQTT] OTA command received!");
        
        if (message == "update" || message == "UPDATE") {
            Serial.println("🚀 [MQTT] Triggering OTA update...");
            // Trigger OTA update trong task riêng để không block MQTT callback
            xTaskCreate([](void* param) {
                ota->performUpdate(true);  // Force update
                vTaskDelete(NULL);
            }, "OTAUpdateTask", 8192, NULL, 1, NULL);
        } 
        else if (message == "check" || message == "CHECK") {
            Serial.println("🔍 [MQTT] Checking for OTA updates...");
            xTaskCreate([](void* param) {
                if (ota->hasNewVersion()) {
                    mqtt->publish("device/notification", "New firmware available");
                } else {
                    mqtt->publish("device/notification", "Already on latest version");
                }
                vTaskDelete(NULL);
            }, "OTACheckTask", 8192, NULL, 1, NULL);
        }
        else if (message == "version" || message == "VERSION") {
            String versionMsg = "Current version: " + ota->getCurrentVersion();
            mqtt->publish("device/notification", versionMsg.c_str());
        }
        else if (message == "info" || message == "INFO") {
            ota->printInfo();
        }
        
        return;
    }
    
    // Xử lý các MQTT commands khác như bình thường
    if (type == "CT") {
        CommandData commandData;
        commandData.VirtualPin = virtualPin.toInt();
        commandData.Message = String(message);
        commandData.timestamp = millis();
        xQueueSend(commandQueue, &commandData, pdMS_TO_TICKS(10));
    } 
    else if (type == "SS") {
        CommandData commandData;
        commandData.VirtualPin = virtualPin.toInt();
        commandData.Message = "ER " + String(message);
        commandData.timestamp = millis();
        xQueueSend(commandQueue, &commandData, pdMS_TO_TICKS(10));
    } 
    else if (type == "NC") {
        Serial.printf("📨 [MQTT] Received notification: %s\n", message.c_str());
    }
}

/**
 * ============================================================================
 * MQTT TOPICS FOR OTA CONTROL
 * ============================================================================
 * 
 * Các MQTT topics để điều khiển OTA / MQTT topics for OTA control:
 * 
 * 1. Trigger update:
 *    Topic: OTA/{CLIENT_ID}/ota
 *    Payload: "update" hoặc "UPDATE"
 * 
 * 2. Check for updates:
 *    Topic: OTA/{CLIENT_ID}/ota
 *    Payload: "check" hoặc "CHECK"
 * 
 * 3. Get version:
 *    Topic: OTA/{CLIENT_ID}/ota
 *    Payload: "version" hoặc "VERSION"
 * 
 * 4. Print info:
 *    Topic: OTA/{CLIENT_ID}/ota
 *    Payload: "info" hoặc "INFO"
 * 
 * ============================================================================
 * OTA SERVER API REQUIREMENTS
 * ============================================================================
 * 
 * OTA server cần implement các endpoints sau:
 * 
 * 1. Check for updates:
 *    GET /ota/check?device_id={CLIENT_ID}&current_version={VERSION}
 *    
 *    Response (JSON):
 *    {
 *      "has_update": true,
 *      "version": "1.0.1",
 *      "download_url": "http://192.168.1.100:8080/ota/download/firmware.bin",
 *      "changelog": "Bug fixes and improvements"
 *    }
 * 
 * 2. Download firmware:
 *    GET /ota/download/firmware.bin
 *    
 *    Response: Binary firmware file (.bin)
 * 
 * ============================================================================
 */

