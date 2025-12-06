# 🚀 OTA Update - Quick Start Guide

## Bắt đầu nhanh trong 5 phút / Get Started in 5 Minutes

---

## 📋 Checklist

- [ ] ESP32 board với 4MB Flash
- [ ] Arduino IDE đã cài đặt
- [ ] WiFi connection
- [ ] ArduinoJson library
- [ ] OTA Server đang chạy

---

## 🔧 Bước 1: Cài đặt Dependencies

### 1.1. Arduino IDE Setup
```
Tools → Board → ESP32 Dev Module
Tools → Partition Scheme → "Minimal SPIFFS (1.9MB APP with OTA)"
Tools → Flash Size → 4MB
```

### 1.2. Install ArduinoJson
```
Tools → Manage Libraries → Search "ArduinoJson" → Install version 6.x
```

---

## 📁 Bước 2: Copy Files vào Project

Copy 3 files này vào thư mục project của bạn:
```
firmware_client/
├── OTAUpdate.h       ✅
├── OTAUpdate.cpp     ✅
└── ESP32_client.ino
```

---

## 💻 Bước 3: Thêm Code vào ESP32_client.ino

### 3.1. Include OTA Header
```cpp
#include "OTAUpdate.h"
```

### 3.2. Định nghĩa Config
```cpp
#define SLAVE_VERSION "1.0.0"
#define OTA_SERVER_URL "http://192.168.1.100:8080/ota"  // Thay đổi IP của bạn
#define CLIENT_ID "066420c45a4e819437bbfbea63b83739"
```

### 3.3. Khai báo OTA Instance
```cpp
OTAUpdate* ota;
```

### 3.4. Khởi tạo trong setup()
```cpp
void setup() {
    // ... existing WiFi setup ...
    
    // Get OTA instance
    ota = &OTAUpdate::getInstance();
    
    // Initialize OTA
    ota->begin(OTA_SERVER_URL, SLAVE_VERSION, CLIENT_ID, 3600000);
    
    // Print info
    ota->printInfo();
}
```

**✅ XONG! Bạn đã tích hợp OTA thành công!**

---

## 🖥️ Bước 4: Setup OTA Server

### Option A: Node.js Server (Khuyên dùng)

```bash
# 1. Install Node.js từ https://nodejs.org/

# 2. Install Express
npm install express

# 3. Tạo thư mục firmware
mkdir firmware

# 4. Copy file OTA_SERVER_EXAMPLE.js

# 5. Chạy server
node OTA_SERVER_EXAMPLE.js
```

Server sẽ chạy tại: `http://localhost:8080`

### Option B: Python Server

```bash
# 1. Install Python từ https://www.python.org/

# 2. Install Flask
pip install flask

# 3. Tạo thư mục firmware
mkdir firmware

# 4. Copy file OTA_SERVER_EXAMPLE.py

# 5. Chạy server
python OTA_SERVER_EXAMPLE.py
```

Server sẽ chạy tại: `http://localhost:5000`

---

## 📦 Bước 5: Tạo Firmware Binary

### 5.1. Compile Code
1. Mở Arduino IDE
2. Compile code (Ctrl+R hoặc Verify)
3. Đợi compile xong

### 5.2. Export Binary
1. Click **Sketch → Export compiled Binary**
2. File .bin sẽ được tạo trong thư mục sketch
3. Tên file: `ESP32_client.ino.esp32.bin`

### 5.3. Copy vào Firmware Folder
```bash
# Rename file theo version
cp ESP32_client.ino.esp32.bin firmware/firmware_v1.0.0.bin
```

---

## 🌐 Bước 6: Configure Network

### 6.1. Tìm IP Address của máy tính

**Windows:**
```cmd
ipconfig
```
Tìm dòng: `IPv4 Address. . . . . . . . . . . : 192.168.1.XXX`

**Mac/Linux:**
```bash
ifconfig
```
Tìm dòng: `inet 192.168.1.XXX`

### 6.2. Update OTA_SERVER_URL trong code

```cpp
#define OTA_SERVER_URL "http://192.168.1.XXX:8080/ota"  // Thay XXX bằng IP của bạn
```

---

## 🧪 Bước 7: Test OTA Update

### 7.1. Upload Firmware Version 1.0.0
1. Set `#define SLAVE_VERSION "1.0.0"` trong code
2. Upload lên ESP32
3. Mở Serial Monitor
4. Xem log: `Current Version: 1.0.0`

### 7.2. Tạo Version 1.0.1
1. Thay đổi code (ví dụ: thêm 1 dòng print)
2. Set `#define SLAVE_VERSION "1.0.1"` 
3. Compile → Export Binary
4. Copy `.bin` file vào `firmware/firmware_v1.0.1.bin`

### 7.3. Update Server Config
Edit `OTA_SERVER_EXAMPLE.js` (hoặc `.py`):
```javascript
latestVersion: '1.0.1',  // Thay đổi từ 1.0.0 → 1.0.1
```

### 7.4. Restart Server
```bash
# Node.js
node OTA_SERVER_EXAMPLE.js

# Python
python OTA_SERVER_EXAMPLE.py
```

### 7.5. Trigger Update

**Option A: Tự động (ESP32 sẽ tự check sau 1 giờ)**
- Đợi hoặc restart ESP32

**Option B: MQTT Command (nếu đã tích hợp MQTT)**
```bash
# Publish MQTT message
Topic: OTA/066420c45a4e819437bbfbea63b83739/ota
Payload: update
```

**Option C: Code Manual Trigger**
```cpp
void loop() {
    if (digitalRead(BUTTON_PIN) == LOW) {
        ota->performUpdate(true);  // Force update
    }
}
```

### 7.6. Xem Log trên Serial Monitor
```
🔍 [OTA] Checking for updates...
🎉 [OTA] New version available!
   📌 New Version: 1.0.1
📥 [OTA] Starting firmware download...
📊 [OTA] Progress: 10%
📊 [OTA] Progress: 20%
...
📊 [OTA] Progress: 100%
✅ [OTA] Update successfully completed!
🔄 [OTA] Rebooting in 3 seconds...
```

### 7.7. Verify New Version
Sau khi ESP32 reboot:
```
Current Version: 1.0.1  ✅
```

**🎉 Chúc mừng! Bạn đã update firmware OTA thành công!**

---

## 🎯 Cách sử dụng nâng cao

### 1. MQTT Control Commands

Subscribe ESP32 vào topic: `OTA/{CLIENT_ID}/ota`

**Commands:**
```bash
# Trigger update
Payload: "update"

# Check for new version
Payload: "check"

# Get current version
Payload: "version"

# Print OTA info
Payload: "info"
```

### 2. Callbacks

```cpp
void onOTAStart() {
    Serial.println("Starting update...");
    // Tắt các sensor, LED, etc.
}

void onOTAProgress(int progress) {
    Serial.printf("Progress: %d%%\n", progress);
    // Update LED indicator
}

void onOTAEnd(bool success) {
    if (success) {
        Serial.println("Update successful!");
    }
}

void onOTAError(const char* error) {
    Serial.printf("Error: %s\n", error);
}

void setup() {
    // ... setup code ...
    
    ota->setOnStartCallback(onOTAStart);
    ota->setOnProgressCallback(onOTAProgress);
    ota->setOnEndCallback(onOTAEnd);
    ota->setOnErrorCallback(onOTAError);
}
```

### 3. Manual Check & Update

```cpp
// Check if new version available
if (ota->hasNewVersion()) {
    Serial.println("New version available!");
    
    // Force update
    bool success = ota->performUpdate(true);
    
    if (success) {
        Serial.println("Update successful!");
    } else {
        Serial.println("Update failed: " + ota->getLastError());
    }
}
```

---

## 🐛 Troubleshooting

### ❌ Problem: "Not enough space for OTA"
**Solution:** 
```
Tools → Partition Scheme → "Minimal SPIFFS (1.9MB APP with OTA)"
```

### ❌ Problem: "WiFi not connected"
**Solution:** Đảm bảo WiFi đã connect trước khi gọi `ota->begin()`

### ❌ Problem: "HTTP error: -1"
**Solution:**
- Check IP address đúng chưa
- Check server đang chạy chưa (`netstat -an | find "8080"`)
- Check firewall
- Test bằng browser: `http://192.168.1.XXX:8080/ota/check?device_id=test&current_version=1.0.0`

### ❌ Problem: "JSON parse error"
**Solution:** 
- Check ArduinoJson đã install chưa
- Check server response format (phải là JSON)

### ❌ Problem: Update thành công nhưng không reboot
**Solution:** ESP32 sẽ tự reboot sau 3 giây. Nếu không, thêm `ESP.restart()` trong callback.

---

## 📚 Tài liệu đầy đủ

- **OTA_README.md** - Hướng dẫn chi tiết đầy đủ
- **OTA_USAGE_EXAMPLE.ino** - Code example đầy đủ
- **OTA_SERVER_EXAMPLE.js** - Node.js server
- **OTA_SERVER_EXAMPLE.py** - Python server

---

## 💡 Tips

1. **Version Naming:** Dùng semantic versioning (1.0.0, 1.0.1, 1.1.0)
2. **Test First:** Test trên 1 device trước khi deploy hàng loạt
3. **Backup:** Luôn giữ backup của firmware working version
4. **Monitoring:** Log tất cả OTA activities
5. **Security:** Nên dùng HTTPS thay vì HTTP trong production

---

## 🎊 Hoàn thành!

Bạn đã setup thành công OTA Update cho ESP32! 🎉

**Next Steps:**
- [ ] Test với nhiều devices
- [ ] Tích hợp với production server
- [ ] Thêm authentication
- [ ] Setup HTTPS
- [ ] Implement rollback mechanism

**Need Help?**
- Check OTA_README.md để biết chi tiết
- Check Troubleshooting section
- Enable Serial debug để xem logs

---

**Happy Updating! 🚀**

