#ifndef DHT21MANAGER_H
#define DHT21MANAGER_H

#include <DHT.h>

class DHT21Manager {
  public:
    DHT21Manager(uint8_t pin);
    void begin();
    float readTemperature();  // Retorna temperatura en °C
    float readHumidity();     // Retorna humedad relativa en %

  private:
    DHT dht;
};

#endif
