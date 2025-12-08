#ifndef MQTT_HANDLER_H
#define MQTT_HANDLER_H
#define REG_SS 0
#define REG_CT 1
#define REG_NC 2
#define SEND_NC true
#define SEND_SS_CT false
#include <Arduino.h>
#include <WiFiClient.h>
#include <PubSubClient.h>
#include "settings.h"

class MQTTProtocol {
public:
  // ======= Singleton Accessor =======
  static MQTTProtocol& getInstance() {
    static MQTTProtocol instance;   // tạo duy nhất 1 lần trong toàn chương trình
    return instance;
  }
  
  // ======= Public API =======
  void begin();  // Tự động load từ NVS
  void setCallback(MQTT_CALLBACK_SIGNATURE);

  void loop();
  bool connected();
  void reconnect();

  void publish(const char* topic, const String &payload, bool retained = false);
  void subscribe(const char* topic);
  void registerVirtualpin(int type , int virtualPin);
  void send(int virtualPin, const String &payload , bool retained , bool isnotification);
  String getBroker() const { return _broker; }
  uint16_t getPort() const { return _port; }
  
  // Thêm phương thức để cập nhật cấu hình
  void updateConfig(const String& broker, uint16_t port, const String& clientId);

  // Cấm copy & gán
  MQTTProtocol(const MQTTProtocol&) = delete;
  MQTTProtocol& operator=(const MQTTProtocol&) = delete;

private:
  // ======= Private constructor =======
  MQTTProtocol();

  WiFiClient _wifiClient;
  PubSubClient _mqttClient;
  String _broker;
  uint16_t _port;
  String _user;
  String _password;
  String _clientId;
  String _topicSS;
  String _topicCT;
  String _topicNC;
};

#endif
