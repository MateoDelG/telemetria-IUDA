#pragma once
#include <Arduino.h>
#include <EEPROM.h>

class ConfigStore {
public:
  // Inicializa la EEPROM emulada (ESP32). eepromSize típico: 128–256 bytes.
  bool begin(size_t eepromSize = 128, uint16_t baseAddr = 0);

  // Carga desde EEPROM (valida MAGIC/versión/CRC). Devuelve true si OK.
  // Si detecta versión antigua (v1, solo ADC), migra y guarda automáticamente a v2.
  bool load();

  // Guarda a EEPROM (actualiza CRC). Devuelve true si commit() fue OK.
  bool save();

  // Restaura valores por defecto: ADC = (1.0, 0.0), pH 2pt = NaN
  void resetDefaults();

  // --- ADC ---
  void setADC(float scale, float offset);
  void getADC(float& scale, float& offset) const;

  // --- pH 2 puntos (V7, V4, tC) ---
  void setPH2pt(float V7, float V4, float tCalC);
  void getPH2pt(float& V7, float& V4, float& tCalC) const;

  // Último error de load()/save()
  const char* lastError() const { return _err; }

  // Versión del layout almacenado
  static constexpr uint16_t kVersion = 0x0002;  // v2: añade pH2pt {V7,V4,tC}

private:
  struct Pair { float scale; float offset; };
  struct PH2pt { float V7; float V4; float tC; };

  // Bloque almacenado (v2)
  struct ConfigData {
    uint16_t magic;    // 0xC0AD
    uint16_t version;  // kVersion
    Pair     adc;      // calibración ADC (m,b)
    PH2pt    ph2pt;    // calibración pH (V7, V4, tC)
    uint32_t crc;      // CRC32 de todo lo anterior
  };

  // Helpers
  static uint32_t crc32(const uint8_t* data, size_t len);
  void computeCrc_();

  // Memoria / estado
  size_t     _eepromSize = 0;
  uint16_t   _base = 0;
  ConfigData _cfg{};
  char       _err[64] = {0};
};
