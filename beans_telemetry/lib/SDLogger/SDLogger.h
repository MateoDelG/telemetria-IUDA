#ifndef SDLOGGER_H
#define SDLOGGER_H

#include <Arduino.h>
#include <FS.h>
#include <SD.h>
#include <SPI.h>
#include <ArduinoJson.h>
#include "SensorData.h"

class SDLogger {
public:
    SDLogger();
    bool begin();
    void logSensorData(const String& dateTime);

private:
    bool checkSD();
    void writeToFile(const String& jsonData);
    const char* filename = "/datalog.json";
    SPIClass spi;
};

#endif
