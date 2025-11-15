#include "UltrasonicA02YYUW.h"

UltrasonicA02YYUW::UltrasonicA02YYUW(
    HardwareSerial &serialPort,
    int rxPin,
    int txPin,
    float mea_e,
    float est_e,
    float q
)
  : _serial(serialPort),
    _rx(rxPin),
    _tx(txPin),
    _kalman(mea_e, est_e, q)
{}

void UltrasonicA02YYUW::begin(unsigned long baudrate) {
  pinMode(_rx, INPUT_PULLDOWN);   // evita ruido cuando el sensor está desconectado
  _serial.begin(baudrate, SERIAL_8N1, _rx, _tx);
}

void UltrasonicA02YYUW::update() {
  while (_serial.available()) {
    uint8_t byteIn = _serial.read();

    switch (_index) {
      case 0:
        if (byteIn == 0xFF) {
          _buffer[0] = byteIn;
          _index = 1;
        }
        break;

      case 1:
      case 2:
      case 3:
        _buffer[_index++] = byteIn;

        if (_index > 3) {
          processPacket();
          _index = 0;
        }
        break;

      default:
        _index = 0;
    }
  }
}

void UltrasonicA02YYUW::processPacket() {
  uint8_t h   = _buffer[1];
  uint8_t l   = _buffer[2];
  uint8_t sum = _buffer[3];

  // checksum
  uint8_t expected = (uint8_t)((0xFF + h + l) & 0xFF);
  if (expected != sum) {
    return;
  }

  // mm → cm
  int   mm = ((uint16_t)h << 8) | l;
  float cm = mm / 10.0f;

  // rango válido del sensor
  if (cm < 3.0f || cm > 750.0f) {
    return;
  }

  _distanceCmRaw = cm;
  _hasValid = true;

  // ---------------------------
  // 🟢 Arranque inteligente del filtro
  // ---------------------------
  if (!_kalmanInitialized) {
    _distanceCmFiltered = cm;

    // Primer update para inicializar internamente:
    _kalman.updateEstimate(cm);

    _kalmanInitialized = true;
    return;
  }

  // ---------------------------
  // 🟢 Filtro Kalman normal
  // ---------------------------
  _distanceCmFiltered = _kalman.updateEstimate(cm);
}

float UltrasonicA02YYUW::getDistanceRaw() const {
  return _distanceCmRaw;
}

float UltrasonicA02YYUW::getDistance() const {
  return _distanceCmFiltered;
}

bool UltrasonicA02YYUW::isValid() const {
  return _hasValid;
}

void UltrasonicA02YYUW::setMeasurementError(float mea_e) {
  _kalman.setMeasurementError(mea_e);
}

void UltrasonicA02YYUW::setEstimateError(float est_e) {
  _kalman.setEstimateError(est_e);
}

void UltrasonicA02YYUW::setProcessNoise(float q) {
  _kalman.setProcessNoise(q);
}
