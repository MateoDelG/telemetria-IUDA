#include <Arduino.h>
#include "test_board.h"
#include "WiFiPortalManager.h"
#include "DS18B20_manager.h"
#include <globals.h>




// #define TELNET_HOSTNAME "ph-remote"
WiFiPortalManager wifiManager(TELNET_HOSTNAME, "12345678", SW_DOWN);
// RemoteAccessManager remoteManager(TELNET_HOSTNAME);

DS18B20Manager thermo(TEMP_SENSOR, 12);


void initHardware();
void initWiFi();
void initThermo();
void readThermo();

void setup() {
  Serial.begin(115200);
  initHardware();
  initWiFi();
  initThermo();
}

void loop() {
  // TestBoard::testPumps(PUMP_1, PUMP_2, PUMP_3, PUMP_4, MIXER);
  // readThermo();
  TestBoard::testButtons();

  wifiManager.loop();
  remoteManager.handle();
}

void initHardware() {
  pinMode(SW_DOWN, INPUT);
  pinMode(SW_UP, INPUT);
  pinMode(SW_OK, INPUT);
  pinMode(SW_ESC, INPUT);
  pinMode(SENSOR_LEVEL_H2O, INPUT);
  pinMode(SENSOR_LEVEL_KCL, INPUT);
  pinMode(BUZZER, OUTPUT);
  pinMode(PUMP_1, OUTPUT);
  pinMode(PUMP_2, OUTPUT);
  pinMode(PUMP_3, OUTPUT);
  pinMode(PUMP_4, OUTPUT);
  pinMode(MIXER, OUTPUT);

  digitalWrite(BUZZER, LOW);
  digitalWrite(PUMP_1, LOW);
  digitalWrite(PUMP_2, LOW);
  digitalWrite(PUMP_3, LOW);
  digitalWrite(PUMP_4, LOW);
  digitalWrite(MIXER, LOW);
}

void initWiFi() {
  wifiManager.begin();
  if (wifiManager.isConnected())
  {
    remoteManager.begin(); // Solo si hay WiFi
  }
}

void initThermo(){
  uint8_t n = thermo.begin();
  remoteManager.log("DS18B20 encontrados: " + String(n));
}

void readThermo(){
   if (thermo.sensorCount() == 0) {
    // Serial.println("Sin sensores");
    remoteManager.log("Sin sensores");
    // delay(1000);
    return;
  }
  float c = thermo.readC(0);
  if (isnan(c)) {
    // Serial.println("Lectura inválida");
    remoteManager.log("Lectura inválida");
  } else {
    // Serial.printf("T0: %.2f °C\n", c);
    remoteManager.log("T: " + String(c) + " °C");
  }
  // delay(1000);
}



