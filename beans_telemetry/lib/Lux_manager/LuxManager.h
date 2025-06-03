#ifndef LUXMANAGER_H
#define LUXMANAGER_H

#include <Wire.h>
#include "Adafruit_Sensor.h"
#include "Adafruit_TSL2561_U.h"
#include "Adafruit_VEML7700.h"
#include "BH1750.h"


class TSL2561Manager {
  public:
    TSL2561Manager(uint8_t i2c_address, int sensor_id);
    bool begin();
    void displaySensorDetails();
    void configureSensor();
    float readLux();


  private:
    Adafruit_TSL2561_Unified tsl;
};


class VEML7700Manager {
  public:
    VEML7700Manager();
    bool begin();
    void configureSensor();
    float readLux();
  private:
    Adafruit_VEML7700 veml;
};

class BH1750Manager {
  public:
    BH1750Manager();
    bool begin();
    void configureSensor();
    float readLux();
  private:
    BH1750 bh1750;
};

#endif
