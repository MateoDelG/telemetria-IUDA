#ifndef GLOBALS_H
#define GLOBALS_H

#include <Arduino.h>

namespace Globals {

  // ---- Lecturas del sensor ----
  void setDistanceRaw(float v);
  float getDistanceRaw();

  void setDistanceFiltered(float v);
  float getDistanceFiltered();

  // ---- Configuración del usuario ----
  void setMinLevel(float v);
  float getMinLevel();

  void setMaxLevel(float v);
  float getMaxLevel();

  void setSamplePeriod(uint32_t ms);
  uint32_t getSamplePeriod();

  uint8_t getI2CAddress();
  void    setI2CAddress(uint8_t addr);


  // Parámetros filtro Kalman
  void setKalMea(float v);
  float getKalMea();

  void setKalEst(float v);
  float getKalEst();

  void setKalQ(float v);
  float getKalQ();


  // ---- Estado AP / Config Mode ----
  void setAPActive(bool v);
  bool isAPActive();

  void setAPClients(uint8_t n);
  uint8_t getAPClients();

  void setConfigStartTime(uint32_t t);
  uint32_t getConfigStartTime();


}

#endif
