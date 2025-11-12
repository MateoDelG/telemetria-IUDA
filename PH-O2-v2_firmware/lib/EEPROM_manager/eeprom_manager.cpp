#include "eeprom_manager.h"
#include <string.h>
#include <stddef.h>
#include <math.h>

// ===== Privados =====
static constexpr uint16_t kMagic = 0xC0AD;

uint32_t ConfigStore::clampMs(uint32_t ms, uint32_t lo, uint32_t hi) {
  if (ms < lo) ms = lo;
  if (ms > hi) ms = hi;
  return ms;
}

// ===== CRC32 =====
uint32_t ConfigStore::crc32(const uint8_t* data, size_t len) {
  uint32_t crc = 0xFFFFFFFFu;
  while (len--) {
    crc ^= *data++;
    for (uint8_t i = 0; i < 8; ++i)
      crc = (crc & 1) ? (crc >> 1) ^ 0xEDB88320u : (crc >> 1);
  }
  return ~crc;
}

void ConfigStore::computeCrc_() {
  const size_t len = offsetof(ConfigData, crc);
  const uint8_t* bytes = reinterpret_cast<const uint8_t*>(&_cfg);
  _cfg.crc = crc32(bytes, len);
}

// ===== API =====
bool ConfigStore::begin(size_t eepromSize, uint16_t baseAddr) {
  _eepromSize = eepromSize;
  _base = baseAddr;
  if (!EEPROM.begin(_eepromSize)) {
    strncpy(_err, "EEPROM.begin() fallo", sizeof(_err)-1);
    _err[sizeof(_err)-1] = '\0';
    return false;
  }
  _err[0] = '\0';
  return true;
}

bool ConfigStore::load() {
  uint16_t magic = 0, version = 0;
  EEPROM.get(_base + offsetof(ConfigData, magic),   magic);
  EEPROM.get(_base + offsetof(ConfigData, version), version);

  if (magic != kMagic) {
    strncpy(_err, "MAGIC invalido", sizeof(_err)-1);
    return false;
  }

  // Solo aceptamos la versión actual (v7)
  if (version != kVersion) {
    strncpy(_err, "VERSION distinta (no soportada)", sizeof(_err)-1);
    return false;
  }

  ConfigData tmp{};
  EEPROM.get(_base, tmp);

  // Verificar CRC del bloque leído
  const size_t len = offsetof(ConfigData, crc);
  const uint32_t calc = crc32(reinterpret_cast<const uint8_t*>(&tmp), len);
  if (calc != tmp.crc) {
    strncpy(_err, "CRC invalido", sizeof(_err)-1);
    return false;
  }

  _cfg = tmp;
  _err[0] = '\0';
  return true;
}

bool ConfigStore::save() {
  _cfg.magic   = kMagic;
  _cfg.version = kVersion;
  computeCrc_();
  EEPROM.put(_base, _cfg);
  if (!EEPROM.commit()) {
    strncpy(_err, "EEPROM.commit() fallo", sizeof(_err)-1);
    return false;
  }
  _err[0] = '\0';
  return true;
}

void ConfigStore::resetDefaults() {
  memset(&_cfg, 0, sizeof(_cfg));
  _cfg.magic   = kMagic;
  _cfg.version = kVersion;

  // ADC
  _cfg.adc.scale  = 1.0f;
  _cfg.adc.offset = 0.0f;

  // pH 2pt sin calibrar
  _cfg.ph2pt.V7 = NAN;
  _cfg.ph2pt.V4 = NAN;
  _cfg.ph2pt.tC = NAN;

  // pH 3pt sin calibrar
  _cfg.ph3pt.V4  = NAN;
  _cfg.ph3pt.V7  = NAN;
  _cfg.ph3pt.V10 = NAN;
  _cfg.ph3pt.tC  = NAN;

  // FILL TIMES por defecto (ms)
  _cfg.kcl_fill_ms    = 3000;
  _cfg.h2o_fill_ms    = 3000;
  _cfg.sample_fill_ms = 3000;
  _cfg.drain_ms       = 15000;

  // TIMEOUTS por defecto (ms) (solo sample y drain)
  _cfg.sample_timeout_ms = 3000;
  _cfg.drain_timeout_ms  = 15000;

  // Nº SAMPLE (módulo fijo)
  _cfg.sample_count = 4;

  // Stabilization (ms)
  _cfg.stabilization_ms = 30000;

  computeCrc_();
  _err[0] = '\0';
}

// ---- ADC ----
void ConfigStore::setADC(float s, float o) {
  _cfg.adc.scale  = s;
  _cfg.adc.offset = o;
}
void ConfigStore::getADC(float& s, float& o) const {
  s = _cfg.adc.scale;
  o = _cfg.adc.offset;
}

// ---- pH 2pt ----
void ConfigStore::setPH2pt(float V7, float V4, float tCalC) {
  _cfg.ph2pt.V7 = V7;
  _cfg.ph2pt.V4 = V4;
  _cfg.ph2pt.tC = tCalC;
}
void ConfigStore::getPH2pt(float& V7, float& V4, float& tCalC) const {
  V7   = _cfg.ph2pt.V7;
  V4   = _cfg.ph2pt.V4;
  tCalC= _cfg.ph2pt.tC;
}

// ---- pH 3pt ----
void ConfigStore::setPH3pt(float V4, float V7, float V10, float tCalC) {
  _cfg.ph3pt.V4  = V4;
  _cfg.ph3pt.V7  = V7;
  _cfg.ph3pt.V10 = V10;
  _cfg.ph3pt.tC  = tCalC;
}
void ConfigStore::getPH3pt(float& V4, float& V7, float& V10, float& tCalC) const {
  V4   = _cfg.ph3pt.V4;
  V7   = _cfg.ph3pt.V7;
  V10  = _cfg.ph3pt.V10;
  tCalC= _cfg.ph3pt.tC;
}
bool ConfigStore::hasPH2pt() const {
  return !(isnan(_cfg.ph2pt.V7) || isnan(_cfg.ph2pt.V4));
}
bool ConfigStore::hasPH3pt() const {
  return !(isnan(_cfg.ph3pt.V4) || isnan(_cfg.ph3pt.V7) || isnan(_cfg.ph3pt.V10));
}

// ---- FILL TIMES ----
void ConfigStore::setFillTimes(uint32_t kcl_ms, uint32_t h2o_ms, uint32_t sample_ms) {
  _cfg.kcl_fill_ms    = clampFill(kcl_ms);
  _cfg.h2o_fill_ms    = clampFill(h2o_ms);
  _cfg.sample_fill_ms = clampFill(sample_ms);
}
void ConfigStore::getFillTimes(uint32_t& kcl_ms, uint32_t& h2o_ms, uint32_t& sample_ms) const {
  kcl_ms    = _cfg.kcl_fill_ms;
  h2o_ms    = _cfg.h2o_fill_ms;
  sample_ms = _cfg.sample_fill_ms;
}
void ConfigStore::setKclFillMs(uint32_t ms)    { _cfg.kcl_fill_ms    = clampFill(ms); }
void ConfigStore::setH2oFillMs(uint32_t ms)    { _cfg.h2o_fill_ms    = clampFill(ms); }
void ConfigStore::setSampleFillMs(uint32_t ms) { _cfg.sample_fill_ms = clampFill(ms); }
uint32_t ConfigStore::kclFillMs()    const { return _cfg.kcl_fill_ms; }
uint32_t ConfigStore::h2oFillMs()    const { return _cfg.h2o_fill_ms; }
uint32_t ConfigStore::sampleFillMs() const { return _cfg.sample_fill_ms; }

// ---- DRAIN planned duration + timeout ----
void ConfigStore::setDrainMs(uint32_t ms) { _cfg.drain_ms = clampDrain(ms); }
uint32_t ConfigStore::drainMs() const     { return _cfg.drain_ms; }

// ---- TIMEOUTS (solo sample & drain) ----
void ConfigStore::setSampleTimeoutMs(uint32_t ms) { _cfg.sample_timeout_ms = clampTimeout(ms); }
void ConfigStore::setDrainTimeoutMs(uint32_t ms)  { _cfg.drain_timeout_ms  = clampTimeout(ms); }
uint32_t ConfigStore::sampleTimeoutMs() const     { return _cfg.sample_timeout_ms; }
uint32_t ConfigStore::drainTimeoutMs()  const     { return _cfg.drain_timeout_ms; }

// ---- SAMPLE COUNT ----
void    ConfigStore::setSampleCount(uint8_t n) { _cfg.sample_count = n; }
uint8_t ConfigStore::sampleCount() const       { return _cfg.sample_count; }

// ---- Stabilization ----
void ConfigStore::setStabilizationMs(uint32_t ms) { _cfg.stabilization_ms = ms; }
uint32_t ConfigStore::stabilizationMs() const     { return _cfg.stabilization_ms; }
