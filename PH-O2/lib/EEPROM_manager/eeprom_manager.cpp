#include "eeprom_manager.h"
#include <string.h>
#include <stddef.h>   // offsetof

static constexpr uint16_t kMagic = 0xC0AD;

// Estructura antigua v1 (solo ADC) para migración
struct OldConfigV1 {
  uint16_t magic;    // 0xC0AD
  uint16_t version;  // 0x0001
  struct { float scale; float offset; } adc;
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
  EEPROM.get(_base + offsetof(ConfigData, magic), magic);
  EEPROM.get(_base + offsetof(ConfigData, version), version);

  if (magic != kMagic) {
    strncpy(_err, "MAGIC invalido", sizeof(_err)-1);
    return false;
  }

  if (version == 0x0001) {
    // ---- Migración desde v1 (solo ADC) ----
    OldConfigV1 old{};
    EEPROM.get(_base, old);

    // Verificar CRC v1
    const size_t lenOld = offsetof(OldConfigV1, crc);
    const uint8_t* bytesOld = reinterpret_cast<const uint8_t*>(&old);
    const uint32_t calcOld = crc32(bytesOld, lenOld);
    if (calcOld != old.crc) {
      strncpy(_err, "CRC v1 invalido", sizeof(_err)-1);
      return false;
    }

    // Copiar a _cfg (v2) y setear pH2pt = NaN (no calibrado)
    memset(&_cfg, 0, sizeof(_cfg));
    _cfg.magic   = kMagic;
    _cfg.version = kVersion;
    _cfg.adc.scale  = old.adc.scale;
    _cfg.adc.offset = old.adc.offset;
    _cfg.ph2pt.V7 = NAN; _cfg.ph2pt.V4 = NAN; _cfg.ph2pt.tC = NAN;
    computeCrc_();

    // Guardar inmediatamente en formato v2
    EEPROM.put(_base, _cfg);
    if (!EEPROM.commit()) {
      strncpy(_err, "commit mig v2 fallo", sizeof(_err)-1);
      return false;
    }
    _err[0] = '\0';
    return true;
  }

  if (version != kVersion) {
    strncpy(_err, "VERSION distinta", sizeof(_err)-1);
    return false;
  }

  // Leer bloque v2 y validar CRC
  ConfigData tmp{};
  EEPROM.get(_base, tmp);

  const size_t len = offsetof(ConfigData, crc);
  const uint8_t* bytes = reinterpret_cast<const uint8_t*>(&tmp);
  const uint32_t calc = crc32(bytes, len);
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

  // ADC por defecto
  _cfg.adc.scale  = 1.0f;
  _cfg.adc.offset = 0.0f;

  // pH 2pt por defecto: sin calibrar
  _cfg.ph2pt.V7 = NAN;
  _cfg.ph2pt.V4 = NAN;
  _cfg.ph2pt.tC = NAN;

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
