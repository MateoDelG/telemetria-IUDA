#include "TSL2561Manager.h"

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

void TSL2561Manager::scanI2CDevices() {
  byte error, address;
  int nDevices = 0;

  Serial.println("Escaneando dispositivos I2C...");
  for(address = 1; address < 127; address++ ) {
    Wire.beginTransmission(address);
    error = Wire.endTransmission();

    if (error == 0) {
      Serial.print("Dispositivo I2C encontrado en dirección 0x");
      if (address<16) Serial.print("0");
      Serial.print(address, HEX);
      Serial.println(" !");
      nDevices++;
    }
  }
  if (nDevices == 0)
    Serial.println("No se encontraron dispositivos I2C\n");
  else
    Serial.println("Escaneo completado\n");
}
