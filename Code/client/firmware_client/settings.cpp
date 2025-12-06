#include "settings.h"

// Hàm static để khởi tạo NVS một lần duy nhất
bool Settings::initializeNVS() {
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        Serial.println("🔄 [Settings] NVS partition was truncated and needs to be erased");
        Serial.println("⚠️ [Settings] WARNING: This will erase ALL NVS data!");
        
        err = nvs_flash_erase();
        if (err != ESP_OK) {
            Serial.printf("❌ [Settings] Failed to erase NVS: %s\n", esp_err_to_name(err));
            return false;
        }
        
        err = nvs_flash_init();
        if (err != ESP_OK) {
            Serial.printf("❌ [Settings] Failed to initialize NVS after erase: %s\n", esp_err_to_name(err));
            return false;
        }
        
        Serial.println("✅ [Settings] NVS erased and re-initialized");
    } else if (err != ESP_OK) {
        Serial.printf("❌ [Settings] Failed to initialize NVS: %s\n", esp_err_to_name(err));
        return false;
    } else {
        Serial.println("✅ [Settings] NVS initialized successfully");
    }
    
    return true;
}

Settings::Settings(const String& ns, bool read_write)
: _namespace(ns), _readWrite(read_write) {

    // Chỉ mở namespace, không khởi tạo NVS ở đây
    // NVS phải được khởi tạo trước đó trong setup() hoặc main()
    esp_err_t err = nvs_open(_namespace.c_str(), _readWrite ? NVS_READWRITE : NVS_READONLY, &_handle);
    if (err != ESP_OK) {
        Serial.printf("⚠️ [Settings] Failed to open namespace '%s': %s\n",
                      _namespace.c_str(), esp_err_to_name(err));
        Serial.printf("💡 [Settings] Make sure NVS is initialized with nvs_flash_init() first!\n");
    } else {
        Serial.printf("✅ [Settings] Namespace '%s' opened (%s)\n",
                      _namespace.c_str(), _readWrite ? "RW" : "RO");
    }
}

Settings::~Settings() {
    if (_handle != 0) {
        if (_readWrite && _dirty) {
            nvs_commit(_handle);
        }
        nvs_close(_handle);
    }
}

String Settings::getString(const String& key, const String& default_value) {
    if (_handle == 0) return default_value;

    size_t length = 0;
    esp_err_t err = nvs_get_str(_handle, key.c_str(), nullptr, &length);
    if (err != ESP_OK || length == 0) return default_value;

    char* buf = (char*)malloc(length);
    if (buf == nullptr) return default_value;
    
    err = nvs_get_str(_handle, key.c_str(), buf, &length);
    if (err != ESP_OK) {
        free(buf);
        return default_value;
    }

    String value = buf;
    free(buf);
    return value;
}

void Settings::setString(const String& key, const String& value) {
    if (!_readWrite) {
        Serial.printf("⚠️ [Settings] Namespace '%s' not writable\n", _namespace.c_str());
        return;
    }
    esp_err_t err = nvs_set_str(_handle, key.c_str(), value.c_str());
    if (err != ESP_OK) {
        Serial.printf("⚠️ [Settings] Failed to set string '%s': %s\n", key.c_str(), esp_err_to_name(err));
        return;
    }
    _dirty = true;
}

int32_t Settings::getInt(const String& key, int32_t default_value) {
    if (_handle == 0) return default_value;

    int32_t value;
    esp_err_t err = nvs_get_i32(_handle, key.c_str(), &value);
    if (err != ESP_OK) return default_value;
    return value;
}

void Settings::setInt(const String& key, int32_t value) {
    if (!_readWrite) {
        Serial.printf("⚠️ [Settings] Namespace '%s' not writable\n", _namespace.c_str());
        return;
    }
    esp_err_t err = nvs_set_i32(_handle, key.c_str(), value);
    if (err != ESP_OK) {
        Serial.printf("⚠️ [Settings] Failed to set int '%s': %s\n", key.c_str(), esp_err_to_name(err));
        return;
    }
    _dirty = true;
}

bool Settings::getBool(const String& key, bool default_value) {
    if (_handle == 0) return default_value;

    uint8_t val;
    esp_err_t err = nvs_get_u8(_handle, key.c_str(), &val);
    if (err != ESP_OK) return default_value;
    return val != 0;
}

void Settings::setBool(const String& key, bool value) {
    if (!_readWrite) {
        Serial.printf("⚠️ [Settings] Namespace '%s' not writable\n", _namespace.c_str());
        return;
    }
    esp_err_t err = nvs_set_u8(_handle, key.c_str(), value ? 1 : 0);
    if (err != ESP_OK) {
        Serial.printf("⚠️ [Settings] Failed to set bool '%s': %s\n", key.c_str(), esp_err_to_name(err));
        return;
    }
    _dirty = true;
}

void Settings::eraseKey(const String& key) {
    if (!_readWrite) {
        Serial.printf("⚠️ [Settings] Namespace '%s' not writable\n", _namespace.c_str());
        return;
    }
    esp_err_t err = nvs_erase_key(_handle, key.c_str());
    if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) {
        Serial.printf("⚠️ [Settings] Failed to erase key '%s': %s\n", key.c_str(), esp_err_to_name(err));
        return;
    }
    _dirty = true;
}

void Settings::eraseAll() {
    if (!_readWrite) {
        Serial.printf("⚠️ [Settings] Namespace '%s' not writable\n", _namespace.c_str());
        return;
    }
    esp_err_t err = nvs_erase_all(_handle);
    if (err != ESP_OK) {
        Serial.printf("⚠️ [Settings] Failed to erase all: %s\n", esp_err_to_name(err));
        return;
    }
    _dirty = true;
}

// Hàm kiểm tra thông tin NVS
void Settings::printNVSInfo() {
    nvs_stats_t nvs_stats;
    esp_err_t err = nvs_get_stats("nvs", &nvs_stats);
    
    if (err != ESP_OK) {
        Serial.printf("❌ [Settings] Failed to get NVS stats: %s\n", esp_err_to_name(err));
        return;
    }
    
    Serial.println("📊 ===== NVS Storage Information =====");
    Serial.printf("   Total entries: %d\n", nvs_stats.total_entries);
    Serial.printf("   Used entries: %d\n", nvs_stats.used_entries);
    Serial.printf("   Free entries: %d\n", nvs_stats.free_entries);
    
    // Tính phần trăm sử dụng
    float usagePercent = (float)nvs_stats.used_entries / (float)nvs_stats.total_entries * 100.0;
    Serial.printf("   Usage: %.1f%%\n", usagePercent);
    
    // Ước tính dung lượng (mỗi entry ~32 bytes overhead + data)
    // NVS mặc định thường là 24KB (24576 bytes)
    size_t estimatedSizeKB = (nvs_stats.total_entries * 32) / 1024;
    Serial.printf("   Estimated size: ~%d KB\n", estimatedSizeKB);
    
    if (usagePercent > 80) {
        Serial.println("⚠️ [Settings] WARNING: NVS usage is above 80%!");
        Serial.println("💡 [Settings] Consider cleaning unused data or increasing NVS partition size");
    } else if (usagePercent > 50) {
        Serial.println("💡 [Settings] NVS usage is moderate");
    } else {
        Serial.println("✅ [Settings] NVS has plenty of free space");
    }
    Serial.println("=====================================");
}

size_t Settings::getNVSFreeEntries() {
    nvs_stats_t nvs_stats;
    if (nvs_get_stats("nvs", &nvs_stats) == ESP_OK) {
        return nvs_stats.free_entries;
    }
    return 0;
}

size_t Settings::getNVSUsedEntries() {
    nvs_stats_t nvs_stats;
    if (nvs_get_stats("nvs", &nvs_stats) == ESP_OK) {
        return nvs_stats.used_entries;
    }
    return 0;
}

size_t Settings::getNVSTotalEntries() {
    nvs_stats_t nvs_stats;
    if (nvs_get_stats("nvs", &nvs_stats) == ESP_OK) {
        return nvs_stats.total_entries;
    }
    return 0;
}