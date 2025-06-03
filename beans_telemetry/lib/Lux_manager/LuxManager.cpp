#include "LuxManager.h"


// ----------------------
// TSL2561 Manager
// ----------------------

TSL2561Manager::TSL2561Manager(uint8_t i2c_address, int sensor_id) : tsl(i2c_address, sensor_id) {}

bool TSL2561Manager::begin() {
  return tsl.begin();
}

void TSL2561Manager::displaySensorDetails() {
  sensor_t sensor;
  tsl.getSensor(&sensor);
  Serial.println("------------------------------------");
  Serial.print("Sensor:       "); Serial.println(sensor.name);
  Serial.print("Driver Ver:   "); Serial.println(sensor.version);
  Serial.print("Unique ID:    "); Serial.println(sensor.sensor_id);
  Serial.print("Max Value:    "); Serial.print(sensor.max_value); Serial.println(" lux");
  Serial.print("Min Value:    "); Serial.print(sensor.min_value); Serial.println(" lux");
  Serial.print("Resolution:   "); Serial.print(sensor.resolution); Serial.println(" lux");
  Serial.println("------------------------------------");
  Serial.println("");
  delay(500);
}

void TSL2561Manager::configureSensor() {
  tsl.enableAutoRange(true);
  tsl.setIntegrationTime(TSL2561_INTEGRATIONTIME_13MS);
  tsl.setGain(TSL2561_GAIN_1X);


  Serial.println("------------------------------------");
  Serial.print("Gain:         "); Serial.println("Auto");
  Serial.print("Timing:       "); Serial.println("13 ms");
  Serial.println("------------------------------------");
}

float TSL2561Manager::readLux() {
  sensors_event_t event;
  tsl.getEvent(&event);
  return event.light ? event.light : -1.0;  // Devuelve -1 si hay saturación
}



// ----------------------
// VEML7700 Manager
// ----------------------

VEML7700Manager::VEML7700Manager() {}

bool VEML7700Manager::begin() {
  return veml.begin();
}

void VEML7700Manager::configureSensor() {
  veml.setGain(VEML7700_GAIN_1);               // Ganancia x1
  veml.setIntegrationTime(VEML7700_IT_25MS);  // Tiempo de integración
  veml.setLowThreshold(0);                     // Umbral bajo (opcional)
  veml.setHighThreshold(1000);                 // Umbral alto (opcional)
  veml.interruptEnable(false);                 // Sin interrupciones

  Serial.println("------------------------------------");
  Serial.println("VEML7700 configurado: ganancia x1, integración 100 ms");
  Serial.println("------------------------------------");
}

float VEML7700Manager::readLux() {
  return veml.readLux();
}


// ----------------------
// BH1750 Manager
// ----------------------

BH1750Manager::BH1750Manager() : bh1750(0x23) {}  // Dirección por defecto

bool BH1750Manager::begin() {
  return bh1750.begin(BH1750::CONTINUOUS_HIGH_RES_MODE);
}

void BH1750Manager::configureSensor() {
  bh1750.configure(BH1750::CONTINUOUS_HIGH_RES_MODE);  // Resolución alta continua

  Serial.println("------------------------------------");
  Serial.println("BH1750 configurado: modo continuo alta resolución");
  Serial.println("------------------------------------");
}

float BH1750Manager::readLux() {
  float lux = bh1750.readLightLevel();
  return (lux < 0) ? -1.0 : lux;
}

