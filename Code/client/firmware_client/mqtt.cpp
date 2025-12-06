#include "mqtt.h"
#include <Arduino.h>
#include <WiFiClient.h>
#include <PubSubClient.h>
#include <freertos/semphr.h>
// #define REG_SS 0
// #define REG_CT 1
// #define REG_NC 2
// #define SEND_NC true
// #define SEND_SS_CT false
// Mutex được quản lý bởi module MQTT
SemaphoreHandle_t mqttMutex = NULL;

MQTTProtocol::MQTTProtocol() : _mqttClient(_wifiClient) {}

void MQTTProtocol::begin() {
  // Tạo mutex nếu chưa có
  if (mqttMutex == NULL) {
    mqttMutex = xSemaphoreCreateMutex();
    if (mqttMutex == NULL) {
      Serial.println("❌ [MQTT] Failed to create mutex!");
      return;
    }
    Serial.println("✅ [MQTT] Mutex created successfully");
  }
  
  // Load cấu hình từ NVS
  Settings mqttSettings("mqtt", true);
  
  _broker = mqttSettings.getString("broker", "");
  _port = mqttSettings.getInt("port", 1883);
  _clientId = mqttSettings.getString("clientId", "");
  _topicSS = String("SS/") + _clientId;
  _topicCT = String("CT/") + _clientId;
  _topicNC = String("NC/") + _clientId;

  if (_broker.length() == 0) {
    Serial.println("⚠️ [MQTT] No broker configuration found in NVS!");
    Serial.println("💡 [MQTT] Please set MQTT configuration first using updateConfig()");
    return;
  }
  _mqttClient.setServer(_broker.c_str(), _port);

  Serial.printf("🔧 [MQTT] Loaded config from NVS: host=%s, port=%u\n", _broker.c_str(), _port);
  if (_user.length() > 0)
    Serial.printf("   [MQTT] user=%s\n", _user.c_str());
}

void MQTTProtocol::setCallback(MQTT_CALLBACK_SIGNATURE) {
  _mqttClient.setCallback(callback);
}

void MQTTProtocol::updateConfig(const String& broker, uint16_t port, const String& clientId) {
  // Lưu cấu hình vào NVS
  Settings mqttSettings("mqtt", true);
  _clientId = clientId;
  mqttSettings.setString("broker", broker);
  mqttSettings.setInt("port", port);
  mqttSettings.setString("clientId", clientId);
  // Cập nhật biến thành viên
  _broker = broker;
  _port = port;
  
  // Cập nhật server cho MQTT client
  _mqttClient.setServer(_broker.c_str(), _port);
  
  Serial.printf("✅ [MQTT] Configuration updated and saved to NVS:\n");
  Serial.printf("   [MQTT] Broker: %s:%u\n", _broker.c_str(), _port);
  Serial.printf("   [MQTT] Client ID: %s\n", _clientId.c_str());
}

void MQTTProtocol::loop() {
  if (mqttMutex != NULL && xSemaphoreTake(mqttMutex, portMAX_DELAY)) {
    if (!_mqttClient.connected()) {
      reconnect();
    }
    _mqttClient.loop();
    xSemaphoreGive(mqttMutex);
  }
}

void MQTTProtocol::reconnect() {
  if (_mqttClient.connected()) return;

  Serial.printf("🔄 Reconnecting to MQTT broker %s:%u...\n", _broker.c_str(), _port);

  // Note: reconnect() được gọi từ loop() đã có mutex protection
  while (!_mqttClient.connected()) {
    bool ok = _mqttClient.connect(_clientId.c_str());

    if (ok) {
      Serial.println("✅ MQTT connected!");
      break;
    } else {
      Serial.printf("❌ Failed, rc=%d. Retry in 5s...\n", _mqttClient.state());
      delay(5000);
    }
  }
}

void MQTTProtocol::publish(const char* topic, const String &payload, bool retained) {
  if (mqttMutex != NULL && xSemaphoreTake(mqttMutex, pdMS_TO_TICKS(100))) {
    if (!_mqttClient.connected()) {
      Serial.println("⚠️ MQTT not connected, cannot publish");
      xSemaphoreGive(mqttMutex);
      return;
    }
    _mqttClient.publish(topic, payload.c_str(), retained);
    Serial.printf("📤 Published [%s] => %s\n", topic, payload.c_str());
    xSemaphoreGive(mqttMutex);
  } else {
    Serial.println("⚠️ [MQTT] Could not acquire mutex for publish");
  }
}
void MQTTProtocol::send(int virtualPin, const String &payload , bool retained , bool isnotification) {
  if (mqttMutex != NULL && xSemaphoreTake(mqttMutex, pdMS_TO_TICKS(100))) {
    if (!_mqttClient.connected()) {
      Serial.println("⚠️ MQTT not connected, cannot send");
      xSemaphoreGive(mqttMutex);
      return;
    }
    String topic;
    if(isnotification) {
      topic = _topicNC;
    } else {
      topic = _topicSS + "/" + virtualPin;
    }
    _mqttClient.publish(topic.c_str(), payload.c_str(), retained);
    Serial.printf("📤 Sent [%s] => %s\n", topic.c_str(), payload.c_str());
    xSemaphoreGive(mqttMutex);
  }
  else {
    Serial.println("⚠️ [MQTT] Could not acquire mutex for send");
  }
} 

void MQTTProtocol::subscribe(const char* topic) {
  if (mqttMutex != NULL && xSemaphoreTake(mqttMutex, pdMS_TO_TICKS(100))) {
    if (!_mqttClient.connected()) {
      Serial.println("⚠️ MQTT not connected, cannot subscribe");
      xSemaphoreGive(mqttMutex);
      return;
    }
    _mqttClient.subscribe(topic);
    Serial.printf("📡 Subscribed to: %s\n", topic);
    xSemaphoreGive(mqttMutex);
  } else {
    Serial.println("⚠️ [MQTT] Could not acquire mutex for subscribe");
  }
}
void MQTTProtocol::registerVirtualpin(int type , int virtualPin) {
    if(type == REG_SS) {
      subscribe((String("SS/") + _clientId + "/" + virtualPin).c_str());
    } else if(type == REG_CT) {
      subscribe((String("CT/") + _clientId + "/" + virtualPin).c_str());
    } else if(type == REG_NC) {
      subscribe((String("NC/") + _clientId).c_str());
    }
    else {
      Serial.println("⚠️ [MQTT] Invalid type");
    }
}

bool MQTTProtocol::connected() {
  bool result = false;
  if (mqttMutex != NULL && xSemaphoreTake(mqttMutex, pdMS_TO_TICKS(100))) {
    result = _mqttClient.connected();
    xSemaphoreGive(mqttMutex);
  }
  return result;
}
