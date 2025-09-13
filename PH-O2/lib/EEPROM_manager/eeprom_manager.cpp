#include "eeprom_manager.h"
#include <string.h>
#include <stddef.h>   // offsetof
#include <math.h>

static constexpr uint16_t kMagic = 0xC0AD;

// v1 (solo ADC)
struct OldConfigV1 {
  uint16_t magic;    // 0xC0AD
  uint16_t version;  // 0x0001
  struct { float scale; float offset; } adc;
  uint32_t crc;
};

// v2 (ADC + pH2pt)
struct OldConfigV2 {
  uint16_t magic;    // 0xC0AD
  uint16_t version;  // 0x0002
  struct { float scale; float offset; } adc;
  struct { float V7; float V4; float tC; } ph2pt;
  uint32_t crc;
};

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
  // Leer cabecera mínima para detectar versión
  uint16_t magic = 0, version = 0;
  EEPROM.get(_base + offsetof(ConfigData, magic),   magic);
  EEPROM.get(_base + offsetof(ConfigData, version), version);

  if (magic != kMagic) {
    strncpy(_err, "MAGIC invalido", sizeof(_err)-1);
    return false;
  }

  if (version == 0x0001) {
    // ---- Migra v1 -> v3 ----
    OldConfigV1 old{}; EEPROM.get(_base, old);

    // CRC v1
    const size_t lenOld = offsetof(OldConfigV1, crc);
    const uint8_t* bytesOld = reinterpret_cast<const uint8_t*>(&old);
    const uint32_t calcOld = crc32(bytesOld, lenOld);
    if (calcOld != old.crc) { strncpy(_err, "CRC v1 invalido", sizeof(_err)-1); return false; }

    // Copia a v3
    memset(&_cfg, 0, sizeof(_cfg));
    _cfg.magic   = kMagic;
    _cfg.version = kVersion;
    _cfg.adc.scale  = old.adc.scale;
    _cfg.adc.offset = old.adc.offset;
    _cfg.ph2pt.V7 = NAN; _cfg.ph2pt.V4 = NAN; _cfg.ph2pt.tC = NAN;
    _cfg.kcl_fill_ms    = 3000;
    _cfg.h2o_fill_ms    = 3000;
    _cfg.sample_fill_ms = 3000;
    computeCrc_();

    EEPROM.put(_base, _cfg);
    if (!EEPROM.commit()) { strncpy(_err, "commit mig v3 fallo", sizeof(_err)-1); return false; }
    _err[0] = '\0';
    return true;
  }

  if (version == 0x0002) {
    // ---- Migra v2 -> v3 ----
    OldConfigV2 old{}; EEPROM.get(_base, old);

    // CRC v2
    const size_t lenOld = offsetof(OldConfigV2, crc);
    const uint8_t* bytesOld = reinterpret_cast<const uint8_t*>(&old);
    const uint32_t calcOld = crc32(bytesOld, lenOld);
    if (calcOld != old.crc) { strncpy(_err, "CRC v2 invalido", sizeof(_err)-1); return false; }

    // Copia a v3 con defaults nuevos
    memset(&_cfg, 0, sizeof(_cfg));
    _cfg.magic   = kMagic;
    _cfg.version = kVersion;
    _cfg.adc.scale  = old.adc.scale;
    _cfg.adc.offset = old.adc.offset;
    _cfg.ph2pt.V7 = old.ph2pt.V7;
    _cfg.ph2pt.V4 = old.ph2pt.V4;
    _cfg.ph2pt.tC = old.ph2pt.tC;
    _cfg.kcl_fill_ms    = 3000;
    _cfg.h2o_fill_ms    = 3000;
    _cfg.sample_fill_ms = 3000;
    computeCrc_();

    EEPROM.put(_base, _cfg);
    if (!EEPROM.commit()) { strncpy(_err, "commit mig v3 fallo", sizeof(_err)-1); return false; }
    _err[0] = '\0';
    return true;
  }

  if (version != kVersion) {
    strncpy(_err, "VERSION distinta", sizeof(_err)-1);
    return false;
  }

  // Leer v3 y validar CRC
  ConfigData tmp{}; EEPROM.get(_base, tmp);
  const size_t len = offsetof(ConfigData, crc);
  const uint8_t* bytes = reinterpret_cast<const uint8_t*>(&tmp);
  const uint32_t calc = crc32(bytes, len);
  if (calc != tmp.crc) { strncpy(_err, "CRC invalido", sizeof(_err)-1); return false; }

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

  // ADC por defecto
  _cfg.adc.scale  = 1.0f;
  _cfg.adc.offset = 0.0f;

  // pH 2pt por defecto: sin calibrar
  _cfg.ph2pt.V7 = NAN;
  _cfg.ph2pt.V4 = NAN;
  _cfg.ph2pt.tC = NAN;

  // Fill times por defecto (ms)
  _cfg.kcl_fill_ms    = 3000;
  _cfg.h2o_fill_ms    = 3000;
  _cfg.sample_fill_ms = 3000;

  computeCrc_();
  _err[0] = '\0';
}

void ConfigStore::setADC(float s, float o) {
  _cfg.adc.scale  = s;
  _cfg.adc.offset = o;
}
void ConfigStore::getADC(float& s, float& o) const {
  s = _cfg.adc.scale;
  o = _cfg.adc.offset;
}

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

// ---- Fill times ----
void ConfigStore::setFillTimes(uint32_t kcl_ms, uint32_t h2o_ms, uint32_t sample_ms) {
  _cfg.kcl_fill_ms    = kcl_ms;
  _cfg.h2o_fill_ms    = h2o_ms;
  _cfg.sample_fill_ms = sample_ms;
}
void ConfigStore::getFillTimes(uint32_t& kcl_ms, uint32_t& h2o_ms, uint32_t& sample_ms) const {
  kcl_ms    = _cfg.kcl_fill_ms;
  h2o_ms    = _cfg.h2o_fill_ms;
  sample_ms = _cfg.sample_fill_ms;
}
void ConfigStore::setKclFillMs(uint32_t ms)    { _cfg.kcl_fill_ms = ms; }
void ConfigStore::setH2oFillMs(uint32_t ms)    { _cfg.h2o_fill_ms = ms; }
void ConfigStore::setSampleFillMs(uint32_t ms) { _cfg.sample_fill_ms = ms; }
uint32_t ConfigStore::kclFillMs() const        { return _cfg.kcl_fill_ms; }
uint32_t ConfigStore::h2oFillMs() const        { return _cfg.h2o_fill_ms; }
uint32_t ConfigStore::sampleFillMs() const     { return _cfg.sample_fill_ms; }

// ===== CRC32 (polinomio 0xEDB88320) =====
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
