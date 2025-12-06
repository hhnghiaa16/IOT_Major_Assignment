# 🔄 ESP32 OTA Update Class - Hướng dẫn sử dụng

## 📋 Mục lục / Table of Contents

1. [Giới thiệu / Introduction](#giới-thiệu)
2. [Tính năng / Features](#tính-năng)
3. [Yêu cầu / Requirements](#yêu-cầu)
4. [Cài đặt / Installation](#cài-đặt)
5. [Cách sử dụng / Usage](#cách-sử-dụng)
6. [OTA Server Setup](#ota-server-setup)
7. [API Reference](#api-reference)
8. [Examples](#examples)
9. [Troubleshooting](#troubleshooting)

---

## 🎯 Giới thiệu / Introduction

**OTAUpdate** là một class C++ được thiết kế để quản lý việc cập nhật firmware từ xa (Over-The-Air) cho ESP32. Class này hỗ trợ:

- ✅ Kiểm tra phiên bản firmware mới từ server
- ✅ Download và cập nhật firmware tự động
- ✅ Tích hợp với FreeRTOS
- ✅ Callback functions để theo dõi tiến trình
- ✅ Lưu trữ thông tin version vào NVS
- ✅ Tự động kiểm tra update theo chu kỳ
- ✅ MQTT integration để remote control

---

## ⚡ Tính năng / Features

### Core Features
- **Version Management**: Tự động kiểm tra và so sánh phiên bản firmware
- **Secure Download**: Download firmware an toàn qua HTTP/HTTPS
- **Progress Tracking**: Theo dõi tiến trình update real-time (0-100%)
- **Error Handling**: Xử lý lỗi chi tiết với error messages
- **Rollback Support**: Tự động rollback nếu update thất bại
- **NVS Storage**: Lưu thông tin update vào Non-Volatile Storage

### Advanced Features
- **FreeRTOS Integration**: Task riêng để không block main thread
- **Callback System**: 4 callbacks (Start, Progress, End, Error)
- **Auto-Check**: Tự động kiểm tra update theo interval
- **MQTT Control**: Trigger update qua MQTT commands
- **Singleton Pattern**: Đảm bảo chỉ có 1 instance duy nhất

---

## 📦 Yêu cầu / Requirements

### Hardware
- ESP32 (bất kỳ board nào)
- Tối thiểu 4MB Flash (để lưu 2 partitions)
- WiFi connection

### Software
- Arduino IDE 1.8.x hoặc Arduino IDE 2.x
- ESP32 Board Package v2.0.0 trở lên
- Libraries cần thiết:
  - `WiFi.h` (built-in)
  - `HTTPClient.h` (built-in)
  - `Update.h` (built-in)
  - `ArduinoJson.h` (v6.x) - [Install from Library Manager]

### Partition Scheme
**QUAN TRỌNG**: Phải chọn partition scheme có OTA trong Arduino IDE:
- Tools → Partition Scheme → **"Minimal SPIFFS (1.9MB APP with OTA/190KB SPIFFS)"**
- Hoặc: **"Default 4MB with spiffs (1.2MB APP/1.5MB SPIFFS)"**
- Hoặc: Tạo custom partition table

---

## 🔧 Cài đặt / Installation

### Bước 1: Copy files
Copy 2 files sau vào thư mục project của bạn:
```
firmware_client/
├── OTAUpdate.h
├── OTAUpdate.cpp
└── ESP32_client.ino
```

### Bước 2: Install ArduinoJson library
1. Mở Arduino IDE
2. Tools → Manage Libraries
3. Search "ArduinoJson"
4. Install version 6.x

### Bước 3: Configure Partition Scheme
1. Tools → Board → ESP32 Dev Module (hoặc board của bạn)
2. Tools → Partition Scheme → **"Minimal SPIFFS (1.9MB APP with OTA)"**
3. Tools → Flash Size → **4MB** (hoặc lớn hơn)

### Bước 4: Include trong code
```cpp
#include "OTAUpdate.h"
```

---

## 🚀 Cách sử dụng / Usage

### Basic Usage

#### 1. Khai báo và khởi tạo

```cpp
#include "OTAUpdate.h"

#define SLAVE_VERSION "1.0.0"
#define OTA_SERVER_URL "http://192.168.1.100:8080/ota"
#define CLIENT_ID "066420c45a4e819437bbfbea63b83739"

OTAUpdate* ota;

void setup() {
    Serial.begin(115200);
    
    // Initialize WiFi first
    WiFi.begin("YOUR_SSID", "YOUR_PASSWORD");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    
    // Get OTA instance
    ota = &OTAUpdate::getInstance();
    
    // Initialize OTA
    ota->begin(
        OTA_SERVER_URL,     // Server URL
        SLAVE_VERSION,      // Current version
        CLIENT_ID,          // Device ID
        3600000            // Check interval (1 hour)
    );
    
    // Print OTA info
    ota->printInfo();
}

void loop() {
    // Your code here
    delay(1000);
}
```

#### 2. Sử dụng với Callbacks

```cpp
// Define callback functions
void onOTAStart() {
    Serial.println("🚀 OTA Update started!");
}

void onOTAProgress(int progress) {
    Serial.printf("📊 Progress: %d%%\n", progress);
}

void onOTAEnd(bool success) {
    if (success) {
        Serial.println("✅ Update successful!");
    } else {
        Serial.println("❌ Update failed!");
    }
}

void onOTAError(const char* error) {
    Serial.printf("❌ Error: %s\n", error);
}

void setup() {
    // ... WiFi setup ...
    
    ota = &OTAUpdate::getInstance();
    ota->begin(OTA_SERVER_URL, SLAVE_VERSION, CLIENT_ID);
    
    // Register callbacks
    ota->setOnStartCallback(onOTAStart);
    ota->setOnProgressCallback(onOTAProgress);
    ota->setOnEndCallback(onOTAEnd);
    ota->setOnErrorCallback(onOTAError);
}
```

#### 3. Manual Update Check

```cpp
void loop() {
    // Check for updates manually
    if (ota->hasNewVersion()) {
        Serial.println("New version available!");
        
        // Perform update (force = true)
        ota->performUpdate(true);
    }
    
    delay(60000); // Check every minute
}
```

#### 4. MQTT Integration

```cpp
void mqttCallback(char* topic, byte* payload, unsigned int length) {
    String message = "";
    for (int i = 0; i < length; i++) {
        message += (char)payload[i];
    }
    
    String topicStr = String(topic);
    
    // Check if this is OTA command
    if (topicStr.endsWith("/ota")) {
        if (message == "update") {
            // Trigger update in separate task
            xTaskCreate([](void* param) {
                ota->performUpdate(true);
                vTaskDelete(NULL);
            }, "OTAUpdateTask", 8192, NULL, 1, NULL);
        }
        else if (message == "check") {
            if (ota->hasNewVersion()) {
                mqtt->publish("device/notification", "New version available");
            } else {
                mqtt->publish("device/notification", "Already latest");
            }
        }
        else if (message == "version") {
            String ver = "Version: " + ota->getCurrentVersion();
            mqtt->publish("device/notification", ver.c_str());
        }
    }
}
```

---

## 🖥️ OTA Server Setup

### Server Requirements

OTA server cần implement 2 endpoints chính:

#### 1. Check for updates
```
GET /ota/check?device_id={DEVICE_ID}&current_version={VERSION}
```

**Response (JSON):**
```json
{
  "has_update": true,
  "version": "1.0.1",
  "download_url": "http://192.168.1.100:8080/ota/download/firmware.bin",
  "changelog": "Bug fixes and performance improvements"
}
```

#### 2. Download firmware
```
GET /ota/download/firmware.bin
```

**Response:** Binary firmware file (.bin)

### Example Server (Node.js + Express)

Xem file `OTA_SERVER_EXAMPLE.js` để có implementation đầy đủ.

**Quick Setup:**
```bash
npm install express
node OTA_SERVER_EXAMPLE.js
```

### Example Server (Python + Flask)

Xem file `OTA_SERVER_EXAMPLE.py` để có implementation đầy đủ.

**Quick Setup:**
```bash
pip install flask
python OTA_SERVER_EXAMPLE.py
```

---

## 📚 API Reference

### Public Methods

#### `begin()`
```cpp
bool begin(const String& serverUrl, 
           const String& currentVersion, 
           const String& deviceId, 
           int checkInterval = 3600000)
```
Khởi tạo OTA service.

**Parameters:**
- `serverUrl`: URL của OTA server
- `currentVersion`: Phiên bản hiện tại
- `deviceId`: ID thiết bị
- `checkInterval`: Khoảng thời gian check (ms), default: 1 giờ

**Returns:** `true` nếu thành công

---

#### `performUpdate()`
```cpp
bool performUpdate(bool forceUpdate = false)
```
Kiểm tra và thực hiện update.

**Parameters:**
- `forceUpdate`: `true` = update ngay, `false` = chỉ check

**Returns:** `true` nếu update thành công

---

#### `hasNewVersion()`
```cpp
bool hasNewVersion()
```
Kiểm tra xem có phiên bản mới không.

**Returns:** `true` nếu có version mới

---

#### `getCurrentVersion()`
```cpp
String getCurrentVersion() const
```
Lấy phiên bản hiện tại.

**Returns:** String chứa version

---

#### `getProgress()`
```cpp
int getProgress() const
```
Lấy tiến trình update (0-100%).

**Returns:** Số nguyên từ 0-100

---

#### `isUpdateInProgress()`
```cpp
bool isUpdateInProgress() const
```
Kiểm tra có đang update không.

**Returns:** `true` nếu đang update

---

#### `getLastError()`
```cpp
String getLastError() const
```
Lấy lỗi gần nhất.

**Returns:** String chứa error message

---

#### `enableAutoCheck()`
```cpp
void enableAutoCheck(bool enable)
```
Bật/tắt tự động kiểm tra update.

**Parameters:**
- `enable`: `true` = bật, `false` = tắt

---

#### `printInfo()`
```cpp
void printInfo()
```
In thông tin OTA ra Serial.

---

### Callback Functions

#### `setOnStartCallback()`
```cpp
void setOnStartCallback(void (*callback)())
```
Set callback khi bắt đầu update.

---

#### `setOnProgressCallback()`
```cpp
void setOnProgressCallback(void (*callback)(int progress))
```
Set callback khi có tiến trình update.

**Callback Parameter:**
- `progress`: Tiến trình từ 0-100%

---

#### `setOnEndCallback()`
```cpp
void setOnEndCallback(void (*callback)(bool success))
```
Set callback khi kết thúc update.

**Callback Parameter:**
- `success`: `true` nếu thành công, `false` nếu thất bại

---

#### `setOnErrorCallback()`
```cpp
void setOnErrorCallback(void (*callback)(const char* error))
```
Set callback khi có lỗi.

**Callback Parameter:**
- `error`: String chứa error message

---

## 💡 Examples

### Example 1: Simple Check and Update
```cpp
#include "OTAUpdate.h"

void setup() {
    Serial.begin(115200);
    WiFi.begin("SSID", "PASS");
    
    OTAUpdate::getInstance().begin(
        "http://192.168.1.100:8080/ota",
        "1.0.0",
        "device123"
    );
}

void loop() {
    if (OTAUpdate::getInstance().hasNewVersion()) {
        OTAUpdate::getInstance().performUpdate(true);
    }
    delay(3600000); // Check every hour
}
```

### Example 2: With Button Trigger
```cpp
#define UPDATE_BUTTON 0 // Boot button

void loop() {
    if (digitalRead(UPDATE_BUTTON) == LOW) {
        Serial.println("Button pressed - checking for update...");
        
        if (ota->hasNewVersion()) {
            ota->performUpdate(true);
        } else {
            Serial.println("Already on latest version");
        }
        
        delay(1000); // Debounce
    }
}
```

### Example 3: With LED Indicator
```cpp
#define LED_PIN 2

void onOTAProgress(int progress) {
    // Blink LED based on progress
    if (progress % 10 == 0) {
        digitalWrite(LED_PIN, !digitalRead(LED_PIN));
    }
}

void setup() {
    pinMode(LED_PIN, OUTPUT);
    
    ota = &OTAUpdate::getInstance();
    ota->begin("http://192.168.1.100:8080/ota", "1.0.0", "device123");
    ota->setOnProgressCallback(onOTAProgress);
}
```

---

## 🔍 Troubleshooting

### Problem: "Not enough space for OTA"

**Solution:**
- Chọn partition scheme lớn hơn trong Arduino IDE
- Tools → Partition Scheme → "Minimal SPIFFS (1.9MB APP with OTA)"

---

### Problem: "WiFi not connected"

**Solution:**
```cpp
// Đảm bảo WiFi đã connect trước khi gọi OTA
while (WiFi.status() != WL_CONNECTED) {
    delay(500);
}
```

---

### Problem: "HTTP error: -1" hoặc "HTTP error: -11"

**Solution:**
- Kiểm tra server URL có đúng không
- Kiểm tra server có đang chạy không
- Kiểm tra firewall/network
- Test URL bằng browser hoặc Postman

---

### Problem: "JSON parse error"

**Solution:**
- Kiểm tra server response có đúng format JSON không
- Đảm bảo đã cài ArduinoJson library version 6.x

---

### Problem: Update thành công nhưng không reboot

**Solution:**
- ESP32 sẽ tự reboot sau 3 giây
- Nếu không reboot, gọi `ESP.restart()` trong callback

---

### Problem: "Written bytes mismatch"

**Solution:**
- Kiểm tra kết nối WiFi ổn định
- Tăng timeout cho HTTP client
- Kiểm tra file firmware.bin có bị corrupt không

---

## 📝 Notes

### Quan trọng / Important
1. **Partition Scheme**: Phải chọn partition có OTA support
2. **WiFi Connection**: Phải kết nối WiFi trước khi gọi OTA functions
3. **Firmware Size**: Firmware không được vượt quá OTA partition size
4. **Version Format**: Nên dùng semantic versioning (x.y.z)

### Best Practices
1. Luôn test firmware mới trên 1 device trước khi deploy hàng loạt
2. Có backup/rollback mechanism
3. Log tất cả OTA activities
4. Notify users trước khi update
5. Update vào thời điểm ít traffic

### Security
1. **HTTPS**: Nên dùng HTTPS thay vì HTTP
2. **Authentication**: Thêm authentication cho OTA server
3. **Signature Verification**: Verify firmware signature trước khi install
4. **Encrypted Firmware**: Encrypt firmware binary

---

## 📞 Support

Nếu gặp vấn đề:
1. Check Troubleshooting section
2. Enable Serial debug và check logs
3. Test với example code trước
4. Check GitHub issues

---

## 📄 License

MIT License - Free to use for personal and commercial projects.

---

## 🎉 Changelog

### Version 1.0.0
- Initial release
- Basic OTA functionality
- FreeRTOS integration
- Callback system
- NVS storage
- MQTT integration

---

**Happy Coding! 🚀**

