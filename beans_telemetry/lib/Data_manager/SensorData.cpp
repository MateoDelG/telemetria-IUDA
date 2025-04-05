#include "SensorData.h"

// Instancia global
SensorData sensorData;

// Setters
void SensorData::setTemperatureIndoor(float temp)  { temperatureIndoor = temp; }
void SensorData::setHumidityIndoor(float hum)      { humidityIndoor = hum; }
void SensorData::setTemperatureOutdoor(float temp) { temperatureOutdoor = temp; }
void SensorData::setHumidityOutdoor(float hum)     { humidityOutdoor = hum; }

void SensorData::setLux1(float lux) { lux1 = lux; }
void SensorData::setLux2(float lux) { lux2 = lux; }
void SensorData::setLux3(float lux) { lux3 = lux; }

// Getters
float SensorData::getTemperatureIndoor() const  { return temperatureIndoor; }
float SensorData::getHumidityIndoor() const     { return humidityIndoor; }
float SensorData::getTemperatureOutdoor() const { return temperatureOutdoor; }
float SensorData::getHumidityOutdoor() const    { return humidityOutdoor; }

float SensorData::getLux1() const { return lux1; }
float SensorData::getLux2() const { return lux2; }
float SensorData::getLux3() const { return lux3; }
