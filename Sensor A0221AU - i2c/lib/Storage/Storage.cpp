#include "Storage.h"
#include <Preferences.h>

namespace {
  Preferences prefs;
  const char *NAMESPACE = "sensorCfg";
  const uint32_t CFG_VERSION = 1;  // por si el día de mañana cambias la estructura
}

void Storage::begin() {
  // RW = false (segunda bandera) indica modo lectura/escritura
  prefs.begin(NAMESPACE, false);
}

bool Storage::loadConfig(ConfigData &cfg) {
  uint32_t ver = prefs.getUInt("ver", 0);
  if (ver != CFG_VERSION) {
    // No hay config previa o versión distinta
    return false;
  }

    cfg.minLevel     = prefs.getFloat("minL",  10.0f);
    cfg.maxLevel     = prefs.getFloat("maxL", 100.0f);
    cfg.samplePeriod = prefs.getInt  ("samp", 200);

    cfg.kalMea       = prefs.getFloat("kMea",  3.0f);
    cfg.kalEst       = prefs.getFloat("kEst",  8.0f);
    cfg.kalQ         = prefs.getFloat("kQ",    0.08f);

    cfg.i2cAddress   = prefs.getUChar("i2c",  0x30);   // default 0x30


  return true;
}

void Storage::saveConfig(const ConfigData &cfg) {
prefs.putUInt ("ver",  CFG_VERSION);
prefs.putFloat("minL", cfg.minLevel);
prefs.putFloat("maxL", cfg.maxLevel);
prefs.putInt  ("samp", cfg.samplePeriod);

prefs.putFloat("kMea", cfg.kalMea);
prefs.putFloat("kEst", cfg.kalEst);
prefs.putFloat("kQ",   cfg.kalQ);

prefs.putUChar("i2c",  cfg.i2cAddress);

}
