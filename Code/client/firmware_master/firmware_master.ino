#include "wifiStation.h"
#include "settings.h"
#include "mqtt.h"
// #include "gpioManager.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>
#include "OTAUpdate.h"
#include "MicRecorder.h"
#include "AudioPlayer.h"
// #include "DHT.h"
#define CLIENT_ID "2c80d03e31ff68f4d1b0a2300f113a2e"
#define version  "Master_1.0.2"
#define OTA_SERVER_URL "http://10.1.0.32:8000/ota/get_info_update"
// ======= Global References =======
WiFiStation* wifi;
MQTTProtocol* mqtt;
// GPIOManager* gpio;
OTAUpdate* ota;
MicRecorder* mic;           // Microphone recorder pointer
AudioPlayer* audioPlayer;   // Audio player pointer
volatile bool isProcessingVoice = false;  // Flag: đang xử lý voice (từ AU:OFF đến audio xong)
volatile unsigned long processingVoiceStartTime = 0;  // Thời điểm bắt đầu xử lý voice
const unsigned long VOICE_PROCESSING_TIMEOUT = 12000;  // Timeout 60 giây
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
TaskHandle_t otaTaskHandle = NULL;   // Task cho OTA update (động)
TaskHandle_t micTaskHandle = NULL;   // Task cho microphone recording
TaskHandle_t audioTaskHandle = NULL; // Task cho audio playback (động)

// ======= Task Functions =======
void wifiTask(void* parameter);
void mqttTask(void* parameter);
void gpioTask(void* parameter);
void ReadDataTask(void* parameter);
void otaTask(void* parameter);
void micTask(void* parameter);  // Task xử lý microphone recording
void audioPlaybackTask(void* parameter);  // Task phát audio (tự hủy sau khi xong)

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
    // gpio = &GPIOManager::getInstance();
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
    // gpio->begin();
    // gpio->setOutputPin(4, true);
    // gpio->saveGPIOConfig(); // luu cau hinh GPIO vao NVS
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
    
    // xTaskCreatePinnedToCore(
    //     gpioTask,           // Task function
    //     "GPIOTask",         // Task name
    //     4096,               // Stack size
    //     NULL,               // Parameters
    //     2,                  // Priority
    //     &gpioTaskHandle,    // Task handle
    //     0                   // Core 0
    // );
    
    // xTaskCreatePinnedToCore(
    //     ReadDataTask,         // Task function
    //     "SensorTask",       // Task name
    //     4096,               // Stack size
    //     NULL,               // Parameters
    //     1,                  // Priority
    //     &readTaskHandle,  // Task handle
    //     1                   // Core 1
    // );
    
    // OTA Task - KHÔNG tạo ở đây, sẽ tạo động khi cần để tiết kiệm 16KB RAM
    // otaTaskHandle sẽ được tạo trong mqttCallback khi nhận "OTA:UP"
    
    // ======= Initialize MicRecorder =======
    mic = &MicRecorder::getInstance();
    mic->begin();
    // WebSocket URL sẽ được set từ OTA response (đã lưu trong NVS)
    
    // ======= Initialize AudioPlayer =======
    // audioPlayer = &AudioPlayer::getInstance();
    // audioPlayer->begin();  // Default pins: BCLK=26, LRC=25, DOUT=22
    Serial.println("✅ [AudioPlayer] Initialized successfully");
    
    // ======= Create Mic Task - xử lý recording và WebSocket streaming =======
    xTaskCreatePinnedToCore(
        micTask,            // Task function
        "MicTask",          // Task name
        8192,               // Stack size 8KB cho WebSocket
        NULL,               // Parameters
        2,                  // Priority
        &micTaskHandle,     // Task handle
        1                   // Core 1
    );
    Serial.println("✅ [MicRecorder] Mic task created successfully");

    
    Serial.println("✅ All tasks created successfully!");
    Serial.printf("📊 Free heap after setup: %d bytes\n", ESP.getFreeHeap());
    
    // Subscribe to MQTT topics
    // Mutex đã được xử lý bên trong mqtt->subscribe()
    // phải đăng kí các topic trước khi gửi nhận mới được .
    // mqtt->registerVirtualpin(REG_SS, 4);
    // mqtt->registerVirtualpin(REG_CT, 5);
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
// void gpioTask(void* parameter) {
//     Serial.println("🔌 [GPIOTask] Started on Core 0");
    
//     while (true) {
//         // gpio->loop();
        
//         // Process command queue
//         CommandData commandData;
//         if (xQueueReceive(commandQueue, &commandData, 0) == pdTRUE) {
//             if(strncmp(commandData.Message, "ER", 2) == 0) {
//                 Serial.printf("📤 [ERROR] SERVER ERROR: %s ", commandData.Message);
//                 continue;
//             }
//             // cho virtual pin bằng với pin thật 
//             gpio->processCommand(commandData.VirtualPin, String(commandData.Message), 1);
//         }
        
//         vTaskDelay(pdMS_TO_TICKS(100)); // 100ms
//     }
// }

// ======= Sensor Task (Core 1) =======
// void ReadDataTask(void* parameter) {
//     Serial.println("🌡️ [SensorTask] Started on Core 1");
//     float t ;
//     float h ;
//     while (true) {
//         //  h = dht.readHumidity();
//         //  t = dht.readTemperature();
//         //222222
//         // Read sensors and send data
//         // DeviceData deviceData;
        
//         // // Read digital pins
//         // deviceData.pin = 5;
//         // deviceData.VirtualPin = 5;
//         // deviceData.value = String(t);
//         // deviceData.timestamp = millis();
//         // xQueueSend(deviceDataQueue, &deviceData, pdMS_TO_TICKS(10));
        
//         // sensorData.sensorName = "pin_4";
//         // sensorData.value = String(gpio->readDigital(4));
//         // sensorData.timestamp = millis();
//         // xQueueSend(deviceDataQueue, &deviceData, pdMS_TO_TICKS(10));
        
//         // // Read analog pins
//         // sensorData.sensorName = "pin_36_analog";
//         // sensorData.value = String(gpio->readAnalog(36));
//         // sensorData.timestamp = millis();
//         // xQueueSend(deviceDataQueue, &deviceData, pdMS_TO_TICKS(10));
        
//         // // System info
//         // sensorData.sensorName = "system_uptime";
//         // sensorData.value = String(millis() / 1000);
//         // sensorData.timestamp = millis();
//         // xQueueSend(deviceDataQueue, &deviceData, pdMS_TO_TICKS(10));
        
//         // sensorData.sensorName = "system_free_heap";
//         // sensorData.value = String(ESP.getFreeHeap());
//         // sensorData.timestamp = millis();
//         // xQueueSend(deviceDataQueue, &deviceData, pdMS_TO_TICKS(10));
        
//         vTaskDelay(pdMS_TO_TICKS(8000)); // 5 seconds
//     }
// }

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
        if(message.substring(0, 3) == "WAV") { 
            if(message.substring(4, 6) == "RD") { //"WAV:RD" - Audio ready to play
                Serial.println("🔊 [AUDIO] Received WAV:RD - Creating audio playback task...");
                
                // Tạo Audio Playback Task ĐỘNG (tự hủy sau khi xong)
                if (audioTaskHandle == NULL) {
                    xTaskCreatePinnedToCore(
                        audioPlaybackTask,
                        "AudioTask",
                        8192,           // 8KB stack cho HTTP + audio streaming
                        NULL,
                        2,              // Priority
                        &audioTaskHandle,
                        1               // Core 1
                    );
                    Serial.println("🔊 [AUDIO] Created audio playback task");
                } else {
                    Serial.println("⚠️ [AUDIO] Audio task already running!");
                }
            }
        }
    }
}

// ======= OTA Task (Core 1) - Tạo động khi cần, tiết kiệm RAM =======
void otaTask(void* parameter) {
    Serial.println("🔄 [OTATask] Started (16KB stack, chỉ chạy 1 lần)");
    Serial.printf("📊 [OTATask] Free heap: %d bytes\n", ESP.getFreeHeap());
    
    // // Kiểm tra xem có đang ghi âm không
    // if (isDeviceRecording) {
    //     Serial.println("⚠️ [OTATask] Cannot update while recording! Aborting...");
    //     otaTaskHandle = NULL;
    //     vTaskDelete(NULL);
    //     return;
    // }
    
    // Thực hiện update (sau khi xong, ESP sẽ restart)
    ota->performUpdate(true);
    
    // Nếu update thất bại (không restart), xóa task để giải phóng RAM
    Serial.println("⚠️ [OTATask] Update failed or cancelled, cleaning up...");
    otaTaskHandle = NULL;  // Reset handle trước khi xóa
    vTaskDelete(NULL);     // Tự xóa task này
}

// ======= Mic Task (Core 1) - Xử lý recording và WebSocket streaming =======
void micTask(void* parameter) {
    Serial.println("🎤 [MicTask] Started on Core 1");
    
    bool lastButtonState = BUTTON_ACTIVE_LOW ? HIGH : LOW;
    bool currentButtonState;
    unsigned long lastDebounceTime = 0;
    const unsigned long debounceDelay = 50;  // 50ms debounce
    
    while (true) {
        // Đọc trạng thái nút bấm
        currentButtonState = digitalRead(RECORD_BUTTON_PIN);
        // Serial.printf("Button GPIO33 = %s\n", currentButtonState ? "HIGH" : "LOW");
        // Debounce
        if (currentButtonState != lastButtonState) {
            lastDebounceTime = millis();
        }
        
        if ((millis() - lastDebounceTime) > debounceDelay) {
            // Button đã ổn định
            bool buttonPressed = BUTTON_ACTIVE_LOW ? (currentButtonState == LOW) : (currentButtonState == HIGH);
            
            if (buttonPressed && !mic->isRecording()) {
                // Kiểm tra nếu đang phát audio thì không cho ghi âm
                if (audioTaskHandle != NULL) {
                    Serial.println("⚠️ [MicTask] Cannot record while audio is playing!");
                    vTaskDelay(pdMS_TO_TICKS(100));  // Chờ một chút
                    lastButtonState = currentButtonState;
                    continue;  // Bỏ qua, không ghi âm
                }
                
                // Kiểm tra nếu đang xử lý voice (STT + TTS) thì không cho ghi âm
                if (isProcessingVoice) {
                    // Check timeout - nếu quá 60 giây thì auto-reset
                    if (millis() - processingVoiceStartTime > VOICE_PROCESSING_TIMEOUT) {
                        Serial.println("⚠️ [MicTask] Voice processing timeout! Auto-resetting...");
                        isProcessingVoice = false;
                    } else {
                        Serial.println("⚠️ [MicTask] Cannot record while processing voice!");
                        vTaskDelay(pdMS_TO_TICKS(100));
                        lastButtonState = currentButtonState;
                        continue;
                    }
                }
                
                // Bắt đầu ghi âm
                Serial.println("🎤 [MicTask] Button pressed - Starting recording...");
                
                // Kiểm tra có WebSocket URL không
                String wsUrl = mic->getWebSocketUrl();
                if (wsUrl.length() == 0) {
                    // Nếu chưa set, dùng URL mặc định (có thể lấy từ OTA response)
                    wsUrl = "ws://192.168.3.102:8000/audio_stream/ws";
                    mic->setWebSocketUrl(wsUrl);
                }
                
                // Bắt đầu recording với client ID - gửi MQTT notify
                DeviceData deviceData;
                deviceData.VirtualPin = 0;
                deviceData.pin = 0;
                strncpy(deviceData.value, "AU:ON", MAX_VALUE_LEN - 1);
                deviceData.value[MAX_VALUE_LEN - 1] = '\0';
                deviceData.timestamp = millis();
                deviceData.isNC = true;
                xQueueSendToFront(deviceDataQueue, &deviceData, pdMS_TO_TICKS(100));
                // mqtt->send(0, "AU:ON", false, true);
                mic->startRecording(wsUrl, CLIENT_ID);
            }
            else if (!buttonPressed && mic->isRecording()) {
                // Kết thúc ghi âm
                Serial.println("🎤 [MicTask] Button released - Stopping recording...");
                DeviceData deviceData;
                deviceData.VirtualPin = 0;
                deviceData.pin = 0;
                strncpy(deviceData.value, "AU:OFF", MAX_VALUE_LEN - 1);
                deviceData.value[MAX_VALUE_LEN - 1] = '\0';
                deviceData.timestamp = millis();
                deviceData.isNC = true;
                xQueueSendToFront(deviceDataQueue, &deviceData, pdMS_TO_TICKS(100));
                mic->stopRecording();
                isProcessingVoice = true;  // Bắt đầu xử lý voice (STT + TTS)
                processingVoiceStartTime = millis();  // Ghi lại thời điểm bắt đầu
            }
        }
        
        lastButtonState = currentButtonState;
        
        // Update mic (đọc I2S và gửi WebSocket)
        mic->update();
        
        // Note: Audio playback is handled in separate audioPlaybackTask
        // to avoid conflicts with mic recording
        
        vTaskDelay(pdMS_TO_TICKS(5));  // 5ms - cần nhanh để không bỏ lỡ audio data
    }
}

// ======= Audio Playback Task (Core 1) - Tạo động, tự hủy sau khi xong =======
void audioPlaybackTask(void* parameter) {
    Serial.println("🔊 [AudioTask] Started");
    Serial.printf("📊 [AudioTask] Free heap: %d bytes\n", ESP.getFreeHeap());
    
    // Step 1: Fetch audio URL from server
   HTTPClient http;
    String audioApiUrl = String("http://10.1.0.32:8000/audio_stream/get-audio-url?client_id=") + CLIENT_ID;

    // tạo URL có params (encode nếu cần)
    // String url = audioApiUrl + "?client_id=" + CLIENT_ID;
    String audioUrl = "";
    Serial.printf("🔊 [AudioTask] Fetching URL from: %s\n", audioApiUrl.c_str());
    
    http.begin(audioApiUrl);
    // http.addHeader("client_id", CLIENT_ID);  // Truyền client_id trong header
    int httpCode = http.GET();
    
    if (httpCode == HTTP_CODE_OK) {
        String payload = http.getString();
        Serial.printf("📦 [AudioTask] Response: %s\n", payload.c_str());
        
        // Parse JSON response
        DynamicJsonDocument doc(512);
        DeserializationError error = deserializeJson(doc, payload);
        
        if (!error) {
            bool success = doc["success"] | false;
            
            if (success) {
                audioUrl = doc["audio_url"].as<String>();
            } else {
                String errMsg = doc["message"] | "Unknown error";
                Serial.printf("❌ [AudioTask] Server error: %s\n", errMsg.c_str());
            }
        } else {
            Serial.printf("❌ [AudioTask] JSON parse error: %s\n", error.c_str());
        }
    } else {
        Serial.printf("❌ [AudioTask] HTTP error: %d\n", httpCode);
    }
    
    http.end();
    
    // Step 2: Play audio if URL is valid
    if (audioUrl.length() > 0) {
        Serial.printf("🔊 [AudioTask] Playing: %s\n", audioUrl.c_str());
        
        // Initialize AudioPlayer if not already
        if (audioPlayer == nullptr) {
            audioPlayer = &AudioPlayer::getInstance();
            audioPlayer->begin();
        }
        audioPlayer->setVolume(0.5);  // Giảm xuống 2% vì MAX98357A có gain 9dB cố định
        // Start playing
        audioPlayer->play(audioUrl);
        
        // Loop until audio finishes
        while (audioPlayer->isPlaying()) {
            audioPlayer->update();
            vTaskDelay(pdMS_TO_TICKS(10));  // 10ms - fast enough for audio streaming
        }
        
        Serial.println("✅ [AudioTask] Audio playback completed");
    } else {
        Serial.println("❌ [AudioTask] No valid audio URL, skipping playback");
    }
    
    // Step 3: Cleanup and self-destruct
    Serial.println("🧹 [AudioTask] Cleaning up and exiting...");
    isProcessingVoice = false;  // Cho phép ghi âm lại
    audioTaskHandle = NULL;  // Reset handle
    vTaskDelete(NULL);       // Self-destruct
}
