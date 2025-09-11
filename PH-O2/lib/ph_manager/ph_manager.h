#ifndef PH_MANAGER_H
#define PH_MANAGER_H

#include <Arduino.h>
#include "ADS1115_manager.h" // Tu wrapper del ADS1115 (uint16_t data rate)

// Librería para manejar un sensor de pH tipo Gravity (salida analógica).
// La conversión de pH se hace con una calibración lineal de 2 puntos
// y ajuste de pendiente por temperatura usando la ecuación de Nernst.
//
// Modelo lineal a temperatura de calibración (Tcal):
//   pH = a_cal * V + b_cal
// Para temperatura medida T:
//   slope_T = slope_cal * (T_K / Tcal_K)      // Nernst scaling
//   pH(T)   = pH_cal + (pH(Tcal) - pH_cal) * (T_K / Tcal_K)  --> equivalente a
//   pH(T)   = (a_cal * (T_K / Tcal_K)) * V + b_cal'
// (Aquí implementamos una forma estable: convertimos con a_cal/b_cal y luego
//  reescalamos la PENDIENTE con T_K/Tcal_K manteniendo el offset en pH7.)

class PHManager {
public:
  // ctor: necesita un puntero al ADS y el canal analógico donde está el módulo
  //        (A0..A3 -> 0..3). 'avgSamples' controla el promedio simple.
  explicit PHManager(ADS1115Manager* ads,
                     uint8_t ads_channel,
                     uint8_t avgSamples = 8);

  // Inicialización (no hace I2C, eso ya lo maneja ads->begin())
  bool begin();

  // Lectura con compensación de temperatura. 'tempC' puede venir de tu DS18B20.
  // Devuelve true si OK y deja 'ph' y 'volts' (voltaje promediado y calibrado solo por el ADS).
  bool readPH(float tempC, float& ph, float* volts = nullptr);

  // --- Calibración ---
  // Carga una calibración de 2 puntos realizada a 'tCalC' (°C).
  // Ej.: setTwoPointCalibration(7.00, V7, 4.00, V4, 25.0);
  void setTwoPointCalibration(float pH1, float V1, float pH2, float V2, float tCalC);

  // Carga/lee calibración directa (por si guardas en NVS/EEPROM).
  void setCalibrationCoeffs(float a_cal, float b_cal, float tCalC);
  void getCalibrationCoeffs(float& a_cal, float& b_cal, float& tCalC) const;

  // Configuraciones varias
  void setAveraging(uint8_t n);        // n>=1
  void setChannel(uint8_t ch);         // 0..3

  // Últimos valores
  float lastPH()     const { return last_ph_;    }
  float lastVolts()  const { return last_volts_; }
  const char* lastError() const { return last_error_; }

private:
  ADS1115Manager* ads_ = nullptr;
  uint8_t ch_ = 0;
  uint8_t avg_ = 8;

  // Calibración a temperatura Tcal (°C)
  float a_cal_ = -3.5f;   // pendiente aproximada (pH/V) -> ajusta con calibración
  float b_cal_ = 7.0f;    // offset aproximado (pH @ V≈0)  -> ajusta con calibración
  float tcal_c_ = 25.0f;  // °C de la calibración (por defecto 25°C)

  // Últimos resultados
  float last_ph_ = NAN;
  float last_volts_ = NAN;
  char  last_error_[64] = {0};

  // Helpers
  void  setError_(const char* msg);
  bool  readAveragedVolts_(float& volts);
  float kelvin_(float c) const { return c + 273.15f; }
};

#endif // PH_MANAGER_H
