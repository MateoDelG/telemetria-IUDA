#include "uart_manager.h"
#include "globals.h"   // aquí está definido remoteManager

namespace UartProto {

UARTManager::UARTManager(Stream& inOut) : io_(inOut) {
  remoteManager.log("[UART] Manager creado");
}

void UARTManager::begin(unsigned long baud) {
  (void)baud; // SerialX.begin(...) lo hace el usuario
  remoteManager.log("[UART] begin() llamado (recuerda inicializar SerialX)");
}

void UARTManager::loop() {
  while (io_.available()) {
    char c = (char)io_.read();
    if (c == '\n') {
      String line = lineBuf_;
      lineBuf_ = "";
      if (line.length() > 0) {
        remoteManager.log(String("[UART] RX: ") + line);
        processLine_(line);
      }
    } else if (c != '\r') {
      lineBuf_ += c;
      if (lineBuf_.length() > 1024) {
        lineBuf_.clear();
        remoteManager.log("[UART] ERROR: BAD_ARGS (línea demasiado larga)");
        sendError_("BAD_ARGS");
      }
    }
  }
}

// ====== Setters ======
void UARTManager::setLevelH2O(bool v)         { lock(); levelH2O_ok_ = v; unlock(); }
void UARTManager::setLevelKCL(bool v)         { lock(); levelKCL_ok_ = v; unlock(); }
void UARTManager::setAutoRunning(bool v)      { lock(); auto_running_ = v; unlock(); }
void UARTManager::setAutoMeasureRequested(bool v){ lock(); autoMeasureRequested_ = v; unlock(); }

void UARTManager::setLastPh(float v)          { lock(); last_ph_ = v; last_has_data_ = true; unlock(); }
void UARTManager::setLastTempC(float v)       { lock(); last_tempC_ = v; last_has_data_ = true; unlock(); }
void UARTManager::setLastResult(const String& r){ lock(); last_result_ = r; last_has_data_ = true; unlock(); }
void UARTManager::setLastHasData(bool v)      { lock(); last_has_data_ = v; unlock(); }

// ====== Getters ======
bool   UARTManager::getLevelH2O() const       { return levelH2O_ok_; }
bool   UARTManager::getLevelKCL() const       { return levelKCL_ok_; }
bool   UARTManager::getAutoRunning() const    { return auto_running_; }
bool   UARTManager::getAutoMeasureRequested() const { return autoMeasureRequested_; }

float  UARTManager::getLastPh() const         { return last_ph_; }
float  UARTManager::getLastTempC() const      { return last_tempC_; }
String UARTManager::getLastResult() const     { return last_result_; }
bool   UARTManager::getLastHasData() const    { return last_has_data_; }

// ================== Procesamiento NDJSON ==================
void UARTManager::processLine_(const String& lineRaw) {

    String line = lineRaw;
    line.trim();
    int i = line.indexOf('{');
    if (i > 0) line.remove(0, i);
    if (line.isEmpty() || line[0] != '{') {
        remoteManager.log("[UART] DESCARTE: línea sin JSON válido");
        sendError_("BAD_JSON");
        return;
    }

  StaticJsonDocument<256> in;
  DeserializationError err = deserializeJson(in, line);
  if (err) {
    remoteManager.log(String("[UART] ERROR: BAD_JSON (") + err.c_str() + ")");
    sendError_("BAD_JSON");
    return;
  }

  const char* op = in["op"] | "";
  JsonObject dataIn = in["data"].isNull() ? JsonObject() : in["data"].as<JsonObject>();

  if      (!strcmp(op, "get_status"))   { remoteManager.log("[UART] OP: get_status");  handle_get_status_(); }
  else if (!strcmp(op, "get_last"))     { remoteManager.log("[UART] OP: get_last");    handle_get_last_(); }
  else if (!strcmp(op, "auto_measure")) { remoteManager.log("[UART] OP: auto_measure");handle_auto_measure_(dataIn); }
  else {
    remoteManager.log(String("[UART] ERROR: BAD_OP (") + op + ")");
    sendError_("BAD_OP");
  }
}

// --- get_status ---
void UARTManager::handle_get_status_() {
  lock();
  bool h2o   = levelH2O_ok_;
  bool kcl   = levelKCL_ok_;
  bool run   = auto_running_;
  bool areq  = autoMeasureRequested_;
  unlock();

  StaticJsonDocument<192> out;
  out["ok"] = true;
  JsonObject data = out.createNestedObject("data");
  JsonObject js = data.createNestedObject("level_sensors");
  js["h2o"] = h2o;
  js["kcl"] = kcl;
  data["auto_running"] = run;
  data["auto_req"]     = areq;
  sendJson_(out);

  remoteManager.log(String("[UART] TX get_status -> h2o=") + (h2o?"1":"0") +
             " kcl=" + (kcl?"1":"0") +
             " auto_running=" + (run?"1":"0") +
             " auto_req=" + (areq?"1":"0"));
}

// --- get_last ---
void UARTManager::handle_get_last_() {
  lock();
  bool has = last_has_data_;
  float ph = last_ph_;
  float tc = last_tempC_;
  String res = last_result_;
  bool h2o = levelH2O_ok_;
  bool kcl = levelKCL_ok_;
  unlock();

  if (!has) {
    remoteManager.log("[UART] TX get_last -> NO_DATA");
    sendError_("NO_DATA");
    return;
  }

  StaticJsonDocument<256> out;
  out["ok"] = true;
  JsonObject data = out.createNestedObject("data");
  data["ph"] = ph;
  data["tempC"] = tc;
  JsonObject js = data.createNestedObject("level_sensors");
  js["h2o"] = h2o;
  js["kcl"] = kcl;
  data["result"] = res;
  sendJson_(out);

  remoteManager.log(String("[UART] TX get_last -> ph=") + ph +
             " tempC=" + tc +
             " h2o=" + (h2o?"1":"0") +
             " kcl=" + (kcl?"1":"0") +
             " result=" + res);
}

// --- auto_measure ---
void UARTManager::handle_auto_measure_(JsonObject) {
  if (getAutoRunning()) {
    remoteManager.log("[UART] auto_measure rechazado -> BUSY");
    sendError_("BUSY");
    return;
  }

  setAutoMeasureRequested(true);
  sendOk_();
  remoteManager.log("[UART] auto_measure aceptado -> auto_req=true");
}

// ================== Helpers ==================
void UARTManager::sendOk_() {
  StaticJsonDocument<32> out;
  out["ok"] = true;
  sendJson_(out);
  remoteManager.log("[UART] TX ok=true");
}

void UARTManager::sendError_(const char* err) {
  StaticJsonDocument<96> out;
  out["ok"] = false;
  out["error"] = err;
  sendJson_(out);
  remoteManager.log(String("[UART] TX ok=false error=") + err);
}

void UARTManager::sendJson_(const JsonDocument& doc) {
  // Serializa a un String y envía con println -> agrega \r\n como Serial.println
  String payload;
  serializeJson(doc, payload);
  io_.println(payload);
}


} // namespace UartProto
