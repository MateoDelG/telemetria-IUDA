#pragma once
#include <Arduino.h>
#include <EEPROM.h>

class ConfigStore {
public:
  // Inicializa la EEPROM emulada (ESP32). eepromSize típico: 128–256 bytes.
  bool begin(size_t eepromSize = 128, uint16_t baseAddr = 0);

  // Carga desde EEPROM (valida MAGIC/versión/CRC). Devuelve true si OK.
  // Migra automáticamente v1→v3 y v2→v3.
  bool load();

  // Guarda a EEPROM (actualiza CRC). Devuelve true si commit() fue OK.
  bool save();

  // Restaura valores por defecto
  // ADC = (1.0, 0.0), pH 2pt = NaN, fill times = 3000 ms
  void resetDefaults();

  // --- ADC ---
  void setADC(float scale, float offset);
  void getADC(float& scale, float& offset) const;

  // --- pH 2 puntos (V7, V4, tC) ---
  void setPH2pt(float V7, float V4, float tCalC);
  void getPH2pt(float& V7, float& V4, float& tCalC) const;

  // --- Fill times (ms) ---
  void setFillTimes(uint32_t kcl_ms, uint32_t h2o_ms, uint32_t sample_ms);
  void getFillTimes(uint32_t& kcl_ms, uint32_t& h2o_ms, uint32_t& sample_ms) const;

  // Conveniencia individual
  void setKclFillMs(uint32_t ms);
  void setH2oFillMs(uint32_t ms);
  void setSampleFillMs(uint32_t ms);
  uint32_t kclFillMs() const;
  uint32_t h2oFillMs() const;
  uint32_t sampleFillMs() const;

  // Último error de load()/save()
  const char* lastError() const { return _err; }

  // Versión del layout almacenado
  static constexpr uint16_t kVersion = 0x0003;  // v3: añade fill times

private:
  struct Pair { float scale; float offset; };
  struct PH2pt { float V7; float V4; float tC; };

  // v3
  struct ConfigData {
    uint16_t magic;    // 0xC0AD
    uint16_t version;  // kVersion
    Pair     adc;      // calibración ADC (m,b)
    PH2pt    ph2pt;    // calibración pH (V7, V4, tC)
    // --- nuevos campos v3 ---
    uint32_t kcl_fill_ms;
    uint32_t h2o_fill_ms;
    uint32_t sample_fill_ms;
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
