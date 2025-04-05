#include <Arduino.h>
#include "TSL2561Manager.h"
#include "DHT21Manager.h"
#include "SensorData.h"
#include "UbidotsManager.h"
#include "TimeManager.h"

const char* SSID = "Delga";
const char* PASS = "Delga1213";
const char* UBIDOTS_TOKEN = "BBUS-k2DesIYqrGRk5133NrNl748KpgD6Nv";
const char* DEVICE_LABEL = "beans-001";

#define dht_indoor_PIN 19
#define dht_outdoor_PIN 18

// Crea tres sensores con diferentes direcciones I2C
TSL2561Manager sensor_lux_1(TSL2561_ADDR_LOW, 0x29);   // 0x29
TSL2561Manager sensor_lux_2(TSL2561_ADDR_FLOAT,0x39); // 0x39
TSL2561Manager sensor_lux_3(TSL2561_ADDR_HIGH, 0x49);  // 0x49

DHT21Manager dht_indoor(dht_indoor_PIN);
DHT21Manager dht_outdoor(dht_outdoor_PIN);

UbidotsManager ubidots(UBIDOTS_TOKEN, SSID, PASS, DEVICE_LABEL, 120000);  // cada 5 segundos

TimeManager timeManager("pool.ntp.org", -5 * 3600);  // Colombia GMT-5




void readLuxSensors();
void setupLuxSensors();
void setupDHTSensors();
void readDHTSensors();
void updateData();

void setup() {
  Serial.begin(115200);

  setupLuxSensors();
  setupDHTSensors();
  ubidots.begin();
  timeManager.begin();
  delay(1000);

}

void loop() {

  updateData();
  ubidots.update();
  // delay(30000);
}


void setupLuxSensors(){
  Wire.begin();

  if (!sensor_lux_1.begin()) {
    Serial.println("Error al iniciar sensor_lux_1");
  } else {
    sensor_lux_1.displaySensorDetails();
    sensor_lux_1.configureSensor();
  }

  if (!sensor_lux_2.begin()) {
    Serial.println("Error al iniciar sensor_lux_2");
  } else {
    sensor_lux_2.displaySensorDetails();
    sensor_lux_2.configureSensor();
  }

  if (!sensor_lux_3.begin()) {
    Serial.println("Error al iniciar sensor_lux_3");
  } else {
    sensor_lux_3.displaySensorDetails();
    sensor_lux_3.configureSensor();
  }
}
void readLuxSensors() {
  float value;

  value = sensor_lux_1.readLux();
  if (value >= 0) {
    sensorData.setLux1(value);
  } else {
    sensorData.setLux1(-1);  // Valor inválido para representar saturación
  }

  value = sensor_lux_2.readLux();
  if (value >= 0) {
    sensorData.setLux2(value);
  } else {
    sensorData.setLux2(-1);
  }

  value = sensor_lux_3.readLux();
  if (value >= 0) {
    sensorData.setLux3(value);
  } else {
    sensorData.setLux3(-1);
  }

  // Mostrar resultados desde SensorData
  Serial.print("Sensor 1: ");
  if (sensorData.getLux1() >= 0) Serial.print(sensorData.getLux1());
  else Serial.print("Saturado");
  Serial.println(" lux");

  Serial.print("Sensor 2: ");
  if (sensorData.getLux2() >= 0) Serial.print(sensorData.getLux2());
  else Serial.print("Saturado");
  Serial.println(" lux");

  Serial.print("Sensor 3: ");
  if (sensorData.getLux3() >= 0) Serial.print(sensorData.getLux3());
  else Serial.print("Saturado");
  Serial.println(" lux");

  Serial.println("----------------------------");
}
void setupDHTSensors(){
  dht_indoor.begin();
  dht_outdoor.begin();
}
void readDHTSensors(){
  // Leer y guardar directamente
  sensorData.setTemperatureIndoor(dht_indoor.readTemperature());
  sensorData.setHumidityIndoor(dht_indoor.readHumidity());

  sensorData.setTemperatureOutdoor(dht_outdoor.readTemperature());
  sensorData.setHumidityOutdoor(dht_outdoor.readHumidity());

  // Mostrar datos usando getters
  Serial.println("Sensor Indoor:");
  if (sensorData.getTemperatureIndoor() > -100.0) {
    Serial.print("  Temperatura: "); Serial.print(sensorData.getTemperatureIndoor()); Serial.println(" °C");
  } else {
    Serial.println("  Error al leer temperatura");
  }

  if (sensorData.getHumidityIndoor() >= 0) {
    Serial.print("  Humedad: "); Serial.print(sensorData.getHumidityIndoor()); Serial.println(" %");
  } else {
    Serial.println("  Error al leer humedad");
  }

  Serial.println("Sensor Outdoor:");
  if (sensorData.getTemperatureOutdoor() > -100.0) {
    Serial.print("  Temperatura: "); Serial.print(sensorData.getTemperatureOutdoor()); Serial.println(" °C");
  } else {
    Serial.println("  Error al leer temperatura");
  }

  if (sensorData.getHumidityOutdoor() >= 0) {
    Serial.print("  Humedad: "); Serial.print(sensorData.getHumidityOutdoor()); Serial.println(" %");
  } else {
    Serial.println("  Error al leer humedad");
  }

  Serial.println("----------------------------------");
}

void updateData(){
  static int update_time = 2000;
  static unsigned long int current_time = millis();

  if (millis() - current_time >= update_time) {
    readLuxSensors();
    readDHTSensors();
    Serial.println("Fecha y hora: " + timeManager.getDateTime());
    current_time = millis();
  }
  
}