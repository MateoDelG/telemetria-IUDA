#ifndef SENSORDATA_H
#define SENSORDATA_H

class SensorData {
  public:
    // Setters
    void setTemperatureIndoor(float temp);
    void setHumidityIndoor(float hum);
    void setTemperatureOutdoor(float temp);
    void setHumidityOutdoor(float hum);

    void setLux1(float lux);
    void setLux2(float lux);
    void setLux3(float lux);

    // Getters
    float getTemperatureIndoor() const;
    float getHumidityIndoor() const;
    float getTemperatureOutdoor() const;
    float getHumidityOutdoor() const;

    float getLux1() const;
    float getLux2() const;
    float getLux3() const;

  private:
    float temperatureIndoor, humidityIndoor;
    float temperatureOutdoor, humidityOutdoor;
    float lux1, lux2, lux3;
};

// Instancia global para acceso compartido
extern SensorData sensorData;

#endif
