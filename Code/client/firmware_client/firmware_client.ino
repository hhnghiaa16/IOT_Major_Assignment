#include "wifiStation.h"
#include "settings.h"
#include "mqtt.h"
#include "gpioManager.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>
#include "OTAUpdate.h"
#include "DHT.h"
#define CLIENT_ID "066420c45a4e819437bbfbea63b83739"
#define version  "Slave_1.0.1"
#define OTA_SERVER_URL "http://192.168.3.102:8000/ota/get_info_update"
// ======= Global References =======
WiFiStation* wifi;
MQTTProtocol* mqtt;
GPIOManager* gpio;
OTAUpdate* ota;
#define DHTPIN 5
#define DHTTYPE DHT11
// DHT dht(DHTPIN, DHTTYPE);
// ======= FreeRTOS Objects =======
QueueHandle_t deviceDataQueue;
QueueHandle_t commandQueue;


// ======= Data Structures =======
#define MAX_VALUE_LEN 128  // Độ dài tối đa cho value string
#define MAX_MSG_LEN 64     // Độ dài tối đa cho message string

struct DeviceData {
    int pin;
    int VirtualPin;
    char value[MAX_VALUE_LEN];  // Thay String bằng char array
    unsigned long timestamp;
    bool isNC;
};

struct CommandData {
    int VirtualPin;
    char Message[MAX_MSG_LEN];  // Thay String bằng char array
    unsigned long timestamp;
};

// ======= Task Handles =======
TaskHandle_t wifiTaskHandle = NULL;
TaskHandle_t mqttTaskHandle = NULL;
TaskHandle_t gpioTaskHandle = NULL;
TaskHandle_t readTaskHandle = NULL;
TaskHandle_t otaTaskHandle = NULL;  // Task cho OTA update (tạo động khi cần)

// ======= Task Functions =======
void wifiTask(void* parameter);
void mqttTask(void* parameter);
void gpioTask(void* parameter);
void ReadDataTask(void* parameter);
void otaTask(void* parameter);  // Task xử lý OTA update

// ======= MQTT Callback =======
void mqttCallback(char* topic, byte* payload, unsigned int length);

// ======= Setup Function =======
void setup() {
    Serial.begin(115200);
    delay(2000);
    Settings deviceSettings("device", true);
    deviceSettings.setString("clientId", CLIENT_ID);
    deviceSettings.setString("slave_version", version);

    Serial.println("🚀 Starting ESP32 Multi-Thread IoT Device...");
    Serial.printf("📊 Free heap at start: %d bytes\n", ESP.getFreeHeap());
    
    // Initialize NVS
    Settings::initializeNVS();
    
    // Kiểm tra dung lượng NVS (có thể xóa nếu không cần)
    Settings::printNVSInfo();
    
    // Get singleton references
    wifi = &WiFiStation::getInstance();
    mqtt = &MQTTProtocol::getInstance();
    gpio = &GPIOManager::getInstance();
    ota = &OTAUpdate::getInstance();
    // Initialize WiFi (blocking until connected)
    wifi->begin();
    ota->begin( // nó sẽ tạo luồng mới để chạy nên có thể là broker server sẽ không được update kịp thời 
        OTA_SERVER_URL,     // Server URL
        version,      // Current version
        CLIENT_ID,          // Device ID
        3600000            // Check interval (1 hour)
    );
    if (ota->hasNewVersion()) { // bắt buộc phả check version trước khi bắt đầu kết nối mqtt 
        Serial.println("🔄 [OTA] New version available!");
        // Serial.printf("   📌 New Version: %s\n", ota->newVersion.c_str());
        // Serial.printf("   📌 Download URL: %s\n", ota->downloadUrl.c_str());
        // Serial.printf("   📌 Changelog: %s\n", ota->changelog.c_str());
    }
    else {
        Serial.println("✅ [OTA] No new version available!");
    }
    // Initialize MQTT
    // mqtt->updateConfig("10.1.1.20", 1883, CLIENT_ID); // Laáy broker sau khi OTA check update xong 
    mqtt->begin();
    mqtt->setCallback(mqttCallback);
    
    // Initialize GPIO
    gpio->begin();
    gpio->setOutputPin(4, true);
    gpio->saveGPIOConfig(); // luu cau hinh GPIO vao NVS
    // dht.begin(); // chan 5 lam cam bien nhiet do 

    // Initialize OTA
    // Create FreeRTOS objects
    deviceDataQueue = xQueueCreate(10, sizeof(DeviceData));
    commandQueue = xQueueCreate(10, sizeof(CommandData));
    
    if (deviceDataQueue == NULL || commandQueue == NULL) {
        Serial.println("❌ Failed to create FreeRTOS objects!");
        return;
    }
    
    // Create tasks
    xTaskCreatePinnedToCore(
        wifiTask,           // Task function
        "WiFiTask",         // Task name
        4096,               // Stack size
        NULL,               // Parameters
        2,                  // Priority
        &wifiTaskHandle,    // Task handle
        0                   // Core 0
    );
    
    xTaskCreatePinnedToCore(
        mqttTask,           // Task function
        "MQTTTask",         // Task name
        4096,               // Stack size
        NULL,               // Parameters
        3,                  // Priority
        &mqttTaskHandle,    // Task handle
        1                   // Core 1
    );
    
    xTaskCreatePinnedToCore(
        gpioTask,           // Task function
        "GPIOTask",         // Task name
        4096,               // Stack size
        NULL,               // Parameters
        2,                  // Priority
        &gpioTaskHandle,    // Task handle
        0                   // Core 0
    );
    
    xTaskCreatePinnedToCore(
        ReadDataTask,         // Task function
        "SensorTask",       // Task name
        4096,               // Stack size
        NULL,               // Parameters
        1,                  // Priority
        &readTaskHandle,  // Task handle
        1                   // Core 1
    );
    
    // OTA Task - KHÔNG tạo ở đây, sẽ tạo động khi cần để tiết kiệm 16KB RAM
    // otaTaskHandle sẽ được tạo trong mqttCallback khi nhận "OTA:UP"
    
    Serial.println("✅ All tasks created successfully!");
    Serial.printf("📊 Free heap after setup: %d bytes\n", ESP.getFreeHeap());
    
    // Subscribe to MQTT topics
    // Mutex đã được xử lý bên trong mqtt->subscribe()
    // phải đăng kí các topic trước khi gửi nhận mới được .
    mqtt->registerVirtualpin(REG_SS, 4);
    mqtt->registerVirtualpin(REG_CT, 5);
    mqtt->registerVirtualpin(REG_NC, 0);
    // mqtt->subscribe("device/gpio");
    // mqtt->subscribe("device/led");
    
    Serial.println("🎉 Setup completed successfully!");
    Serial.println("Code test ota=================================================");
}

// ======= Loop Function =======
void loop() {
    // Main loop is now handled by FreeRTOS tasks
    // This loop can be used for monitoring or low-priority tasks
    vTaskDelay(pdMS_TO_TICKS(10000)); // 10 seconds
    
    Serial.printf("📊 System Status - Free heap: %d bytes, Uptime: %d seconds\n", 
                  ESP.getFreeHeap(), millis() / 1000);
}

// ======= WiFi Task (Core 0) =======
void wifiTask(void* parameter) {
    Serial.println("📡 [WiFiTask] Started on Core 0");
    
    while (true) {
        wifi->loop();
        
        // Check WiFi status
        if (!wifi->isConnected()) {
            Serial.println("⚠️ [WiFiTask] WiFi disconnected!");
        }
        
        vTaskDelay(pdMS_TO_TICKS(1000)); // 1 second
    }
}

// ======= MQTT Task (Core 1) =======
void mqttTask(void* parameter) {
    Serial.println("📨 [MQTTTask] Started on Core 1");
    
    while (true) {
        if (wifi->isConnected()) {
            // Mutex đã được xử lý bên trong mqtt->loop()
            mqtt->loop();
            
            // Process sensor data queue
            DeviceData deviceData;
            if (xQueueReceive(deviceDataQueue, &deviceData, 0) == pdTRUE) {
                if(deviceData.isNC){
                    mqtt->send(deviceData.VirtualPin, String(deviceData.value), false, true);
                }
                else{
                    mqtt->send(deviceData.VirtualPin, String(deviceData.value), false, false);
                }
            }
        }
        
        vTaskDelay(pdMS_TO_TICKS(100)); // 100ms
    }
}

// ======= GPIO Task (Core 0) =======
void gpioTask(void* parameter) {
    Serial.println("🔌 [GPIOTask] Started on Core 0");
    
    while (true) {
        // gpio->loop();
        
        // Process command queue
        CommandData commandData;
        if (xQueueReceive(commandQueue, &commandData, 0) == pdTRUE) {
            if(strncmp(commandData.Message, "ER", 2) == 0) {
                Serial.printf("📤 [ERROR] SERVER ERROR: %s ", commandData.Message);
                continue;
            }
            // cho virtual pin bằng với pin thật 
            gpio->processCommand(commandData.VirtualPin, String(commandData.Message), 1);
        }
        
        vTaskDelay(pdMS_TO_TICKS(100)); // 100ms
    }
}

// ======= Sensor Task (Core 1) =======
void ReadDataTask(void* parameter) {
    Serial.println("🌡️ [SensorTask] Started on Core 1");
    float t ;
    float h ;
    while (true) {
        //  h = dht.readHumidity();
        //  t = dht.readTemperature();
        //222222
        // Read sensors and send data
        // DeviceData deviceData;
        
        // // Read digital pins
        // deviceData.pin = 5;
        // deviceData.VirtualPin = 5;
        // deviceData.value = String(t);
        // deviceData.timestamp = millis();
        // xQueueSend(deviceDataQueue, &deviceData, pdMS_TO_TICKS(10));
        
        // sensorData.sensorName = "pin_4";
        // sensorData.value = String(gpio->readDigital(4));
        // sensorData.timestamp = millis();
        // xQueueSend(deviceDataQueue, &deviceData, pdMS_TO_TICKS(10));
        
        // // Read analog pins
        // sensorData.sensorName = "pin_36_analog";
        // sensorData.value = String(gpio->readAnalog(36));
        // sensorData.timestamp = millis();
        // xQueueSend(deviceDataQueue, &deviceData, pdMS_TO_TICKS(10));
        
        // // System info
        // sensorData.sensorName = "system_uptime";
        // sensorData.value = String(millis() / 1000);
        // sensorData.timestamp = millis();
        // xQueueSend(deviceDataQueue, &deviceData, pdMS_TO_TICKS(10));
        
        // sensorData.sensorName = "system_free_heap";
        // sensorData.value = String(ESP.getFreeHeap());
        // sensorData.timestamp = millis();
        // xQueueSend(deviceDataQueue, &deviceData, pdMS_TO_TICKS(10));
        
        vTaskDelay(pdMS_TO_TICKS(8000)); // 5 seconds
    }
}

// ======= MQTT Callback =======
void mqttCallback(char* topic, byte* payload, unsigned int length) {
    String message = "";
    for (int i = 0; i < length; i++) {
        message = message + String((char)payload[i]);
    }
    
    Serial.printf("📨 [MQTT] Received: %s = %s\n", topic, message.c_str());
    
    // Process different topics
    String topicStr = String(topic);
    String type = topicStr.substring(0, topicStr.indexOf("/"));
    String clientId = topicStr.substring(topicStr.indexOf("/") + 1, topicStr.lastIndexOf("/"));
    String virtualPin = topicStr.substring(topicStr.lastIndexOf("/") + 1);

    // giai nen message dangj json 
    if (type == "CT") {
        // Parse JSON command
        CommandData commandData;
        commandData.VirtualPin = virtualPin.toInt();
        strncpy(commandData.Message, message.c_str(), MAX_MSG_LEN - 1);
        commandData.Message[MAX_MSG_LEN - 1] = '\0';  // Đảm bảo null-terminated
        commandData.timestamp = millis();
        xQueueSend(commandQueue, &commandData, pdMS_TO_TICKS(10));
    } else if (type == "SS") {
        // Parse JSON command
        CommandData commandData;
        commandData.VirtualPin = virtualPin.toInt();
        String errMsg = "ER " + message;
        strncpy(commandData.Message, errMsg.c_str(), MAX_MSG_LEN - 1);
        commandData.Message[MAX_MSG_LEN - 1] = '\0';
        commandData.timestamp = millis();
        xQueueSend(commandQueue, &commandData, pdMS_TO_TICKS(10));
    } else if (type == "NC") {
        Serial.printf("📨 [MQTT] Received notification: %s\n", message.c_str());
        if(message.substring(0, 3) == "OTA") 
        {
            // OTA : 
            // Server :
            //     CK : kiểm tra từ server về phiên bản mới nhất 
            //     UP : yêu cầu cập nhật phiên bản mới nhất từ server về 
            // CLient :
            //     UPDATING : đang cập nhật phiên bản mới nhất từ server về 
            //     INFO : thông tin phiên bản mới nhất từ server về 
            //     ERROR : lỗi khi cập nhật phiên bản mới nhất từ server về 

            if(message.substring(4, 6) == "CK") { //"OTA:CK" // đây là yêu cầu kiểm tra từ server về phiên bản mới nhất 
                String info = ota->Getinfo4mqtt();
                DeviceData deviceData;
                deviceData.VirtualPin = 0;
                deviceData.pin = 0;
                strncpy(deviceData.value, info.c_str(), MAX_VALUE_LEN - 1);
                deviceData.value[MAX_VALUE_LEN - 1] = '\0';
                deviceData.timestamp = millis();
                deviceData.isNC = true;
                xQueueSend(deviceDataQueue, &deviceData, pdMS_TO_TICKS(10));
            }
            if(message.substring(4, 6) == "UP") { //"OTA:UP" 
                // Chỉ set flag, OTA task sẽ thực hiện update
                // (tránh stack overflow vì HTTPS cần stack rất lớn)
                if(ota->hasNewVersion()){
                    DeviceData deviceData;
                    deviceData.VirtualPin = 0;
                    deviceData.pin = 0;
                    strncpy(deviceData.value, "OTA:UPDATING@0", MAX_VALUE_LEN - 1);
                    deviceData.value[MAX_VALUE_LEN - 1] = '\0';
                    deviceData.timestamp = millis();
                    deviceData.isNC = true;
                    xQueueSend(deviceDataQueue, &deviceData, pdMS_TO_TICKS(10));
                    
                    // Tạo OTA task ĐỘNG khi cần (tiết kiệm 16KB RAM)
                    if (otaTaskHandle == NULL) {
                        xTaskCreatePinnedToCore(
                            otaTask,
                            "OTATask",
                            16384,          // 16KB stack cho HTTPS/SSL
                            NULL,
                            2,
                            &otaTaskHandle,
                            1
                        );
                        Serial.println("📤 [OTA] Created OTA task with 16KB stack");
                    } else {
                        Serial.println("⚠️ [OTA] OTA task already running");
                    }
                }
                else{
                    DeviceData deviceData;
                    deviceData.VirtualPin = 0;
                    deviceData.pin = 0;
                    strncpy(deviceData.value, "OTA:ERROR@ da co phien ban moi nhat", MAX_VALUE_LEN - 1);
                    deviceData.value[MAX_VALUE_LEN - 1] = '\0';
                    deviceData.timestamp = millis();
                    deviceData.isNC = true;
                    xQueueSend(deviceDataQueue, &deviceData, pdMS_TO_TICKS(10));
                    Serial.println("OTA:ERROR@ da co phien ban moi nhat");
                }
            } 
            if(message.substring(4, 8) == "AUTO") { //"OTA:AUTO" 
                ota->setAutoUpdate(!(ota->getAutoUpdate()));
                String info = ota->Getinfo4mqtt();
                Serial.println(info.c_str());
                DeviceData deviceData;
                deviceData.VirtualPin = 0;
                deviceData.pin = 0;
                strncpy(deviceData.value, info.c_str(), MAX_VALUE_LEN - 1);
                deviceData.value[MAX_VALUE_LEN - 1] = '\0';
                deviceData.timestamp = millis();
                deviceData.isNC = true;
                xQueueSend(deviceDataQueue, &deviceData, pdMS_TO_TICKS(10));
            }
        }   
        
    }
}

// ======= OTA Task (Core 1) - Tạo động khi cần, tiết kiệm RAM =======
void otaTask(void* parameter) {
    Serial.println("🔄 [OTATask] Started (16KB stack, chỉ chạy 1 lần)");
    Serial.printf("📊 [OTATask] Free heap: %d bytes\n", ESP.getFreeHeap());
    
    // Thực hiện update (sau khi xong, ESP sẽ restart)
    ota->performUpdate(true);
    
    // Nếu update thất bại (không restart), xóa task để giải phóng RAM
    Serial.println("⚠️ [OTATask] Update failed or cancelled, cleaning up...");
    otaTaskHandle = NULL;  // Reset handle trước khi xóa
    vTaskDelete(NULL);     // Tự xóa task này
}
