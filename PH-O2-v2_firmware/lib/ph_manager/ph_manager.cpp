#include "pH_manager.h"

PHManager::PHManager(ADS1115Manager* ads, uint8_t ads_channel, uint8_t avgSamples)
: ads_(ads), ch_(ads_channel), avg_(avgSamples) {}

bool PHManager::begin() {
  if (!ads_) {
    setError_("ADS pointer null");
    return false;
  }
  // Asegura un mínimo de promedios
  if (avg_ == 0) avg_ = 1;
  last_error_[0] = '\0';
  return true;
}

void PHManager::setTwoPointCalibration(float pH1, float V1, float pH2, float V2, float tCalC) {
  // Evita división por cero
  if (fabsf(V2 - V1) < 1e-6f) {
    // Mantiene la calibración previa si inválida
    setError_("Calibración inválida (V1≈V2)");
    return;
  }

  // Ajuste lineal: pH = a*V + b
  a_cal_ = (pH2 - pH1) / (V2 - V1);
  b_cal_ = pH1 - a_cal_ * V1;
  tcal_c_ = tCalC;
  last_error_[0] = '\0';
}

void PHManager::setCalibrationCoeffs(float a_cal, float b_cal, float tCalC) {
  a_cal_ = a_cal;
  b_cal_ = b_cal;
  tcal_c_ = tCalC;
  last_error_[0] = '\0';
}

void PHManager::getCalibrationCoeffs(float& a_cal, float& b_cal, float& tCalC) const {
  a_cal = a_cal_;
  b_cal = b_cal_;
  tCalC = tcal_c_;
}

void PHManager::setAveraging(uint8_t n) {
  if (n == 0) n = 1;
  avg_ = n;
}

void PHManager::setChannel(uint8_t ch) {
  ch_ = ch;
}

void PHManager::setError_(const char* msg) {
  strncpy(last_error_, msg, sizeof(last_error_) - 1);
  last_error_[sizeof(last_error_) - 1] = '\0';
}

bool PHManager::readAveragedVolts_(float& volts) {
  if (!ads_) {
    setError_("ADS pointer null");
    return false;
  }

  float acc = 0.0f;
  float v = 0.0f;
  for (uint8_t i = 0; i < avg_; ++i) {
    if (!ads_->readSingle(ch_, v)) {
      setError_("ADS read fail");
      return false;
    }
    acc += v;
  }
  volts = acc / (float)avg_;
  last_volts_ = volts;
  return true;
}

bool PHManager::readPH(float tempC, float& ph, float* voltsOut) {
  float v;
  if (!readAveragedVolts_(v)) {
    return false;
  }
  if (voltsOut) *voltsOut = v;

  // Conversión a pH con pendiente ajustada por temperatura:
  // Nernst ~ proporcional a Kelvin -> escala de pendiente por factor T_K / Tcal_K
  const float TK_meas = kelvin_(tempC);
  const float TK_cal  = kelvin_(tcal_c_);
  if (TK_cal < 1.0f) {
    setError_("Tcal inválida");
    return false;
  }

  // Reescala SOLO la pendiente. El offset (intersección) se deja igual,
  // centrado alrededor del punto neutro que tu calibración ya acomodó.
  const float scale = TK_meas / TK_cal;
  const float a_T = a_cal_ * scale;

  ph = a_T * v + b_cal_;
  last_ph_ = ph;
  last_error_[0] = '\0';
  return true;
}
