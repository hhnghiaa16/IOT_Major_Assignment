#ifndef OTA_UPDATE_H
#define OTA_UPDATE_H
#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <Update.h>
#include <ArduinoJson.h>
#include "settings.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#define MAX_RETRIES 5
#define RETRY_DELAY_MS 6000 
/**
 * @brief Class quản lý OTA (Over-The-Air) update cho ESP32
 * 
 * Tính năng:
 * - Kiểm tra phiên bản firmware mới từ server
 * - Download và cập nhật firmware tự động
 * - Rollback nếu update thất bại
 * - Tích hợp với FreeRTOS
 * - Lưu trữ thông tin version vào NVS
 */
class OTAUpdate {
private:
    static OTAUpdate* instance;
    // Configuration
    String serverUrl;           // URL của OTA server
    String currentVersion;      // Phiên bản hiện tại
    String clientID;            // ID của thiết bị
    int checkInterval;          // Khoảng thời gian kiểm tra (milliseconds)
    bool autoUpdate;            // Tự động update
    bool isNewVersion;         // Có phiên bản mới không
    // Status
    bool isUpdating;            // Đang trong quá trình update
    int updateProgress;         // Tiến trình update (0-100%)
    String lastError;           // Lỗi gần nhất
    unsigned long lastCheck;    // Lần kiểm tra cuối cùng
    
    // FreeRTOS
    SemaphoreHandle_t mutex;    // Mutex để bảo vệ shared resources
    TaskHandle_t otaTaskHandle; // Task handle cho OTA monitoring
    
    // Callback functions
    typedef void (*OTAStartCallback)();
    typedef void (*OTAProgressCallback)(int progress);
    typedef void (*OTAEndCallback)(bool success);
    typedef void (*OTAErrorCallback)(const char* error);
    
    OTAStartCallback onStartCallback;
    OTAProgressCallback onProgressCallback;
    OTAEndCallback onEndCallback;
    OTAErrorCallback onErrorCallback;
    
    // Private constructor for Singleton
    OTAUpdate();
    void savemqttInfo(String brokerServer , int brokerPort , String wsURL , String clientID);
    /**
     * @brief Lấy thông tin firmware mới từ server
     * @param newVersion Output: Phiên bản mới
     * @param downloadUrl Output: URL download firmware
     * @param changelog Output: Changelog
     * @return true nếu có version mới, false nếu không
     */
    bool checkForUpdate(String& newVersion, String& downloadUrl);
    
    /**
     * @brief Download và cài đặt firmware mới
     * @param url URL của firmware binary
     * @return true nếu thành công, false nếu thất bại
     */
    bool downloadAndUpdate(const String& url);
    
    /**
     * @brief Callback khi có tiến trình download
     */
    static void updateProgressCallback(size_t current, size_t total);
    
    /**
     * @brief Task FreeRTOS để tự động kiểm tra update
     */
    static void otaMonitorTask(void* parameter);
    
    /**
     * @brief Lưu thông tin OTA vào NVS
     */
    void saveOTAInfo(const String& version, const String& updateTime);
    
    /**
     * @brief Đọc thông tin OTA từ NVS
     */
    void loadOTAInfo();

public:
    /**
     * @brief Lấy instance của OTAUpdate (Singleton pattern)
     */
    static OTAUpdate& getInstance();
    
    /**
     * @brief Khởi tạo OTA Update
     * @param serverUrl URL của OTA server (ví dụ: "http://192.168.1.100:8080/ota")
     * @param currentVersion Phiên bản hiện tại của firmware
     * @param deviceId ID của thiết bị
     * @param checkInterval Khoảng thời gian tự động kiểm tra (ms), mặc định 3600000 (1 giờ)
     * @return true nếu khởi tạo thành công
     */
    bool begin(const String& serverUrl, const String& currentVersion, 
               const String& deviceId, int checkInterval = 3600000);
    
    /**
     * @brief Dừng OTA service
     */
    void end();
    
    /**
     * @brief Kiểm tra và thực hiện update nếu có phiên bản mới
     * @param forceUpdate Bắt buộc update không cần hỏi
     * @return true nếu có update và thực hiện thành công
     */
    bool performUpdate(bool forceUpdate = false);
    
    /**
     * @brief Kiểm tra xem có phiên bản mới không
     * @return true nếu có phiên bản mới
     */
    bool hasNewVersion();
    
    /**
     * @brief Lấy phiên bản hiện tại
     */
    String getCurrentVersion() const { return currentVersion; }
    
    /**
     * @brief Lấy tiến trình update (0-100%)
     */
    int getProgress() const { return updateProgress; }
    
    /**
     * @brief Kiểm tra xem đang update không
     */
    bool isUpdateInProgress() const { return isUpdating; }
    
    /**
     * @brief Lấy lỗi gần nhất
     */
    String getLastError() const { return lastError; }
    
    /**
     * @brief Set callback khi bắt đầu update
     */
    void setOnStartCallback(OTAStartCallback callback) { 
        onStartCallback = callback; 
    }
    
    /**
     * @brief Set callback khi có tiến trình update
     */
    void setOnProgressCallback(OTAProgressCallback callback) { 
        onProgressCallback = callback; 
    }
    
    /**
     * @brief Set callback khi kết thúc update
     */
    void setOnEndCallback(OTAEndCallback callback) { 
        onEndCallback = callback; 
    }
    
    /**
     * @brief Set callback khi có lỗi
     */
    void setOnErrorCallback(OTAErrorCallback callback) { 
        onErrorCallback = callback; 
    }
    /**
     * @brief Set tự động update
     */
    void setAutoUpdate(bool autoUpdate);
    /**
     * @brief Lấy tự động update
     */
    bool getAutoUpdate();
    /**
     * @brief Bật/tắt tự động kiểm tra update
     */
    void enableAutoCheck(bool enable);
    
    /**
     * @brief In thông tin OTA
     */
    void printInfo();
    /**
     * @brief Lấy thông tin OTA cho MQTT
     */
    String Getinfo4mqtt();
    
    void loadsettingInNVS();
    
    void saveSettingInNVS();
    // Destructor
    ~OTAUpdate();
};

#endif // OTA_UPDATE_H

