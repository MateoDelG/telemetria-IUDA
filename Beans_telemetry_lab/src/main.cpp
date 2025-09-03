#include <Arduino.h>
#include <esp_task_wdt.h>
#include "LuxManager.h"
#include "DHT21Manager.h"
#include "SensorData.h"
#include "UbidotsManager.h"
#include "TimeManager.h"
#include "WiFiPortalManager.h"
#include "SDLogger.h"
#include "Alerts_manager.h"
#include "LCD_manager.h"
#include <globals.h>

const char *SSID = "Delga";
const char *PASS = "Delga1213";
const char *UBIDOTS_TOKEN = "BBUS-k2DesIYqrGRk5133NrNl748KpgD6Nv";
const char *DEVICE_LABEL = "beans_lab_001";
#define TELNET_HOSTNAME "Beans_telemetry"


#define dht_indoor_PIN 33
#define dht_outdoor_PIN 32


#define MODE_PIN 36
#define BTN_A  34 
#define BTN_B  35

// Crea tres sensores con diferentes direcciones I2C
TSL2561Manager tslSensor(TSL2561_ADDR_FLOAT, 0x39); // 0x29
VEML7700Manager vemlSensor;
BH1750Manager bh;
/////////////////////////

DHT21Manager dht_indoor(dht_indoor_PIN);
DHT21Manager dht_outdoor(dht_outdoor_PIN);

UbidotsManager ubidots(UBIDOTS_TOKEN, SSID, PASS, DEVICE_LABEL, 60000);

TimeManager timeManager("pool.ntp.org", -5 * 3600); // Colombia GMT-5

WiFiPortalManager wifiManager("Beans_telemetry", "12345678", BTN_A);
RemoteAccessManager remoteManager(TELNET_HOSTNAME);

SDLogger logger;

DebugLeds debugLeds;

Lcd16x2 lcd(0x27, 16, 2);

void watchdogUpdate();
void readLuxSensors();
void setupLuxSensors();
void setupDHTSensors();
void readDHTSensors();
void updateData();
void LEDDebug();
void setupLCD();
void uiSetup();
void renderPageSimple(int page);
void uiUpdate();
void setup()
{
  Serial.begin(115200);
  uiSetup();
  setupLCD();

  // delay(3000);
  lcd.clear();
  lcd.centerPrint(0, "Iniciando");
  lcd.centerPrint(1, "WIFI");
  debugLeds.begin();
  debugLeds.setColor(0, 0, 255, 0); // LED 0 - Rojo
  delay(300);
  // Prueba inicial: LED 0 en rojo sólido
  debugLeds.setColor(0, 255, 0, 0); // LED 0 - Rojo
  // logger.begin();

  wifiManager.begin();
  if (wifiManager.isConnected())
  {
    remoteManager.begin(); // Solo si hay WiFi
  }

  debugLeds.setColor(0, 255, 100, 0);

  esp_task_wdt_init(20, true); // en segundos
  esp_task_wdt_add(NULL);

  lcd.clear();
  lcd.centerPrint(0, "Iniciando");
  lcd.centerPrint(1, "sensores");
  setupLuxSensors();
  setupDHTSensors();
  debugLeds.setColor(0, 100, 100, 0);
  lcd.clear();
  lcd.centerPrint(0, "Conectando al");
  lcd.centerPrint(1, "servidor");
  ubidots.begin();
  timeManager.begin();
  debugLeds.setColor(0, 0, 100, 0);

  lcd.clear();
  lcd.centerPrint(0, "Inicializacion");
  lcd.centerPrint(1, "completa");
  delay(2000);
}

void loop()
{

  updateData();
  ubidots.update();
  wifiManager.loop();
  remoteManager.handle();   // <- primero maneja OTA y Telnet
  watchdogUpdate();
  LEDDebug();
  uiUpdate();
}

void watchdogUpdate()
{
  static unsigned long last_Check = 0;
  esp_task_wdt_reset(); // Alimentar el WDT
  if (millis() - last_Check > 10000)
  {
    // Serial.println("Alimentando el WDT...");
    remoteManager.log("Alimentando el WDT...");
    last_Check = millis();

    if (WiFi.status() != WL_CONNECTED)
    {
      // Serial.println("⚠️ WiFi desconectado, reiniciando...");
      remoteManager.log("⚠️ WiFi desconectado, reiniciando...");
      ESP.restart();
    }
  }
}

void setupLuxSensors()
{
  Wire.begin();

  if (!tslSensor.begin())
  {
    // Serial.println("Error al iniciar tslSensor");
    remoteManager.log("Error al iniciar tslSensor");
  }
  else
  {
    tslSensor.displaySensorDetails();
    tslSensor.configureSensor();
  }

  if (!vemlSensor.begin())
  {
    // Serial.println("Error al iniciar vemlSensor");  
    remoteManager.log("Error al iniciar vemlSensor");
  }
  else
  {
    vemlSensor.configureSensor();
  }

  if (!bh.begin())
  {
    // Serial.println("Error al iniciar bh");
    remoteManager.log("Error al iniciar bh");
  }
  else
  {
    bh.configureSensor();
  }
}

void readLuxSensors()
{
  float value;

  value = tslSensor.readLux();
  if (value >= 0)
  {
    sensorData.setLux1(value);
  }
  else
  {
    sensorData.setLux1(-1); // Valor inválido para representar saturación
  }

  value = vemlSensor.readLux();
  if (value >= 0)
  {
    sensorData.setLux2(value);
  }
  else
  {
    sensorData.setLux2(-1); // Valor inválido para representar saturación
  }

  value = bh.readLux();
  if (value >= 0)
  {
    sensorData.setLux3(value);
  }
  else
  {
    sensorData.setLux3(-1); // Valor inválido para representar saturación
  }

  // Mostrar resultados desde SensorData
  // Serial.print("Sensor tsl: ");
  remoteManager.log("Sensor tsl:");
  if (sensorData.getLux1() >= 0)
    // Serial.print(sensorData.getLux1());
    remoteManager.log("  Lux TSL: " + String(sensorData.getLux1()));
  else
    // Serial.print("Saturado");
  // Serial.println(" lux");
    remoteManager.log("  Lux TSL: Saturado");

  // Serial.print("Sensor veml: ");
  remoteManager.log("Sensor veml:");
  if (sensorData.getLux2() >= 0)
    // Serial.print(sensorData.getLux2());
    remoteManager.log("  Lux VEML: " + String(sensorData.getLux2()));
  else
    // Serial.print("Saturado");
  // Serial.println(" lux");
    remoteManager.log("  Lux VEML: Saturado");

  // Serial.print("Sensor bh: ");
  remoteManager.log("Sensor bh:");
  if (sensorData.getLux3() >= 0)
    // Serial.print(sensorData.getLux3());
    remoteManager.log("  Lux BH: " + String(sensorData.getLux3()));
  else
    // Serial.print("Saturado");
  // Serial.println(" lux");
    remoteManager.log("  Lux BH: Saturado");

  // Serial.println("----------------------------");
  remoteManager.log("----------------------------");
}

void setupDHTSensors()
{
  // delay(1000); // Esperar un segundo para estabilizar
  // dht_indoor.begin();
  // delay(1000); // Esperar un segundo para estabilizar
  dht_outdoor.begin();

}

void readDHTSensors()
{
  // Leer y guardar directamente
  // sensorData.setTemperatureIndoor(dht_indoor.readTemperature());
  // sensorData.setHumidityIndoor(dht_indoor.readHumidity());

  sensorData.setTemperatureOutdoor(dht_outdoor.readTemperature());
  sensorData.setHumidityOutdoor(dht_outdoor.readHumidity());

  // // Mostrar datos usando getters
  // // Serial.println("Sensor Indoor:");
  // remoteManager.log("Sensor Indoor:");
  // if (sensorData.getTemperatureIndoor() > -100.0)
  // {
  //   // Serial.print("  Temperatura: ");
  //   // Serial.print(sensorData.getTemperatureIndoor());
  //   // Serial.println(" °C");
  //   remoteManager.log("  Temperatura Indoor: " + String(sensorData.getTemperatureIndoor()) + " °C");
  // }
  // else
  // {
  //   // Serial.println("  Error al leer temperatura");
  //   remoteManager.log("  Error al leer temperatura Indoor");
  // }

  // if (sensorData.getHumidityIndoor() >= 0)
  // {
  //   // Serial.print("  Humedad: ");
  //   // Serial.print(sensorData.getHumidityIndoor());
  //   // Serial.println(" %");
  //   remoteManager.log("  Humedad Indoor: " + String(sensorData.getHumidityIndoor()) + " %");
  // }
  // else
  // {
  //   // Serial.println("  Error al leer humedad");
  //   remoteManager.log("  Error al leer humedad Indoor");
  // }

  Serial.println("Sensor Outdoor:");
  if (sensorData.getTemperatureOutdoor() > -100.0)
  {
    // Serial.print("  Temperatura: ");
    // Serial.print(sensorData.getTemperatureOutdoor());
    // Serial.println(" °C");
    remoteManager.log("  Temperatura Outdoor: " + String(sensorData.getTemperatureOutdoor()) + " °C");
  }
  else
  {
    // Serial.println("  Error al leer temperatura");
    remoteManager.log("  Error al leer temperatura Outdoor");
  }

  if (sensorData.getHumidityOutdoor() >= 0)
  {
    // Serial.print("  Humedad: ");
    // Serial.print(sensorData.getHumidityOutdoor());
    // Serial.println(" %");
    remoteManager.log("  Humedad Outdoor: " + String(sensorData.getHumidityOutdoor()) + " %");
  }
  else
  {
    // Serial.println("  Error al leer humedad");
    remoteManager.log("  Error al leer humedad Outdoor");
  }

  // Serial.println("----------------------------------");
  remoteManager.log("----------------------------------");
}

void stopLuxSensors()
{
  Wire.end();
  // Serial.println("I2C detenido");
  remoteManager.log("I2C detenido");
}

void updateData()
{
  static int update_time = 5000;
  static unsigned long int current_time = millis() + update_time;

  if (millis() - current_time >= update_time)
  {
    readLuxSensors();
    readDHTSensors();
    String timestamp = timeManager.getDateTime();
    // logger.logSensorData(timestamp);
    // Serial.println("Fecha y hora: " + timestamp);
    remoteManager.log("Fecha y hora: " + timestamp);
    current_time = millis();
  }
}

void LEDDebug()
{
  debugLeds.update();

  static unsigned long lastFade = 0;
  static const uint16_t fadeDuration = 1000;
  static bool fadingOut = false;

  // Verifica si terminó el último fade
  if (millis() - lastFade > fadeDuration)
  {
    if (fadingOut)
    {
      debugLeds.setFade(1, 0, 0, 255, fadeDuration); // Azul
    }
    else
    {
      debugLeds.setFade(1, 0, 0, 0, fadeDuration); // Negro
    }
    fadingOut = !fadingOut;
    lastFade = millis();
  }
}

void setupLCD(){
  lcd.begin();
  lcd.setBacklight(true);
  lcd.clear();

  lcd.centerPrint(0, "IUDigital");
  lcd.centerPrint(1, "de Antioquia");
  delay(2000);
  lcd.clear();
  lcd.centerPrint(0, "Beans Telemetry");
  lcd.centerPrint(1, "LAB v1.0");
  delay(2000);
  lcd.clear();
}

void uiSetup() {
  pinMode(BTN_A, INPUT);
  pinMode(BTN_B, INPUT);
}

void renderPageSimple(int page) {
  // Datos fijos (nombre, valor, unidad)
   struct Row { String name; String value; String unit; };

  // Define pantallas aquí (cada case muestra 1 ó 2 filas simples)
  switch (page) {
    case 0: { // TSL
      lcd.clear();
      lcd.centerPrint(0, "TSL2561");
      lcd.centerPrint(1, String(sensorData.getLux1()) + String(" lux"));
    } break;

    case 1: { // VEML
      lcd.clear();
      lcd.centerPrint(0, "VEML7700");
      lcd.centerPrint(1, String(sensorData.getLux2()) + String(" lux"));
    } break;

    case 2: { // BH
      lcd.clear();
      lcd.centerPrint(0, "BH1750");
      lcd.centerPrint(1, String(sensorData.getLux3()) + String(" lux"));
    } break;

    case 3: { // Temp Outdoor
      lcd.clear();
      lcd.centerPrint(0, "DHT22 - Temp");
      lcd.centerPrint(1, String(sensorData.getTemperatureOutdoor()) + (char)223 + String("C"));
    } break;

    case 4: { // Hum Outdoor
      lcd.clear();
      lcd.centerPrint(0, "DHT22 - Hum");
      lcd.centerPrint(1, String(sensorData.getHumidityOutdoor()) + String("%"));
    } break;

    case 5: { // Date
      lcd.clear();
      lcd.centerPrint(0, "Fecha - Hora");
      lcd.centerPrint(1, String(timeManager.getDateTime()));
    } break;

    case 6: { // TELNET
      lcd.clear();
      lcd.centerPrint(0, "Telnet host");
      lcd.centerPrint(1, TELNET_HOSTNAME);
    } break;

    default: {
      lcd.clear();
      lcd.centerPrint(0, "Sin pantalla");
      lcd.centerPrint(1, String(page));
    } break;
  }
}

void uiUpdate() {
  // Estado persistente, pero encapsulado aquí
  static int currentPage = 0;
  static const int numPages = 7;             // actualiza si agregas más cases
  static unsigned long lastRedraw = 0;
  static bool btnA_last = HIGH, btnB_last = HIGH;
  static unsigned long btnA_tlast = 0, btnB_tlast = 0;

  const unsigned long UI_REDRAW_MS = 2000;    // refresco suave
  const unsigned long DEBOUNCE_MS  = 40;

  unsigned long now = millis();

  // Botón A (siguiente)
  bool a_read = digitalRead(BTN_A);
  if (a_read != btnA_last) { btnA_last = a_read; btnA_tlast = now; }
  else if ((now - btnA_tlast) > DEBOUNCE_MS && a_read == LOW) {
    currentPage = (currentPage + 1) % numPages;
    lastRedraw = 0; // fuerza redibujo inmediato
  }

  // Botón B (anterior)
  bool b_read = digitalRead(BTN_B);
  if (b_read != btnB_last) { btnB_last = b_read; btnB_tlast = now; }
  else if ((now - btnB_tlast) > DEBOUNCE_MS && b_read == LOW) {
    currentPage = (currentPage - 1 + numPages) % numPages;
    lastRedraw = 0; // fuerza redibujo inmediato
  }

  // Redibujar
  if (now - lastRedraw >= UI_REDRAW_MS) {
    lastRedraw = now;
    renderPageSimple(currentPage);
  }

  // Si usas efectos no bloqueantes del LCD
  lcd.update();
}