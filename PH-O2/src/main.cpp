#include <Arduino.h>
#include "test_board.h"
#include "WiFiPortalManager.h"
#include "globals.h"


#define SW_DOWN 36
#define SW_UP 39
#define SW_OK 35
#define SW_ESC 34
#define SENSOR_LEVEL_H2O 33
#define SENSOR_LEVEL_KCL 32

#define BUZZER 5
#define PUMP_1 13
#define PUMP_2 12
#define PUMP_3 14 
#define PUMP_4 27
#define MIXER 26

#define TEMP_SENSOR 19

// #define TELNET_HOSTNAME "ph-remote"
WiFiPortalManager wifiManager(TELNET_HOSTNAME, "12345678", SW_DOWN);
// RemoteAccessManager remoteManager(TELNET_HOSTNAME);

void initHardware();
void initWiFi();

void setup() {
  Serial.begin(115200);
  initHardware();
  initWiFi();
}

void loop() {
  TestBoard::testPumps(PUMP_1, PUMP_2, PUMP_3, PUMP_4, MIXER);

  wifiManager.loop();
  remoteManager.handle();
  remoteManager.log("Test log message from PH-O2");
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

