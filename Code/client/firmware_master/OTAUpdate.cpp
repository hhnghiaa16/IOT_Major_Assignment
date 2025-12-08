#include "OTAUpdate.h"

// Static instance
OTAUpdate* OTAUpdate::instance = nullptr;

// Constructor
OTAUpdate::OTAUpdate() {
    serverUrl = "";
    currentVersion = "0.0.0";
    clientID = "";
    checkInterval = 3600000; // 1 hour
    isUpdating = false;
    autoUpdate = false;
    updateProgress = 0;
    lastError = "";
    lastCheck = 0;
    otaTaskHandle = NULL;
    
    // Create mutex
    mutex = xSemaphoreCreateMutex();
    
    // Callbacks
    onStartCallback = nullptr;
    onProgressCallback = nullptr;
    onEndCallback = nullptr;
    onErrorCallback = nullptr;
}

// Destructor
OTAUpdate::~OTAUpdate() {
    end();
    if (mutex != NULL) {
        vSemaphoreDelete(mutex);
    }
}

// Get singleton instance
OTAUpdate& OTAUpdate::getInstance() {
    if (instance == nullptr) {
        instance = new OTAUpdate();
    }
    return *instance;
}

// Begin OTA service
bool OTAUpdate::begin(const String& serverUrl, const String& currentVersion, 
                      const String& clientID, int checkInterval) {
    if (xSemaphoreTake(mutex, portMAX_DELAY) != pdTRUE) {
        return false;
    }
    
    this->serverUrl = serverUrl;
    this->currentVersion = currentVersion;
    this->clientID = clientID;
    this->checkInterval = checkInterval;
    
    Serial.println("🔄 [OTA] Initializing OTA Update Service...");
    Serial.printf("   📌 Server URL: %s\n", serverUrl.c_str());
    Serial.printf("   📌 Current Version: %s\n", currentVersion.c_str());
    Serial.printf("   📌 Device ID: %s\n", clientID.c_str());
    loadsettingInNVS();
    // Load OTA info from NVS
    loadOTAInfo();
    
    // Create OTA monitoring task
    // xTaskCreatePinnedToCore(
    //     otaMonitorTask,
    //     "OTATask",
    //     8192,  // Larger stack size for HTTP operations
    //     this,
    //     1,     // Low priority
    //     &otaTaskHandle,
    //     1      // Core 1
    // );
    
    xSemaphoreGive(mutex);
    
    Serial.println("✅ [OTA] OTA Update Service initialized successfully!");
    return true;
}
void OTAUpdate::loadsettingInNVS() {
    Settings settings("ota", false);
    autoUpdate = settings.getBool("auto_update", false);

}
void OTAUpdate::saveSettingInNVS() {
    Settings settings("ota", true);
    settings.setBool("auto_update", autoUpdate);
}
// End OTA service
void OTAUpdate::end() {
    if (otaTaskHandle != NULL) {
        vTaskDelete(otaTaskHandle);
        otaTaskHandle = NULL;
    }
}

// Check for update from server
bool OTAUpdate::checkForUpdate(String& newVersion, String& downloadUrl) {
     if (WiFi.status() != WL_CONNECTED) {
        lastError = "WiFi not connected";
        return false;
    }

    HTTPClient http;
    String url = serverUrl;

    Serial.printf("🔍 [OTA] Checking for updates: %s\n", url.c_str());

    // Cấu hình retry// 6 giây
    int httpCode = 0;
    
    // Thử lại tối đa MAX_RETRIES lần
    for (int attempt = 1; attempt <= MAX_RETRIES; attempt++) {
        Serial.printf("🔄 [OTA] Attempt %d/%d\n", attempt, MAX_RETRIES);
        
        // Bắt đầu HTTP
        http.begin(url);
        http.addHeader("Content-Type", "application/json");

        // Thêm header Authorization: Bearer {client_id}
        // Lưu ý: ở đây dùng member clientId; nếu bạn không có, truyền làm tham số thay vào.
        String authHeader = "Bearer " + clientID;
        http.addHeader("Authorization", authHeader);

        httpCode = http.GET();

        if (httpCode == HTTP_CODE_OK) {
            // Request thành công, thoát khỏi vòng lặp retry
            break;
        } else {
            // Request thất bại
            lastError = "HTTP error: " + String(httpCode);
            Serial.printf("❌ [OTA] %s (Attempt %d/%d)\n", lastError.c_str(), attempt, MAX_RETRIES);
            http.end();
            
            // Nếu chưa hết số lần thử, chờ và thử lại
            if (attempt < MAX_RETRIES) {
                Serial.printf("⏳ [OTA] Waiting %d seconds before retry...\n", RETRY_DELAY_MS / 1000);
                delay(RETRY_DELAY_MS);
            }
        }
    }

    // Kiểm tra kết quả sau khi retry
    if (httpCode == HTTP_CODE_OK) {
        String payload = http.getString();
        Serial.printf("📦 [OTA] Server response: %s\n", payload.c_str());

        // Parse JSON response - tăng kích thước nếu cần (tùy dữ liệu trả về)
        const size_t capacity = 6 * 256; // chỉnh nếu cần
        DynamicJsonDocument doc(capacity);
        DeserializationError error = deserializeJson(doc, payload);

        if (error) {
            lastError = "JSON parse error: " + String(error.c_str());
            Serial.printf("❌ [OTA] %s\n", lastError.c_str());
            http.end();
            return false;
        }

        // Kiểm tra trường success
        bool success = doc["success"] | false;

        if (!success) {
            lastError = "Server reported success=false";
            Serial.printf("❌ [OTA] %s\n", lastError.c_str());
            http.end();
            return false;
        }

        // Lấy các trường từ JSON (theo mẫu bạn cung cấp)
        String brokerServer = doc["broker_server"] | "";
        int    brokerPort   = doc["broker_port"] | 0;
        String wsURL = doc["ws_url"] | "";

        String masterLink    = doc["master_link"] | "";
        String masterVersion = doc["master_version"] | "";

        String slaveLink     = doc["slave_link"] | "";
        String slaveVersion  = doc["slave_version"] | "";

        savemqttInfo(brokerServer , brokerPort , wsURL, clientID);
         // In log chi tiết
        Serial.println("🎉 [OTA] Update info received:");
        Serial.printf("   📌 Broker server: %s\n", brokerServer.c_str());
        Serial.printf("   📌 Broker port: %d\n", brokerPort);
        Serial.printf("   📌 wsURL: %s\n", wsURL.c_str());
        Serial.printf("   📌 Master version: %s\n", masterVersion.c_str());
        Serial.printf("   📌 Master link: %s\n", masterLink.c_str());
        Serial.printf("   📌 Slave version: %s\n", slaveVersion.c_str());
        Serial.printf("   📌 Slave link: %s\n", slaveLink.c_str());
        // Chọn dùng master làm nguồn update chính (bạn có thể đổi logic nếu muốn)
        bool isMasterDevice = currentVersion.indexOf("Master") >= 0 || 
                      currentVersion.indexOf("master") >= 0;

        if (isMasterDevice) {
            // Device này là Master
            if (masterVersion.length() > 0 && masterLink.length() > 0 && masterVersion > currentVersion) {
                newVersion = masterVersion;
                downloadUrl = masterLink;
                Serial.println("📌 [OTA] Using MASTER firmware");
            } else {
                isNewVersion = false;
                lastError = "Master firmware info is empty";
                Serial.printf("❌ [OTA] %s\n", lastError.c_str());
                http.end();
                return false;
            }
        } else {
            // Device này là Slave
            if (slaveVersion.length() > 0 && slaveLink.length() > 0 && slaveVersion > currentVersion) {
                newVersion = slaveVersion;
                downloadUrl = slaveLink;
                Serial.println("📌 [OTA] Using SLAVE firmware");
            } else {
                isNewVersion = false;
                lastError = "Slave firmware info is empty";
                Serial.printf("❌ [OTA] %s\n", lastError.c_str());
                http.end();
                return false;
            }
        }
        if (newVersion == currentVersion) {
            Serial.println("✅ [OTA] Already running latest version");
            isNewVersion = false;
            http.end();
            return false;
        }
        isNewVersion = true;
        http.end();
        return true;
    } else {
        // Sau MAX_RETRIES lần vẫn thất bại
        lastError = "HTTP error after " + String(MAX_RETRIES) + " attempts: " + String(httpCode);
        Serial.printf("❌ [OTA] %s\n", lastError.c_str());
        http.end();
        return false;
    }
}
void OTAUpdate::savemqttInfo(String brokerServer , int brokerPort ,String wsURL, String clientID) {
    Settings settings("mqtt", true);

    settings.setString("broker", brokerServer);
    settings.setInt("port", brokerPort);
    settings.setString("clientId", clientID);
    settings.setString("url", wsURL);
}
// Download and update firmware
bool OTAUpdate::downloadAndUpdate(const String& url) {
    if (WiFi.status() != WL_CONNECTED) {
        lastError = "WiFi not connected";
        if (onErrorCallback) onErrorCallback(lastError.c_str());
        return false;
    }
    
    HTTPClient http;
    http.begin(url);
    
    Serial.printf("📥 [OTA] Starting firmware download from: %s\n", url.c_str());
    
    int httpCode = http.GET();
    
    if (httpCode == HTTP_CODE_OK) {
        int contentLength = http.getSize();
        
        if (contentLength <= 0) {
            lastError = "Invalid content length";
            Serial.printf("❌ [OTA] %s\n", lastError.c_str());
            if (onErrorCallback) onErrorCallback(lastError.c_str());
            http.end();
            return false;
        }
        
        Serial.printf("📦 [OTA] Firmware size: %d bytes\n", contentLength);
        
        bool canBegin = Update.begin(contentLength);
        
        if (!canBegin) {
            lastError = "Not enough space for OTA";
            Serial.printf("❌ [OTA] %s\n", lastError.c_str());
            if (onErrorCallback) onErrorCallback(lastError.c_str());
            http.end();
            return false;
        }
        
        // Callback when start
        if (onStartCallback) onStartCallback();
        
        WiFiClient* stream = http.getStreamPtr();
        
        size_t written = 0;
        uint8_t buff[128] = { 0 };
        
        Serial.println("🔄 [OTA] Writing firmware...");
        
        while (http.connected() && (written < contentLength)) {
            size_t available = stream->available();
            
            if (available) {
                int c = stream->readBytes(buff, min(available, sizeof(buff)));
                
                if (c > 0) {
                    written += Update.write(buff, c);
                    
                    // Update progress
                    updateProgress = (written * 100) / contentLength;
                    
                    // Callback progress
                    if (onProgressCallback) {
                        onProgressCallback(updateProgress);
                    }
                    
                    // Print progress every 10%
                    static int lastPrintedProgress = 0;
                    if (updateProgress - lastPrintedProgress >= 10) {
                        Serial.printf("📊 [OTA] Progress: %d%%\n", updateProgress);
                        lastPrintedProgress = updateProgress;
                    }
                }
            }
            
            delay(1);
        }
        
        Serial.printf("✅ [OTA] Written %d bytes\n", written);
        
        if (written == contentLength) {
            Serial.println("✅ [OTA] All data written");
        } else {
            lastError = "Written bytes mismatch";
            Serial.printf("❌ [OTA] %s: written=%d, expected=%d\n", 
                         lastError.c_str(), written, contentLength);
            Update.abort();
            if (onErrorCallback) onErrorCallback(lastError.c_str());
            http.end();
            return false;
        }
        
        if (Update.end()) {
            if (Update.isFinished()) {
                Serial.println("🎉 [OTA] Update successfully completed!");
                
                // Save OTA info
                saveOTAInfo(currentVersion, String(millis() / 1000));
                
                // Callback when end
                if (onEndCallback) onEndCallback(true);
                
                Serial.println("🔄 [OTA] Rebooting in 3 seconds...");
                delay(3000);
                ESP.restart();
                
                return true;
            } else {
                lastError = "Update not finished";
                Serial.printf("❌ [OTA] %s\n", lastError.c_str());
                if (onErrorCallback) onErrorCallback(lastError.c_str());
                if (onEndCallback) onEndCallback(false);
            }
        } else {
            lastError = "Update error: " + String(Update.errorString());
            Serial.printf("❌ [OTA] %s\n", lastError.c_str());
            if (onErrorCallback) onErrorCallback(lastError.c_str());
            if (onEndCallback) onEndCallback(false);
        }
    } else {
        lastError = "HTTP error: " + String(httpCode);
        Serial.printf("❌ [OTA] %s\n", lastError.c_str());
        if (onErrorCallback) onErrorCallback(lastError.c_str());
    }
    
    http.end();
    return false;
}

// Perform update
bool OTAUpdate::performUpdate(bool forceUpdate ) {
    if (xSemaphoreTake(mutex, portMAX_DELAY) != pdTRUE) {
        return false;
    }
    
    if (isUpdating) {
        Serial.println("⚠️ [OTA] Update already in progress");
        xSemaphoreGive(mutex);
        return false;
    }
    
    isUpdating = true;
    updateProgress = 0;
    lastError = "";
    
    xSemaphoreGive(mutex);
    
    String newVersion, downloadUrl;
    
    if (checkForUpdate(newVersion, downloadUrl)) {
        if (forceUpdate) {
            Serial.println("🚀 [OTA] Force update initiated...");
            bool result = downloadAndUpdate(downloadUrl);
            
            if (xSemaphoreTake(mutex, portMAX_DELAY) == pdTRUE) {
                isUpdating = false;
                xSemaphoreGive(mutex);
            }
            
            return result;
        } else {
            Serial.println("ℹ️ [OTA] New version available but auto-update disabled");
            Serial.println("   Call performUpdate(true) to force update");
            
            if (xSemaphoreTake(mutex, portMAX_DELAY) == pdTRUE) {
                isUpdating = false;
                xSemaphoreGive(mutex);
            }
            
            return false;
        }
    }
    
    if (xSemaphoreTake(mutex, portMAX_DELAY) == pdTRUE) {
        isUpdating = false;
        xSemaphoreGive(mutex);
    }
    
    return false;
}

// Check if has new version
bool OTAUpdate::hasNewVersion() {
    String newVersion, downloadUrl;
    return checkForUpdate(newVersion, downloadUrl);
}

// OTA monitor task
void OTAUpdate::otaMonitorTask(void* parameter) {
    OTAUpdate* ota = (OTAUpdate*)parameter;
    
    Serial.println("🔄 [OTA] OTA Monitor Task started");
    
    while (true) {
        unsigned long currentTime = millis();
        
        // Check if it's time to check for updates
        if (currentTime - ota->lastCheck >= ota->checkInterval) {
            ota->lastCheck = currentTime;
            
            Serial.println("⏰ [OTA] Periodic update check...");
            
            bool shouldAutoUpdate = false;
            if (xSemaphoreTake(ota->mutex, portMAX_DELAY) == pdTRUE) {
                shouldAutoUpdate = ota->autoUpdate;
                xSemaphoreGive(ota->mutex);
            }
            
            if (shouldAutoUpdate) {
                // Tự động update nếu bật autoUpdate
                Serial.println("🔄 [OTA] Auto-update is enabled, performing update...");
                ota->performUpdate(true);
            } else {
                // Chỉ check, không update
                ota->hasNewVersion();
            }
        }
        
        // Sleep for 60 seconds
        vTaskDelay(pdMS_TO_TICKS(60000));
    }
}

// Save OTA info to NVS
void OTAUpdate::saveOTAInfo(const String& version, const String& updateTime) {
    Settings otaSettings("ota", true);
    otaSettings.setString("last_version", version);
    otaSettings.setString("last_update", updateTime);
    Serial.println("💾 [OTA] Saved OTA info to NVS");
}

// Load OTA info from NVS
void OTAUpdate::loadOTAInfo() {
    Settings otaSettings("ota", true);
    String lastVersion = otaSettings.getString("last_version", "unknown");
    String lastUpdate = otaSettings.getString("last_update", "unknown");
    autoUpdate = otaSettings.getBool("auto_update", false);

    Serial.println("📖 [OTA] Loaded OTA info from NVS:");
    Serial.printf("   Last Version: %s\n", lastVersion.c_str());
    Serial.printf("   Last Update: %s seconds ago\n", lastUpdate.c_str());
    Serial.printf("   Auto Update: %s\n", autoUpdate ? "true" : "false");
}
void OTAUpdate::setAutoUpdate(bool autoUpdate) {

    if (xSemaphoreTake(mutex, portMAX_DELAY) == pdTRUE) {
        this->autoUpdate = autoUpdate;
        
        // Lưu vào NVS
        Settings otaSettings("ota", true);
        otaSettings.setBool("auto_update", autoUpdate);
        Serial.printf("   Auto Update: %s\n", autoUpdate ? "Yes" : "No");
        
        xSemaphoreGive(mutex);
    }
}

bool OTAUpdate::getAutoUpdate() {
    if (xSemaphoreTake(mutex, portMAX_DELAY) == pdTRUE) {
        bool result = autoUpdate;
        xSemaphoreGive(mutex);
        return result;
    }
    return false;
}
// Enable/disable auto check
// Xem xét để xóa đi nếu không cần thiết
void OTAUpdate::enableAutoCheck(bool enable) {
    if (enable) {
        if (otaTaskHandle == NULL) {
            xTaskCreatePinnedToCore(
                otaMonitorTask,
                "OTATask",
                8192,
                this,
                1,
                &otaTaskHandle,
                1
            );
            Serial.println("✅ [OTA] Auto-check enabled");
        }
    } else {
        if (otaTaskHandle != NULL) {
            vTaskDelete(otaTaskHandle);
            otaTaskHandle = NULL;
            Serial.println("⏸️ [OTA] Auto-check disabled");
        }
    }
}
String OTAUpdate::Getinfo4mqtt(){
    String info = "";
    bool updating = false;
    int progress = 0;
    bool autoUpdateEnabled = false;
    
    // Lock 1 lần, đọc hết shared variables
    if (xSemaphoreTake(mutex, portMAX_DELAY) == pdTRUE) {
        updating = isUpdating;
        progress = updateProgress;
        autoUpdateEnabled = autoUpdate;
        xSemaphoreGive(mutex);  // ← Release ngay
    }
    
    // Early return nếu đang update
    if (updating) {
        return "OTA:UPDATING@" + String(progress);
    }
    
    // Lấy thông tin từ NVS
    Settings otaSettings("ota", true);
    String lastVersion = otaSettings.getString("last_version", "unknown");
    String lastUpdate = otaSettings.getString("last_update", "unknown");
    hasNewVersion();
    // Build result
    return "OTA:INFO@" + lastVersion + "@" + lastUpdate + "@" + 
           String(autoUpdateEnabled ? 1 : 0) + "@" + String(isNewVersion ? 1 : 0);
}
// Print OTA info
void OTAUpdate::printInfo() {
    Serial.println("\n╔════════════════════════════════════════╗");
    Serial.println("║         OTA UPDATE INFORMATION         ║");
    Serial.println("╠════════════════════════════════════════╣");
    Serial.printf("║ Current Version:  %-20s ║\n", currentVersion.c_str());
    Serial.printf("║ Device ID:        %-20s ║\n", clientID.c_str());
    Serial.printf("║ Server URL:       %-20s ║\n", serverUrl.c_str());
    Serial.printf("║ Check Interval:   %-17d ms ║\n", checkInterval);
    Serial.printf("║ Is Updating:      %-20s ║\n", isUpdating ? "Yes" : "No");
    Serial.printf("║ Progress:         %-17d %% ║\n", updateProgress);
    Serial.printf("║ Last Error:       %-20s ║\n", 
                  lastError.length() > 0 ? lastError.c_str() : "None");
    Serial.printf("║ Free Heap:        %-17d KB ║\n", ESP.getFreeHeap() / 1024);
    Serial.println("╚════════════════════════════════════════╝\n");
}

