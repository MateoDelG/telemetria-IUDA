#include <Arduino.h>
#include "test_board.h"
#include "WiFiPortalManager.h"
#include "pH_manager.h"
#include <globals.h>

TaskHandle_t TaskCore0;
TaskHandle_t TaskCore1;


// #define TELNET_HOSTNAME "ph-remote"
WiFiPortalManager wifiManager(TELNET_HOSTNAME, "12345678", SW_DOWN);
// RemoteAccessManager remoteManager(TELNET_HOSTNAME);
// DS18B20Manager thermo(TEMP_SENSOR, 12);
PHManager ph(&ads, /*channel=*/0, /*avgSamples=*/8);


void initDualCore();
void initHardware();
void initWiFi();
void initThermo();
void initLCD();
void initADS();
void initPH();
float readThermo();
void readADS();
void readPH();

void setup() {
  Serial.begin(115200);
  initHardware();
  initDualCore();
  initWiFi();
  initThermo();
  initLCD();
  initADS();
  initPH();
}

void loop() {
  delay(100);
}

void taskCore0(void *pvParameters) {
  for (;;) {
    // TestBoard::testPumps(PUMP_1, PUMP_2, PUMP_3, PUMP_4, MIXER);
    // readThermo();
    TestBoard::testButtons();
    readADS();
    readPH();
    delay(1000);
  }
}
//loop
void taskCore1(void *pvParameters) {
  for (;;) {
    wifiManager.loop();
    remoteManager.handle();
  }
}

void initDualCore() {
  // Crear tarea para Core 0
  xTaskCreatePinnedToCore(
    taskCore0,        // función
    "TaskCore0",      // nombre
    4096,             // stack size en palabras
    NULL,             // parámetros
    1,                // prioridad
    &TaskCore0,       // handle
    0                 // core (0 ó 1)
  );

  // Crear tarea para Core 1
  xTaskCreatePinnedToCore(
    taskCore1,
    "TaskCore1",
    4096,
    NULL,
    1,
    &TaskCore1,
    1                 // core 1
  );
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

void initLCD(){
  lcd.begin();
  lcd.setBacklight(true);
  lcd.clear();

  lcd.centerPrint(0, "IUDigital");
  lcd.centerPrint(1, "de Antioquia");
  delay(2000);
  lcd.clear();
  lcd.centerPrint(0, "ph - O2 metter");
  lcd.centerPrint(1, "v1.0");
  delay(2000);
}

void initADS() {
  if (!ads.begin()) {
    remoteManager.log(String("ADS1115 no responde: ") + ads.lastError());
    return;
  }
  // Rango y tasa recomendados (tu versión usa uint16_t en set/getDataRate)
  ads.setGain(GAIN_ONE);                   // ±4.096V (1 LSB ≈ 0.125 mV)
  ads.setDataRate(RATE_ADS1115_250SPS);    // RATE_ADS1115_250SPS 250 SPS (uint16_t)
  ads.setAveraging(4);                     // suaviza ruido promediando 4 lecturas
  ads.setCalibration(1.281f, -0.0075f);     

  remoteManager.log("ADS1115 listo (GAIN_ONE, 250SPS)");
}

void initPH(){
  ph.begin();
  // --- Calibración de 2 puntos (a 25 °C, por ejemplo) ---
// Mide y anota el voltaje en buffer pH 7.00 y pH 4.00:
  float V7 =  0.719307;  // <-- ejemplo, mide de verdad con ads.readSingle(0, V7)
  float V4 = 1.7748;  // <-- ejemplo
  ph.setTwoPointCalibration(7.00, V7, 4.00, V4, /*tCalC=*/28.81);
}

void readADS() {
  // Lee A0..A3 single-ended
  // for (uint8_t ch = 0; ch < 4; ch++) {
  //   float v;
  //   if (ads.readSingle(ch, v)) {
  //     remoteManager.log("A" + String(ch) + ": " + String(v, 6) + " V");
  //   } else {
  //     remoteManager.log(String("ADS err A") + ch + ": " + ads.lastError());
  //   }
  // }
  float v;
  if (ads.readSingle(0, v)) {
        remoteManager.log("A0" ": " + String(v, 6) + " V");
      } else {
        remoteManager.log(String("ADS err A0")  + ": " + ads.lastError());
      }
  }

float readThermo(){
   if (thermo.sensorCount() == 0) {
    // Serial.println("Sin sensores");
    remoteManager.log("Sin sensores");
    // delay(1000);
    return -1;
  }
  float c = thermo.readC(0);
  if (isnan(c)) {
    // Serial.println("Lectura inválida");
    remoteManager.log("Lectura inválida");
    return -1;
  } else {
    // Serial.printf("T0: %.2f °C\n", c);
    remoteManager.log("T: " + String(c) + " °C");
    return c;
  }
  // delay(1000);
}

void readPH(){
float tC = readThermo();  
float phValue, volts;
if (ph.readPH(tC, phValue, &volts)) {
  remoteManager.log("pH = " + String(phValue, 3) + "   V = " + String(volts, 4));
} else {
  remoteManager.log(String("pH ERR: ") + ph.lastError());
}
}

