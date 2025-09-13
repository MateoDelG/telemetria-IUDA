#include "ADS1115_manager.h"
#include <cstring> // strncpy

ADS1115Manager::ADS1115Manager(uint8_t i2c_addr, TwoWire* wire)
: addr_(i2c_addr), wire_(wire) {}

bool ADS1115Manager::begin() {
  connected_ = ads_.begin(addr_, wire_);
  if (!connected_) {
    setError_("ADS1115 no responde");
    return false;
  }
  ads_.setGain(gain_);
  ads_.setDataRate(rate_);
  last_error_[0] = '\0';
  return true;
}

void ADS1115Manager::setGain(adsGain_t g) {
  gain_ = g;
  if (connected_) ads_.setGain(gain_);
}

void ADS1115Manager::setDataRate(uint16_t r) {
  rate_ = r;
  if (connected_) ads_.setDataRate(rate_);
}

void ADS1115Manager::setAveraging(uint8_t n) {
  if (n == 0) n = 1;
  avg_ = n;
}

void ADS1115Manager::setCalibration(float scale, float offset) {
  cal_scale_  = scale;
  cal_offset_ = offset;
}

bool ADS1115Manager::checkChannel_(uint8_t ch) {
  if (ch > 3) {
    setError_("Canal inválido (0..3)");
    return false;
  }
  if (!connected_) {
    setError_("ADS1115 no inicializado");
    return false;
  }
  return true;
}

void ADS1115Manager::setError_(const char* msg) {
  strncpy(last_error_, msg, sizeof(last_error_) - 1);
  last_error_[sizeof(last_error_) - 1] = '\0';
}

bool ADS1115Manager::readOnceRawSingle_(uint8_t ch, int16_t& raw) {
  switch (ch) {
    case 0: raw = ads_.readADC_SingleEnded(0); return true;
    case 1: raw = ads_.readADC_SingleEnded(1); return true;
    case 2: raw = ads_.readADC_SingleEnded(2); return true;
    case 3: raw = ads_.readADC_SingleEnded(3); return true;
    default: return false;
  }
}

bool ADS1115Manager::readOnceRawDiff_(uint8_t pair, int16_t& raw) {
  if (pair == 0) {           // A0-A1
    raw = ads_.readADC_Differential_0_1();
    return true;
  } else if (pair == 1) {    // A2-A3
    raw = ads_.readADC_Differential_2_3();
    return true;
  }
  return false;
}

bool ADS1115Manager::readSingleRaw(uint8_t channel, int16_t& raw) {
  if (!checkChannel_(channel)) return false;

  // ---- Dummy read para asentamiento ----
  int16_t throwaway;
  if (!readOnceRawSingle_(channel, throwaway)) {
    setError_("Lectura dummy falló");
    return false;
  }

  // ---- Lecturas promediadas ----
  int32_t acc = 0;
  for (uint8_t i = 0; i < avg_; ++i) {
    int16_t r;
    if (!readOnceRawSingle_(channel, r)) {
      setError_("Lectura raw falló");
      return false;
    }
    acc += r;
  }

  last_raw_ = static_cast<int16_t>(acc / (int32_t)avg_);
  last_error_[0] = '\0';
  raw = last_raw_;
  return true;
}


bool ADS1115Manager::readSingle(uint8_t channel, float& volts) {
  int16_t raw;
  if (!readSingleRaw(channel, raw)) return false;

  float v = ads_.computeVolts(raw);
  last_volts_ = applyCal_(v);
  volts = last_volts_;
  return true;
}

bool ADS1115Manager::readDifferentialRaw(uint8_t pair, int16_t& raw) {
  if (!connected_) {
    setError_("ADS1115 no inicializado");
    return false;
  }
  if (pair > 1) {
    setError_("Par inválido (0->01, 1->23)");
    return false;
  }

  int32_t acc = 0;
  for (uint8_t i = 0; i < avg_; ++i) {
    int16_t r;
    if (!readOnceRawDiff_(pair, r)) {
      setError_("Lectura diff raw falló");
      return false;
    }
    acc += r;
  }
  last_raw_ = static_cast<int16_t>(acc / (int32_t)avg_);
  last_error_[0] = '\0';
  raw = last_raw_;
  return true;
}

bool ADS1115Manager::readDifferential01(float& volts) {
  int16_t raw;
  if (!readDifferentialRaw(0, raw)) return false;
  float v = ads_.computeVolts(raw);
  last_volts_ = applyCal_(v);
  volts = last_volts_;
  return true;
}

bool ADS1115Manager::readDifferential23(float& volts) {
  int16_t raw;
  if (!readDifferentialRaw(1, raw)) return false;
  float v = ads_.computeVolts(raw);
  last_volts_ = applyCal_(v);
  volts = last_volts_;
  return true;
}
