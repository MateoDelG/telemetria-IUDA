#include "UbidotsManager.h"
#include "SensorData.h"

UbidotsManager::UbidotsManager(const char* token, const char* ssid, const char* pass, const char* deviceLabel, unsigned long interval)
  : _token(token), _ssid(ssid), _pass(pass), _deviceLabel(deviceLabel), _interval(interval), _ubidots(token) {}

void UbidotsManager::begin() {
  // _ubidots.connectToWifi(_ssid, _pass);
  _ubidots.setCallback(callback);
  _ubidots.setup();
  reconnect();
  _lastTime = millis();
}

void UbidotsManager::update() {
  if (!_ubidots.connected()) {
    reconnect();
  }

  _ubidots.loop();

  if (millis() - _lastTime >= _interval) {
    sendData();
    _lastTime = millis();
  }
}

void UbidotsManager::reconnect() {
  _ubidots.reconnect();
}

void UbidotsManager::sendData() {
  static bool sendFirstGroup = true;

  if (sendFirstGroup) {
    _ubidots.add("temperature_indoor", sensorData.getTemperatureIndoor());
    _ubidots.add("humidity_indoor", sensorData.getHumidityIndoor());
    _ubidots.add("temperature_outdoor", sensorData.getTemperatureOutdoor());
    _ubidots.add("humidity_outdoor", sensorData.getHumidityOutdoor());
    Serial.println("Enviando primer grupo...");
  } else {
    _ubidots.add("lux1", sensorData.getLux1());
    _ubidots.add("lux2", sensorData.getLux2());
    _ubidots.add("lux3", sensorData.getLux3());
    Serial.println("Enviando segundo grupo...");
  }


  bool success = _ubidots.publish(_deviceLabel);
  if (success) {
    Serial.println("✔️ Datos enviados a Ubidots (MQTT)");
  } else {
    Serial.println("❌ Error al publicar en Ubidots");
  }

  sendFirstGroup = !sendFirstGroup;  // alternar para el siguiente envío
}

void UbidotsManager::callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Mensaje recibido [");
  Serial.print(topic);
  Serial.print("]: ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}
