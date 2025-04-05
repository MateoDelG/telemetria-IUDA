#ifndef TSL2561MANAGER_H
#define TSL2561MANAGER_H

#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_TSL2561_U.h>

class TSL2561Manager {
  public:
    TSL2561Manager(uint8_t i2c_address, int sensor_id);
    bool begin();
    void displaySensorDetails();
    void configureSensor();
    float readLux();

    static void scanI2CDevices();  // Escanea dispositivos I2C

  private:
    Adafruit_TSL2561_Unified tsl;
};

#endif
