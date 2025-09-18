#pragma once
#include <Arduino.h>
#include <ArduinoJson.h>

namespace UartProto {

class UARTManager {
public:
  explicit UARTManager(Stream& inOut);

  void begin(unsigned long baud = 115200);
  void loop();

  // ====== Setters individuales ======
  void setLevelH2O(bool v);
  void setLevelKCL(bool v);
  void setAutoRunning(bool v);

  void setLastPh(float v);
  void setLastTempC(float v);
  void setLastResult(const String& r);
  void setLastHasData(bool v);

  // Nuevo: control por flag de solicitud de medición automática
  void setAutoMeasureRequested(bool v);

  // ====== Getters individuales ======
  bool   getLevelH2O() const;
  bool   getLevelKCL() const;
  bool   getAutoRunning() const;

  float  getLastPh() const;
  float  getLastTempC() const;
  String getLastResult() const;
  bool   getLastHasData() const;

  // Nuevo
  bool   getAutoMeasureRequested() const;

private:
  Stream& io_;
  String lineBuf_;

  // Variables de estado
  volatile bool levelH2O_ok_ = false;
  volatile bool levelKCL_ok_ = false;
  volatile bool auto_running_ = false;

  // Petición de medición automática (sin callback)
  volatile bool autoMeasureRequested_ = false;

  volatile float  last_ph_ = NAN;
  volatile float  last_tempC_ = NAN;
  String          last_result_ = "OK";
  volatile bool   last_has_data_ = false;

#if defined(ESP32)
  portMUX_TYPE mux_ = portMUX_INITIALIZER_UNLOCKED;
  void lock()   { taskENTER_CRITICAL(&mux_); }
  void unlock() { taskEXIT_CRITICAL(&mux_); }
#else
  void lock()   {}
  void unlock() {}
#endif

  // Procesamiento de comandos
  void processLine_(const String& line);
  void handle_get_status_();
  void handle_get_last_();
  void handle_auto_measure_(JsonObject dataIn);

  // Helpers de salida
  void sendOk_();
  void sendError_(const char* err);
  void sendJson_(const JsonDocument& doc);
};

} // namespace UartProto
