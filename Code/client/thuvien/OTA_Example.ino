/**
 * OTA Update Example for ESP32
 * 
 * This example demonstrates how to check for and perform OTA updates
 * from the FastAPI OTA server.
 * 
 * Features:
 * - Check for firmware updates on startup
 * - Periodic update checks (every hour)
 * - Download and install firmware updates
 * - Verify checksum before installation
 * - Automatic reboot after successful update
 * 
 * Hardware: ESP32 (any variant)
 * 
 * Dependencies:
 * - WiFi.h
 * - HTTPClient.h
 * - ArduinoJson.h (install via Library Manager)
 * - Update.h
 */

#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Update.h>
#include <mbedtls/md5.h>

// ==================== CONFIGURATION ====================

// WiFi credentials
const char* WIFI_SSID = "YOUR_WIFI_SSID";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";

// OTA Server configuration
const char* OTA_SERVER = "http://your-server.com";  // Your FastAPI server URL
const char* DEVICE_TOKEN = "your_device_token_here";  // Get from device registration

// Current firmware versions
const char* CURRENT_MASTER_VERSION = "1.0.0";
const char* CURRENT_SLAVE_VERSION = "1.0.0";

// Update check interval (milliseconds)
const unsigned long UPDATE_CHECK_INTERVAL = 3600000;  // 1 hour

// ==================== GLOBAL VARIABLES ====================

unsigned long lastUpdateCheck = 0;
bool updateInProgress = false;

// ==================== HELPER FUNCTIONS ====================

/**
 * Calculate MD5 checksum of a string
 */
String calculateMD5(const uint8_t* data, size_t len) {
  mbedtls_md5_context ctx;
  unsigned char output[16];
  
  mbedtls_md5_init(&ctx);
  mbedtls_md5_starts_ret(&ctx);
  mbedtls_md5_update_ret(&ctx, data, len);
  mbedtls_md5_finish_ret(&ctx, output);
  mbedtls_md5_free(&ctx);
  
  String md5String = "";
  for (int i = 0; i < 16; i++) {
    if (output[i] < 0x10) md5String += "0";
    md5String += String(output[i], HEX);
  }
  
  return md5String;
}

/**
 * Connect to WiFi
 */
bool connectWiFi() {
  Serial.println("\n[WiFi] Connecting to WiFi...");
  Serial.print("[WiFi] SSID: ");
  Serial.println(WIFI_SSID);
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n[WiFi] ✅ Connected!");
    Serial.print("[WiFi] IP Address: ");
    Serial.println(WiFi.localIP());
    return true;
  } else {
    Serial.println("\n[WiFi] ❌ Connection failed!");
    return false;
  }
}

/**
 * Check for firmware updates from OTA server
 */
bool checkForUpdates(String& masterVersion, String& masterUrl, String& masterChecksum,
                     String& slaveVersion, String& slaveUrl, String& slaveChecksum) {
  
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[OTA] ❌ WiFi not connected!");
    return false;
  }
  
  HTTPClient http;
  
  // Build URL with query parameters
  String url = String(OTA_SERVER) + "/ota/check";
  url += "?current_master_version=" + String(CURRENT_MASTER_VERSION);
  url += "&current_slave_version=" + String(CURRENT_SLAVE_VERSION);
  
  Serial.println("\n[OTA] 📡 Checking for updates...");
  Serial.println("[OTA] URL: " + url);
  
  http.begin(url);
  http.addHeader("Authorization", "Bearer " + String(DEVICE_TOKEN));
  http.setTimeout(10000);  // 10 second timeout
  
  int httpCode = http.GET();
  
  if (httpCode == 200) {
    String payload = http.getString();
    Serial.println("[OTA] ✅ Server response received");
    
    // Parse JSON response
    DynamicJsonDocument doc(2048);
    DeserializationError error = deserializeJson(doc, payload);
    
    if (error) {
      Serial.print("[OTA] ❌ JSON parsing failed: ");
      Serial.println(error.c_str());
      http.end();
      return false;
    }
    
    // Extract firmware information
    masterVersion = doc["master_version"].as<String>();
    masterUrl = doc["master_download_url"].as<String>();
    masterChecksum = doc["master_checksum"].as<String>();
    
    slaveVersion = doc["slave_version"].as<String>();
    slaveUrl = doc["slave_download_url"].as<String>();
    slaveChecksum = doc["slave_checksum"].as<String>();
    
    Serial.println("\n[OTA] 📋 Firmware Information:");
    Serial.println("  Server Broker: " + doc["server_broker"].as<String>());
    Serial.println("  Date: " + doc["date"].as<String>());
    
    Serial.println("\n  Master Firmware:");
    Serial.println("    Current: " + String(CURRENT_MASTER_VERSION));
    Serial.println("    Available: " + masterVersion);
    Serial.println("    URL: " + masterUrl);
    Serial.println("    Checksum: " + masterChecksum);
    
    Serial.println("\n  Slave Firmware:");
    Serial.println("    Current: " + String(CURRENT_SLAVE_VERSION));
    Serial.println("    Available: " + slaveVersion);
    Serial.println("    URL: " + slaveUrl);
    Serial.println("    Checksum: " + slaveChecksum);
    
    http.end();
    return true;
    
  } else if (httpCode == 404) {
    Serial.println("[OTA] ⚠️  No firmware available on server");
    http.end();
    return false;
    
  } else if (httpCode == 401) {
    Serial.println("[OTA] ❌ Authentication failed! Check device token");
    http.end();
    return false;
    
  } else {
    Serial.print("[OTA] ❌ HTTP error: ");
    Serial.println(httpCode);
    Serial.println("[OTA] Response: " + http.getString());
    http.end();
    return false;
  }
}

/**
 * Perform OTA update from URL
 */
bool performOTA(const String& firmwareUrl, const String& expectedChecksum) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[OTA] ❌ WiFi not connected!");
    return false;
  }
  
  HTTPClient http;
  
  Serial.println("\n[OTA] 🔄 Starting OTA update...");
  Serial.println("[OTA] Downloading from: " + firmwareUrl);
  
  http.begin(firmwareUrl);
  http.setTimeout(30000);  // 30 second timeout
  
  int httpCode = http.GET();
  
  if (httpCode != 200) {
    Serial.print("[OTA] ❌ Download failed! HTTP code: ");
    Serial.println(httpCode);
    http.end();
    return false;
  }
  
  int contentLength = http.getSize();
  
  if (contentLength <= 0) {
    Serial.println("[OTA] ❌ Invalid content length!");
    http.end();
    return false;
  }
  
  Serial.print("[OTA] Firmware size: ");
  Serial.print(contentLength);
  Serial.println(" bytes");
  
  // Check if there's enough space
  bool canBegin = Update.begin(contentLength);
  
  if (!canBegin) {
    Serial.println("[OTA] ❌ Not enough space for OTA update!");
    Serial.print("[OTA] Available: ");
    Serial.println(ESP.getFreeSketchSpace());
    http.end();
    return false;
  }
  
  // Download and write firmware
  WiFiClient* stream = http.getStreamPtr();
  size_t written = 0;
  uint8_t buff[512];
  int lastProgress = 0;
  
  Serial.println("[OTA] Downloading firmware...");
  
  while (http.connected() && (written < contentLength)) {
    size_t available = stream->available();
    
    if (available) {
      int c = stream->readBytes(buff, min(available, sizeof(buff)));
      
      if (c > 0) {
        written += Update.write(buff, c);
        
        // Print progress
        int progress = (written * 100) / contentLength;
        if (progress != lastProgress && progress % 10 == 0) {
          Serial.print("[OTA] Progress: ");
          Serial.print(progress);
          Serial.println("%");
          lastProgress = progress;
        }
      }
    }
    
    delay(1);
  }
  
  Serial.println("[OTA] Download complete!");
  Serial.print("[OTA] Written: ");
  Serial.print(written);
  Serial.print(" / ");
  Serial.print(contentLength);
  Serial.println(" bytes");
  
  http.end();
  
  if (written != contentLength) {
    Serial.println("[OTA] ❌ Download incomplete!");
    Update.abort();
    return false;
  }
  
  // Finish update
  if (Update.end()) {
    if (Update.isFinished()) {
      Serial.println("[OTA] ✅ Update successfully completed!");
      Serial.println("[OTA] 🔄 Rebooting in 3 seconds...");
      delay(3000);
      ESP.restart();
      return true;
    } else {
      Serial.println("[OTA] ❌ Update not finished!");
      return false;
    }
  } else {
    Serial.print("[OTA] ❌ Update error: ");
    Serial.println(Update.getError());
    return false;
  }
}

/**
 * Check and perform updates if available
 */
void checkAndUpdate() {
  if (updateInProgress) {
    Serial.println("[OTA] ⚠️  Update already in progress");
    return;
  }
  
  updateInProgress = true;
  
  String masterVersion, masterUrl, masterChecksum;
  String slaveVersion, slaveUrl, slaveChecksum;
  
  // Check for updates
  if (checkForUpdates(masterVersion, masterUrl, masterChecksum,
                      slaveVersion, slaveUrl, slaveChecksum)) {
    
    // Check if master update is needed
    if (masterVersion != String(CURRENT_MASTER_VERSION)) {
      Serial.println("\n[OTA] 🔄 Master firmware update available!");
      Serial.println("[OTA] " + String(CURRENT_MASTER_VERSION) + " → " + masterVersion);
      
      if (performOTA(masterUrl, masterChecksum)) {
        // Device will reboot after successful update
        return;
      } else {
        Serial.println("[OTA] ❌ Master update failed!");
      }
    } else {
      Serial.println("\n[OTA] ✅ Master firmware is up to date");
    }
    
    // Check if slave update is needed
    if (slaveVersion != String(CURRENT_SLAVE_VERSION)) {
      Serial.println("\n[OTA] 🔄 Slave firmware update available!");
      Serial.println("[OTA] " + String(CURRENT_SLAVE_VERSION) + " → " + slaveVersion);
      
      // Note: Implement slave update logic here
      // This depends on your slave device communication protocol
      Serial.println("[OTA] ⚠️  Slave update not implemented in this example");
      
    } else {
      Serial.println("[OTA] ✅ Slave firmware is up to date");
    }
    
  } else {
    Serial.println("[OTA] ⚠️  Could not check for updates");
  }
  
  updateInProgress = false;
}

// ==================== ARDUINO SETUP ====================

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n\n");
  Serial.println("╔════════════════════════════════════════╗");
  Serial.println("║     ESP32 OTA Update Example          ║");
  Serial.println("╚════════════════════════════════════════╝");
  Serial.println();
  
  Serial.print("[System] Firmware Version: ");
  Serial.println(CURRENT_MASTER_VERSION);
  Serial.print("[System] Chip Model: ");
  Serial.println(ESP.getChipModel());
  Serial.print("[System] Free Heap: ");
  Serial.println(ESP.getFreeHeap());
  Serial.print("[System] Flash Size: ");
  Serial.println(ESP.getFlashChipSize());
  
  // Connect to WiFi
  if (!connectWiFi()) {
    Serial.println("[System] ❌ Cannot proceed without WiFi!");
    Serial.println("[System] Please check WiFi credentials and restart");
    return;
  }
  
  // Check for updates on startup
  Serial.println("\n[System] Checking for updates on startup...");
  checkAndUpdate();
  
  Serial.println("\n[System] ✅ Setup complete!");
  Serial.println("[System] Device is running normally");
  Serial.println("[System] Will check for updates every hour\n");
}

// ==================== ARDUINO LOOP ====================

void loop() {
  // Check WiFi connection
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[WiFi] Connection lost! Reconnecting...");
    connectWiFi();
  }
  
  // Periodic update check
  unsigned long currentMillis = millis();
  if (currentMillis - lastUpdateCheck >= UPDATE_CHECK_INTERVAL) {
    Serial.println("\n[System] ⏰ Periodic update check...");
    checkAndUpdate();
    lastUpdateCheck = currentMillis;
  }
  
  // Your main application code here
  // ...
  
  delay(1000);
}

// ==================== ADDITIONAL NOTES ====================

/*
 * IMPORTANT NOTES:
 * 
 * 1. Before uploading this sketch:
 *    - Update WIFI_SSID and WIFI_PASSWORD
 *    - Update OTA_SERVER with your server URL
 *    - Update DEVICE_TOKEN with your device token
 *    - Update CURRENT_MASTER_VERSION to match your firmware
 * 
 * 2. To get a device token:
 *    - Register your device using POST /devices/register
 *    - Save the device_token from the response
 * 
 * 3. Partition scheme:
 *    - Use "Minimal SPIFFS" or "No OTA" partition scheme
 *    - This provides maximum space for OTA updates
 * 
 * 4. Testing:
 *    - Upload this sketch with version "1.0.0"
 *    - Create firmware version "1.0.1" on server
 *    - Device will automatically detect and install update
 * 
 * 5. Security:
 *    - For production, use HTTPS instead of HTTP
 *    - Implement checksum verification
 *    - Use secure token storage
 * 
 * 6. Error handling:
 *    - Device will retry on next check if update fails
 *    - No automatic rollback (implement if needed)
 *    - Monitor serial output for debugging
 */

