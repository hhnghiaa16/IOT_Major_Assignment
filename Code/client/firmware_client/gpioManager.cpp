#include "gpioManager.h"

GPIOManager::GPIOManager() : _initialized(false), _lastDataSend(0) {
    // Initialize pin configurations
    for (int i = 0; i < 40; i++) {
        _pinConfigs[i].pin = i;
        _pinConfigs[i].mode = -1;
        _pinConfigs[i].pwmChannel = -1;
        _pinConfigs[i].frequency = 1000;
        _pinConfigs[i].isConfigured = false;
    }
}

void GPIOManager::begin() {
    Serial.println("🔌 [GPIOManager] Initializing GPIO Manager...");
    
    // Load GPIO configuration from NVS
    loadGPIOConfig();
    
    // Configure default pins
    // setOutputPin(2, false); // Built-in LED
    
    _initialized = true;
    Serial.println("✅ [GPIOManager] GPIO Manager initialized");
}

// void GPIOManager::loop() {
//     if (!_initialized) return;
    
//     // Read sensor data every 5 seconds
//     if (millis() - _lastDataSend > 5000) {
//         // readSensors();
//         // sendSensorData();
//         // _lastDataSend = millis();
//     }
// }

void GPIOManager::setInputPin(int pin, bool pullup) {
    if (pin < 0 || pin >= 40) return;
    
    pinMode(pin, pullup ? INPUT_PULLUP : INPUT);
    _pinConfigs[pin].mode = pullup ? INPUT_PULLUP : INPUT;
    _pinConfigs[pin].isConfigured = true;
    
    Serial.printf("📥 [GPIOManager] Pin %d configured as INPUT%s\n", 
                  pin, pullup ? "_PULLUP" : "");
}

void GPIOManager::setOutputPin(int pin, bool initialValue) {
    if (pin < 0 || pin >= 40) return;
    
    pinMode(pin, OUTPUT);
    digitalWrite(pin, initialValue);
    _pinConfigs[pin].mode = OUTPUT;
    _pinConfigs[pin].isConfigured = true;
    
    Serial.printf("📤 [GPIOManager] Pin %d configured as OUTPUT (initial: %s)\n", 
                  pin, initialValue ? "HIGH" : "LOW");
}

void GPIOManager::setPWMChannel(int pin, int channel, int frequency) {
    if (pin < 0 || pin >= 40 || channel < 0 || channel > 15) return;
    
    // ESP32 core 3.x uses ledcAttach with parameters: pin, freq, resolution
    ledcAttach(pin, frequency, 8); // 8-bit resolution
    
    _pinConfigs[pin].mode = OUTPUT;
    _pinConfigs[pin].pwmChannel = channel;
    _pinConfigs[pin].frequency = frequency;
    _pinConfigs[pin].isConfigured = true;
    
    Serial.printf("🌊 [GPIOManager] Pin %d configured as PWM (channel: %d, freq: %d Hz)\n", 
                  pin, channel, frequency);
}

bool GPIOManager::readDigital(int pin) {
    if (pin < 0 || pin >= 40) {
        Serial.printf("⚠️ [GPIOManager] Invalid pin number: %d\n", pin);
        return false;
    }
    
    // Nếu pin chưa được config, tự động config làm INPUT
    if (!_pinConfigs[pin].isConfigured) {
        Serial.printf("💡 [GPIOManager] Pin %d not configured, auto-configuring as INPUT\n", pin);
        setInputPin(pin, true); // Auto-config với pullup
    }
    
    return digitalRead(pin);
}

void GPIOManager::writeDigital(int pin, bool value) {
    if (pin < 0 || pin >= 40) {
        Serial.printf("⚠️ [GPIOManager] Invalid pin number: %d\n", pin);
        return;
    }
    
    // Nếu pin chưa được config, tự động config làm OUTPUT
    if (!_pinConfigs[pin].isConfigured) {
        Serial.printf("💡 [GPIOManager] Pin %d not configured, auto-configuring as OUTPUT\n", pin);
        setOutputPin(pin, value);
        return;
    }
    
    digitalWrite(pin, value);
}

int GPIOManager::readAnalog(int pin) {
    if (pin < 0 || pin >= 40) {
        Serial.printf("⚠️ [GPIOManager] Invalid pin number: %d\n", pin);
        return 0;
    }
    
    // ESP32 chỉ có một số pin hỗ trợ ADC
    // ADC1: GPIO 32, 33, 34, 35, 36, 39
    // ADC2: GPIO 0, 2, 4, 12, 13, 14, 15, 25, 26, 27
    // Kiểm tra xem pin có hỗ trợ ADC không
    if (!isValidAnalogPin(pin)) {
        Serial.printf("⚠️ [GPIOManager] Pin %d does not support analog read\n", pin);
        return 0;
    }
    
    // ESP32 analogRead trả về giá trị 0-4095 (12-bit)
    return analogRead(pin);
}


void GPIOManager::writeAnalog(int pin, int value) {
    if (pin < 0 || pin >= 40) {
        Serial.printf("⚠️ [GPIOManager] Invalid pin number: %d\n", pin);
        return;
    }
    
    // Trên ESP32, analogWrite() thực chất là PWM
    // Nếu pin chưa được config là PWM, tự động config
    if (!_pinConfigs[pin].isConfigured || _pinConfigs[pin].pwmChannel < 0) {
        Serial.printf("💡 [GPIOManager] Pin %d not configured for PWM, auto-configuring\n", pin);
        // Tự động config PWM với channel = pin % 16 (để tránh conflict)
        setPWMChannel(pin, pin % 16, 1000);
    }
    
    // Constrain value từ 0-255
    value = constrain(value, 0, 255);
    analogWrite(pin, value);
}

void GPIOManager::writePWM(int pin, int dutyCycle) {
    if (pin < 0 || pin >= 40) {
        Serial.printf("⚠️ [GPIOManager] Invalid pin number: %d\n", pin);
        return;
    }
    
    // Nếu pin chưa được config là PWM, tự động config
    if (!_pinConfigs[pin].isConfigured || _pinConfigs[pin].pwmChannel < 0) {
        Serial.printf("💡 [GPIOManager] Pin %d not configured for PWM, auto-configuring\n", pin);
        setPWMChannel(pin, pin % 16, 1000);
    }
    
    dutyCycle = constrain(dutyCycle, 0, 255);
    // ESP32 core 3.x uses pin instead of channel
    ledcWrite(pin, dutyCycle);
}

// Thêm hàm đọc giá trị PWM hiện tại
int GPIOManager::readPWM(int pin) {
    if (pin < 0 || pin >= 40) {
        Serial.printf("⚠️ [GPIOManager] Invalid pin number: %d\n", pin);
        return -1;
    }
    
    if (!_pinConfigs[pin].isConfigured || _pinConfigs[pin].pwmChannel < 0) {
        Serial.printf("⚠️ [GPIOManager] Pin %d is not configured as PWM\n", pin);
        return -1;
    }
    
    // ESP32 core 3.x không có hàm ledcRead trực tiếp
    // Chúng ta có thể lưu giá trị duty cycle cuối cùng được set
    // Hoặc đọc từ hardware register (phức tạp hơn)
    // Tạm thời trả về -1 để báo không hỗ trợ
    // TODO: Implement cách lưu/đọc duty cycle nếu cần
    return -1; // Không hỗ trợ đọc PWM value từ hardware
}

void GPIOManager::processCommand(const int virtualPin, const String& message , bool isDigital) {
    Serial.printf("🎛️ [GPIOManager] Processing command: VirtualPin %d = %s (Digital: %s)\n", 
                  virtualPin, message.c_str(), isDigital ? "Yes" : "No");
    
    if (isDigital) {
        // Xử lý lệnh Digital
        bool state = false;
        if (message == "true" || message == "HIGH" || message == "1") {
            state = true;
        } else if (message == "false" || message == "LOW" || message == "0") {
            state = false;
        } else {
            state = (message.toInt() >= 1) ? true : false;
        }
        
        writeDigital(virtualPin, state);
        Serial.printf("📤 [GPIOManager] Pin %d set to %s\n", virtualPin, state ? "HIGH" : "LOW");
    } else {
        // Xử lý lệnh Analog/PWM
        float value = message.toFloat();
        
        // Kiểm tra nếu pin đã được config là PWM
        if (_pinConfigs[virtualPin].isConfigured && _pinConfigs[virtualPin].pwmChannel >= 0) {
            // Sử dụng writePWM nếu pin đã là PWM
            writePWM(virtualPin, (int)value);
            Serial.printf("🌊 [GPIOManager] Pin %d PWM set to %d (%.1f%%)\n", 
                         virtualPin, (int)value, (value / 255.0) * 100.0);
        } else {
            // Sử dụng writeAnalog (sẽ tự động config PWM nếu cần)
            writeAnalog(virtualPin, (int)value);
            Serial.printf("📊 [GPIOManager] Pin %d analog/PWM set to %d\n", virtualPin, (int)value);
        }
    }
}

String GPIOManager::getStatus() const {
    String status = "{";
    status += "\"initialized\":" + String(_initialized ? "true" : "false") + ",";
    status += "\"configured_pins\":[";
    
    bool first = true;
    for (int i = 0; i < 40; i++) {
        if (_pinConfigs[i].isConfigured) {
            if (!first) status += ",";
            status += "{";
            status += "\"pin\":" + String(i) + ",";
            status += "\"mode\":" + String(_pinConfigs[i].mode) + ",";
            status += "\"pwm_channel\":" + String(_pinConfigs[i].pwmChannel) + ",";
            status += "\"frequency\":" + String(_pinConfigs[i].frequency);
            status += "}";
            first = false;
        }
    }
    
    status += "]}";
    return status;
}

void GPIOManager::loadGPIOConfig() {
    Settings gpioSettings("gpio", true);
    
    // Load pin configurations
    for (int i = 0; i < 40; i++) {
        String pinKey = "pin_" + String(i);
        int mode = gpioSettings.getInt(pinKey + "_mode", -1);
        
        if (mode != -1) {
            _pinConfigs[i].mode = mode;
            _pinConfigs[i].pwmChannel = gpioSettings.getInt(pinKey + "_pwm", -1);
            _pinConfigs[i].frequency = gpioSettings.getInt(pinKey + "_freq", 1000);
            _pinConfigs[i].isConfigured = true;
        }
    }
    
    Serial.println("📖 [GPIOManager] GPIO configuration loaded from NVS");
}

void GPIOManager::saveGPIOConfig() {
    Settings gpioSettings("gpio", true);
    
    // Save pin configurations
    for (int i = 0; i < 40; i++) {
        if (_pinConfigs[i].isConfigured) {
            String pinKey = "pin_" + String(i);
            gpioSettings.setInt(pinKey + "_mode", _pinConfigs[i].mode);
            gpioSettings.setInt(pinKey + "_pwm", _pinConfigs[i].pwmChannel);
            gpioSettings.setInt(pinKey + "_freq", _pinConfigs[i].frequency);
        }
    }
    
    Serial.println("💾 [GPIOManager] GPIO configuration saved to NVS");
}

// void GPIOManager::readSensors() {
//     _sensorData = "{";
    
//     // Read digital inputs
//     addSensorData("pin_2_digital", readDigital(2));
//     addSensorData("pin_4_digital", readDigital(4));
    
//     // Read analog inputs
//     addSensorData("pin_36_analog", readAnalog(36));
//     addSensorData("pin_39_analog", readAnalog(39));
    
//     // Add system info
//     addSensorData("uptime", millis() / 1000);
//     addSensorData("free_heap", ESP.getFreeHeap());
//     addSensorData("temperature", temperatureRead());
    
//     _sensorData += "}";
// }

// void GPIOManager::sendSensorData() {
//     // This will be called from the MQTT thread
//     Serial.printf("📊 [GPIOManager] Sensor data: %s\n", _sensorData.c_str());
// }

// Hàm helper để kiểm tra pin có hỗ trợ ADC không
bool GPIOManager::isValidAnalogPin(int pin) {
    // ADC1 channels (luôn hoạt động, không bị ảnh hưởng bởi WiFi)
    if (pin == 32 || pin == 33 || pin == 34 || pin == 35 || pin == 36 || pin == 39) {
        return true;
    }
    // ADC2 channels (có thể bị ảnh hưởng khi WiFi đang hoạt động)
    if (pin == 0 || pin == 2 || pin == 4 || pin == 12 || pin == 13 || 
        pin == 14 || pin == 15 || pin == 25 || pin == 26 || pin == 27) {
        return true;
    }
    return false;
}
