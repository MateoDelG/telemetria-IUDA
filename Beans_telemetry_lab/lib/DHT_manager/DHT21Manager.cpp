#include "DHT21Manager.h"

DHT21Manager::DHT21Manager(uint8_t pin) 
  : dht(pin, DHT21) {}

void DHT21Manager::begin() {
  dht.begin();

}

float DHT21Manager::readTemperature() {
  float temp = dht.readTemperature();
  if (isnan(temp)) {
    return -1.0; // Error: temperatura no válida
  }
  return temp;
}

float DHT21Manager::readHumidity() {
  float hum = dht.readHumidity();
  if (isnan(hum)) {
    return -1.0; // Error: humedad no válida
  }
  return hum;
}
