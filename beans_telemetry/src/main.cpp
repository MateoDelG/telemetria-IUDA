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

const char *SSID = "Delga";
const char *PASS = "Delga1213";
const char *UBIDOTS_TOKEN = "BBUS-k2DesIYqrGRk5133NrNl748KpgD6Nv";
const char *DEVICE_LABEL = "beans-001";

#define dht_indoor_PIN 32
#define dht_outdoor_PIN 33

#define SENSOR_ENABLE 39

#define MODE_PIN 34

// Crea tres sensores con diferentes direcciones I2C
TSL2561Manager tslSensor(TSL2561_ADDR_FLOAT, 0x39); // 0x29
VEML7700Manager vemlSensor;
BH1750Manager bh;
/////////////////////////

DHT21Manager dht_indoor(dht_indoor_PIN);
DHT21Manager dht_outdoor(dht_outdoor_PIN);

UbidotsManager ubidots(UBIDOTS_TOKEN, SSID, PASS, DEVICE_LABEL, 60000);

TimeManager timeManager("pool.ntp.org", -5 * 3600); // Colombia GMT-5

WiFiPortalManager wifiManager("Beans_telemetry", "12345678", MODE_PIN);
RemoteAccessManager remoteManager("Beans_telemetry");

SDLogger logger;

DebugLeds debugLeds;

void watchdogUpdate();
void readLuxSensors();
void setupLuxSensors();
void setupDHTSensors();
void readDHTSensors();
void updateData();
void LEDDebug();

void setup()
{
  Serial.begin(115200);

  // delay(3000);

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
  setupLuxSensors();
  setupDHTSensors();
  debugLeds.setColor(0, 100, 100, 0);
  ubidots.begin();
  timeManager.begin();
  debugLeds.setColor(0, 0, 100, 0);
}

void loop()
{

  updateData();
  ubidots.update();
  wifiManager.loop();
  remoteManager.handle();   // <- primero maneja OTA y Telnet
  watchdogUpdate();
  LEDDebug();

  // delay(30000);
  // readLuxSensors();
  // readDHTSensors();
  // delay(1000);
  // digitalWrite(SENSOR_ENABLE, HIGH);  // Desactivar sensores
  // delay(1000);  // Esperar un segundo para estabilizar
  // digitalWrite(SENSOR_ENABLE, LOW);  // Activar sensores
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
  dht_indoor.begin();
  dht_outdoor.begin();
}

void readDHTSensors()
{
  // Leer y guardar directamente
  sensorData.setTemperatureIndoor(dht_indoor.readTemperature());
  sensorData.setHumidityIndoor(dht_indoor.readHumidity());

  sensorData.setTemperatureOutdoor(dht_outdoor.readTemperature());
  sensorData.setHumidityOutdoor(dht_outdoor.readHumidity());

  // Mostrar datos usando getters
  // Serial.println("Sensor Indoor:");
  remoteManager.log("Sensor Indoor:");
  if (sensorData.getTemperatureIndoor() > -100.0)
  {
    // Serial.print("  Temperatura: ");
    // Serial.print(sensorData.getTemperatureIndoor());
    // Serial.println(" °C");
    remoteManager.log("  Temperatura Indoor: " + String(sensorData.getTemperatureIndoor()) + " °C");
  }
  else
  {
    // Serial.println("  Error al leer temperatura");
    remoteManager.log("  Error al leer temperatura Indoor");
  }

  if (sensorData.getHumidityIndoor() >= 0)
  {
    // Serial.print("  Humedad: ");
    // Serial.print(sensorData.getHumidityIndoor());
    // Serial.println(" %");
    remoteManager.log("  Humedad Indoor: " + String(sensorData.getHumidityIndoor()) + " %");
  }
  else
  {
    // Serial.println("  Error al leer humedad");
    remoteManager.log("  Error al leer humedad Indoor");
  }

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
