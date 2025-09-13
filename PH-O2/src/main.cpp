#include <Arduino.h>
#include "test_board.h"
#include "WiFiPortalManager.h"
#include "pH_manager.h"
#include "menu_manager.h"
#include "eeprom_manager.h"
#include "pumps_manager.h"
#include <globals.h>

TaskHandle_t TaskCore0;
TaskHandle_t TaskCore1;


WiFiPortalManager wifiManager(TELNET_HOSTNAME, "12345678", SW_DOWN);

PumpsManager pumps;
PHManager ph(&ads, /*channel=*/0, /*avgSamples=*/6);
ConfigStore eeprom;



void initDualCore();
void initHardware();
void initWiFi();
void initThermo();
void initLCD();
void initADC();
void initPH();
void initEEPROM();
void initPumps();
float readThermo();
float readADC();
float readPH();

void APIUI();
static void MenuDemoTick();

void setup() {
  Serial.begin(115200);
  initHardware();
  initPumps();
  initDualCore();
  initEEPROM();
  initWiFi();
  initThermo();
  initLCD();
  initADC();
  initPH();

}

void loop() {
  delay(100);
}

void taskCore0(void *pvParameters) {
  for (;;) {
    // TestBoard::testPumps(PUMP_1, PUMP_2, PUMP_3, PUMP_4, MIXER);
    // readThermo();
    // TestBoard::testButtons();
    // readADS();
    // readPH();
    // delay(1000);
    // delay(100);
    MenuDemoTick();

    vTaskDelay(200 / portTICK_PERIOD_MS); // Espera 100 ms
  }
}
//loop
void taskCore1(void *pvParameters) {
  for (;;) {
    wifiManager.loop();
    remoteManager.handle();
    vTaskDelay(200 / portTICK_PERIOD_MS); // Espera 100 ms
  }
}

void initDualCore() {
  // Crear tarea para Core 0
  xTaskCreatePinnedToCore(
    taskCore0,        // función
    "TaskCore0",      // nombre
    4096,             // stack size en palabras
    NULL,             // parámetros
    1,                // prioridad
    &TaskCore0,       // handle
    0                 // core (0 ó 1)
  );

  // Crear tarea para Core 1
  xTaskCreatePinnedToCore(
    taskCore1,
    "TaskCore1",
    4096,
    NULL,
    1,
    &TaskCore1,
    1                 // core 1
  );
}

void initHardware() {
  pinMode(SW_DOWN, INPUT);
  pinMode(SW_UP, INPUT);
  pinMode(SW_OK, INPUT);
  pinMode(SW_ESC, INPUT);
  // pinMode(SENSOR_LEVEL_H2O, INPUT);
  // pinMode(SENSOR_LEVEL_KCL, INPUT);
  pinMode(BUZZER, OUTPUT);
  // pinMode(PUMP_1_PIN, OUTPUT);
  // pinMode(PUMP_2_PIN, OUTPUT);
  // pinMode(PUMP_3_PIN, OUTPUT);
  // pinMode(PUMP_4_PIN, OUTPUT);
  // pinMode(MIXER_PIN, OUTPUT);

  digitalWrite(BUZZER, LOW);
  // digitalWrite(PUMP_1_PIN, LOW);
  // digitalWrite(PUMP_2_PIN, LOW);
  // digitalWrite(PUMP_3_PIN, LOW);
  // digitalWrite(PUMP_4_PIN, LOW);
  digitalWrite(MIXER_PIN, LOW);
}

void initWiFi() {
  wifiManager.begin();
  if (wifiManager.isConnected())
  {
    remoteManager.begin(); // Solo si hay WiFi
  }
}

void initThermo(){
  uint8_t n = thermo.begin();
  remoteManager.log("DS18B20 encontrados: " + String(n));
}

void initLCD(){
  lcd.begin();
  lcd.setBacklight(true);
  lcd.clear();

  lcd.centerPrint(0, "IUDigital");
  lcd.centerPrint(1, "de Antioquia");
  delay(2000);
  lcd.clear();
  lcd.centerPrint(0, "ph - O2 metter");
  lcd.centerPrint(1, "v1.0");
  delay(2000);
}

void initADC() {
  if (!ads.begin()) {
    remoteManager.log(String("ADS1115 no responde: ") + ads.lastError());
    return;
  }
  // Rango y tasa recomendados (tu versión usa uint16_t en set/getDataRate)
  ads.setGain(GAIN_ONE);                   // ±4.096V (1 LSB ≈ 0.125 mV)
  ads.setDataRate(RATE_ADS1115_250SPS);    // RATE_ADS1115_250SPS 250 SPS (uint16_t)
  ads.setAveraging(4);    
  
  float m, b;// suaviza ruido promediando 4 lecturas
  eeprom.getADC(m,b);
  ads.setCalibration(m, b);     

  remoteManager.log("ADS1115 listo (GAIN_ONE, 250SPS)");
}

void initPH() {
  ph.begin();

  // Cargar calibración pH (V7, V4, tC) desde EEPROM
  float V7 = NAN, V4 = NAN, tCalC = NAN;
  eeprom.getPH2pt(V7, V4, tCalC);

  const bool v7ok = !isnan(V7);
  const bool v4ok = !isnan(V4);
  bool tcok = (!isnan(tCalC) && tCalC > -40.0f && tCalC < 125.0f);

  if (v7ok && v4ok) {
    if (!tcok) tCalC = 25.0f;  // fallback razonable si no hay T guardada
    ph.setTwoPointCalibration(7.00f, V7, 4.00f, V4, tCalC);

    // Log de verificación
    remoteManager.log(String("pH cal cargada: V7=") + String(V7, 5) +
                      " V4=" + String(V4, 5) +
                      " Tcal=" + String(tCalC, 1) + "C");
  } else {
    // No hay datos persistidos aún: dejar sin cal específica o valores por defecto
    remoteManager.log("pH cal: no hay V7/V4 en EEPROM. Usa defaults o ejecuta cal.");
    // Si tu clase 'ph' requiere un estado definido, puedes fijar un default aquí:
    ph.setTwoPointCalibration(7.00f, 1.650f, 4.00f, 1.650f, 25.0f); // (ejemplo neutro, opcional)
  }
}

void initEEPROM(){
  eeprom.begin(256);
  if (!eeprom.load()) { eeprom.resetDefaults(); eeprom.save(); }

  // float m,b;
  // eeprom.getADC(m,b);
  // ads.setCalibration(m,b);
}

void initPumps(){
  //pump1 = KCL
  //pump2 = H2O
  //pump3 = SAMPLE
  //pump4 = DRAIN
  //mixer = MIXER
  pumps.begin(PUMP_1_PIN, PUMP_2_PIN, PUMP_3_PIN, PUMP_4_PIN, MIXER_PIN, /*activeHigh(bombas)=*/true,
              /*mixerInverted=*/true);
  pumps.beginLevels(SENSOR_LEVEL_H2O, SENSOR_LEVEL_KCL, /*usePullup=*/false);

  pumps.allOff();

}

float readADC() {
  // Lee A0..A3 single-ended
  // for (uint8_t ch = 0; ch < 4; ch++) {
  //   float v;
  //   if (ads.readSingle(ch, v)) {
  //     remoteManager.log("A" + String(ch) + ": " + String(v, 6) + " V");
  //   } else {
  //     remoteManager.log(String("ADS err A") + ch + ": " + ads.lastError());
  //   }
  // }
  float v;
  if (ads.readSingle(0, v)) {
        remoteManager.log("A0" ": " + String(v, 6) + " V");
        return v;
      } else {
        remoteManager.log(String("ADS err A0")  + ": " + ads.lastError());
        return -1;
      }
  }

float readThermo(){
   if (thermo.sensorCount() == 0) {
    // Serial.println("Sin sensores");
    remoteManager.log("Sin sensores");
    // delay(1000);
    return -1;
  }
  float c = thermo.readC(0);
  if (isnan(c)) {
    // Serial.println("Lectura inválida");
    remoteManager.log("Lectura inválida");
    return -1;
  } else {
    // Serial.printf("T0: %.2f °C\n", c);
    remoteManager.log("T: " + String(c) + " °C");
    return c;
  }
  // delay(1000);
}

float readPH() {
  float tC = readThermo();   // compensa por temperatura si tu 'ph.readPH' lo hace
  float phValue = NAN, volts = NAN;

  if (ph.readPH(tC, phValue, &volts)) {
    remoteManager.log("pH = " + String(phValue, 3) + "   V = " + String(volts, 4));
    return phValue;
  } else {
    remoteManager.log(String("pH ERR: ") + ph.lastError());
    return -99.0f;
  }
}

void APIUI() {
  Buttons::testButtons();

  // Determina un único botón activo (latch); prioridad: OK > ESC > UP > DOWN
  enum class Btn : uint8_t { NONE, OK, ESC, UP, DOWN };
  Btn active = Btn::NONE;

  if (Buttons::BTN_OK.value)      active = Btn::OK;
  else if (Buttons::BTN_ESC.value) active = Btn::ESC;
  else if (Buttons::BTN_UP.value)  active = Btn::UP;
  else if (Buttons::BTN_DOWN.value)active = Btn::DOWN;

  switch (active) {
    case Btn::OK:
      lcd.splash("IUDigital", "de Antioquia", 1000);
      Buttons::BTN_OK.reset();     // libera el latch
      break;

    case Btn::ESC:
      lcd.clear();
      Buttons::BTN_ESC.reset();    // libera el latch
      break;

    case Btn::UP:
      lcd.splash("pH - O2 meter", "v1.0", 1000);
      Buttons::BTN_UP.reset();     // libera el latch
      break;

    case Btn::DOWN:
      for (int i = 0; i <= 100; i += 10) {
        lcd.progressBar(1, i);
        delay(100);
      }
      Buttons::BTN_DOWN.reset();   // libera el latch
      break;

    case Btn::NONE:
    default:
      // no hay botón latcheado
      break;
  }
}

// Calibración a 2 puntos del ADS usando tu ADS1115Manager (single-ended).
// Puntos: 0.00 V y 3.31 V. Captura lecturas *sin calibración* y calcula
// scale/offset para que: V_real = scale * V_meas + offset.
//
// Modo de uso: llama a esta función en cada tick cuando el usuario elija
// “Configuración → Calibrar ADS”. La función maneja la UI paso a paso.
// Devuelve true cuando termina (éxito o cancelado) para que regreses al menú.
static bool runADSCalibration_0V_3p31V(uint8_t channel = 0, uint8_t samples = 32) {
  enum class Step : uint8_t { START, WAIT_ZERO, CAPT_ZERO, WAIT_VREF, CAPT_VREF, APPLY, DONE, CANCEL };
  static Step step = Step::START;

  static float v_meas_0   = 0.0f;   // medición SIN cal a 0 V
  static float v_meas_ref = 0.0f;   // medición SIN cal a 3.31 V

  static constexpr float V_ACT_0   = 0.00f;
  static constexpr float V_ACT_REF = 3.31f;
  static constexpr float kEps      = 1e-6f;

  // --- UI helpers ---
  auto showStep0 = []() {
    lcd.printAt(0,0, "Calibrar ADS");
    lcd.printAt(0,1, "0V: GND y OK");
  };
  auto showStepRef = []() {
    lcd.printAt(0,0, "Calibrar ADS");
    lcd.printAt(0,1, "Vref=3.31V y OK");
  };
  auto showBusy = [](const char* msg) {
    lcd.printAt(0,0, "Calibrar ADS");
    lcd.printAt(0,1, msg);
  };
  auto showError = [](const char* m1, const char* m2 = "") {
    lcd.splash(m1, m2, 900);
  };

  // Botones (latcheados)
  Buttons::testButtons();

  switch (step) {
    case Step::START: {
      // Medir "en crudo": desactiva cal temporalmente
      ads.setCalibration(1.0f, 0.0f);
      ads.setAveraging(samples);

      lcd.clear();
      showStep0();
      remoteManager.log("ADS cal: START -> 0V");
      step = Step::WAIT_ZERO;
      return false;
    }

    case Step::WAIT_ZERO:
      if (Buttons::BTN_ESC.value) { Buttons::BTN_ESC.reset(); step = Step::CANCEL; }
      else if (Buttons::BTN_OK.value) { Buttons::BTN_OK.reset(); step = Step::CAPT_ZERO; }
      return false;

    case Step::CAPT_ZERO: {
      showBusy("Midiendo 0V...");
      float v;
      if (!ads.readSingle(channel, v)) {
        remoteManager.log("ADS cal: fallo lectura 0V");
        showError("Error lectura", "0V");
        step = Step::CANCEL;
        return false;
      }
      v_meas_0 = v;  // sin cal (m=1, b=0)
      remoteManager.log(String("ADS cal: V_meas0=") + String(v_meas_0, 6));

      lcd.splash("0V capturado", "", 500);
      lcd.clear();
      showStepRef();
      step = Step::WAIT_VREF;
      return false;
    }

    case Step::WAIT_VREF:
      if (Buttons::BTN_ESC.value) { Buttons::BTN_ESC.reset(); step = Step::CANCEL; }
      else if (Buttons::BTN_OK.value) { Buttons::BTN_OK.reset(); step = Step::CAPT_VREF; }
      return false;

    case Step::CAPT_VREF: {
      showBusy("Mid. 3.31V...");
      float v;
      if (!ads.readSingle(channel, v)) {
        remoteManager.log("ADS cal: fallo lectura Vref");
        showError("Error lectura", "Vref");
        step = Step::CANCEL;
        return false;
      }
      v_meas_ref = v;   // sin cal
      remoteManager.log(String("ADS cal: V_measRef=") + String(v_meas_ref, 6));
      step = Step::APPLY;
      return false;
    }

    case Step::APPLY: {
      // V_real = scale * V_meas + offset
      const float denom = (v_meas_ref - v_meas_0);
      if (fabsf(denom) < kEps) {
        showError("Error cal ADS", "Denominador ~0");
        step = Step::CANCEL;
        return false;
      }
      const float scale  = (V_ACT_REF - V_ACT_0) / denom;
      const float offset = V_ACT_0 - scale * v_meas_0;

      // Aplica al ADS
      ads.setCalibration(scale, offset);

      // Persiste en EEPROM (usa tu instancia global 'eeprom')
      eeprom.setADC(scale, offset);
      if (!eeprom.save()) {
        remoteManager.log(String("EEPROM save fallo: ") + eeprom.lastError());
        showError("EEPROM", "Save fallo");
        step = Step::DONE;   // igual finalizamos
        return false;
      }

      remoteManager.log(String("ADS cal aplicada: m=") + String(scale, 6) +
                        " b=" + String(offset, 6));
      char l2[17]; snprintf(l2, sizeof(l2), "m=%.3f b=%.3f", scale, offset);
      lcd.splash("ADS calibrado", l2, 800);

      step = Step::DONE;
      return false;
    }

    case Step::CANCEL:
      remoteManager.log("ADS cal: CANCEL");
      showError("Calibracion", "Cancelada");
      step = Step::DONE;
      return false;

    case Step::DONE:
    default:
      step = Step::START;   // listo para próxima vez
      return true;          // informa al caller que ya terminó
  }
}

// Calibración pH 2 puntos fija: 7.00 y 4.00
// Usa ph.readPH(tC, phValue, &volts) para medir V y readThermo() para T.
// No persiste en EEPROM (por ahora). Llama cada tick en la vista "Calibrar pH".
static bool runPHCalibration_7_4(uint8_t samples = 64) {
  enum class Step : uint8_t { START, WAIT_7, CAPT_7, WAIT_4, CAPT_4, APPLY, DONE, CANCEL };
  static Step step = Step::START;

  static float V7 = NAN, V4 = NAN;    // voltajes promediados
  static float T1 = NAN, T2 = NAN;    // temperaturas (trazabilidad)

  // --- UI helpers ---
  auto title = [](){ lcd.printAt(0,0,"Calibrar pH"); };
  auto ask7  = [&](){ title(); lcd.printAt(0,1,"Buffer 7.00  OK"); };      // <-- captura [&]
  auto ask4  = [&](){ title(); lcd.printAt(0,1,"Buffer 4.00  OK"); };      // <-- captura [&]
  auto busy  = [&](const char* m){ title(); lcd.printAt(0,1,m); };          // <-- captura [&]
  auto showErr = [](const char* a, const char* b=""){ lcd.splash(a,b,900); };

  // Promedia VOLTAJE usando ph.readPH()
  auto avgVoltPH = [&](uint16_t N)->float {
    float accV = 0.0f;
    for (uint16_t i=0; i<N; ++i) {
      float tC = readThermo();                      // puede devolver -1 o NaN
      bool tOk = (!isnan(tC) && tC != -1.0f && tC > -40.0f && tC < 125.0f);
      float tUsed = tOk ? tC : 25.0f;

      float phVal = NAN;
      float v     = NAN;
      if (!ph.readPH(tUsed, phVal, &v)) {
        remoteManager.log(String("pH cal readPH ERR: ") + ph.lastError());
        return NAN;
      }
      accV += v;
      delay(4);
    }
    return accV / float(N);
  };

  // Buttons::testButtons();

  switch (step) {
    case Step::START:
      ads.setAveraging(samples);
      lcd.clear(); ask7();
      step = Step::WAIT_7;
      return false;

    case Step::WAIT_7:
      if (Buttons::BTN_ESC.value) { Buttons::BTN_ESC.reset(); step = Step::CANCEL; return false; }
      if (Buttons::BTN_OK.value)  { Buttons::BTN_OK.reset();  step = Step::CAPT_7; return false; }
      return false;

    case Step::CAPT_7: {
      busy("Midiendo pH7...");
      V7 = avgVoltPH(samples);
      T1 = readThermo();
      if (isnan(V7)) { showErr("Error lectura","pH 7.00"); step = Step::CANCEL; return false; }
      remoteManager.log(String("pH cal: V7=") + String(V7,5) + " T1=" + String(T1,1));
      lcd.splash("pH 7.00 OK","", 600);
      lcd.clear(); ask4();
      step = Step::WAIT_4;
      return false;
    }

    case Step::WAIT_4:
      if (Buttons::BTN_ESC.value) { Buttons::BTN_ESC.reset(); step = Step::CANCEL; return false; }
      if (Buttons::BTN_OK.value)  { Buttons::BTN_OK.reset();  step = Step::CAPT_4; return false; }
      return false;

    case Step::CAPT_4: {
      busy("Midiendo pH4...");
      V4 = avgVoltPH(samples);
      T2 = readThermo();
      if (isnan(V4)) { showErr("Error lectura","pH 4.00"); step = Step::CANCEL; return false; }
      remoteManager.log(String("pH cal: V4=") + String(V4,5) + " T2=" + String(T2,1));
      step = Step::APPLY;
      return false;
    }

    case Step::APPLY: {
      // Temperatura de calibración (promedio válido, o fallback 25 °C)
      float tCalC;
      bool t1ok = (!isnan(T1) && T1 > -40.0f && T1 < 125.0f);
      bool t2ok = (!isnan(T2) && T2 > -40.0f && T2 < 125.0f);
      if (t1ok && t2ok)      tCalC = 0.5f * (T1 + T2);
      else if (t1ok)         tCalC = T1;
      else if (t2ok)         tCalC = T2;
      else                   tCalC = 25.0f;

      // Aplica tu API de calibración a 2 puntos (7.00 y 4.00) con los VOLTAJES medidos
      ph.setTwoPointCalibration(7.00f, V7, 4.00f, V4, tCalC);
      extern ConfigStore eeprom;

      eeprom.setPH2pt(V7, V4, tCalC);
      if (!eeprom.save()) {
        remoteManager.log(String("EEPROM save PH2pt fallo: ") + eeprom.lastError());
      } else {
        remoteManager.log("EEPROM: PH2pt guardado");
      }

      char l2[17]; snprintf(l2, sizeof(l2), "V7=%.3f V4=%.3f", V7, V4);
      lcd.splash("pH calibrado", l2, 900);
      remoteManager.log(String("pH cal 2pt OK (Tcal=") + String(tCalC,1) + "C)");
      step = Step::DONE;
      return false;
    }

    case Step::CANCEL:
      lcd.splash("Calibracion","Cancelada", 700);
      step = Step::DONE;
      return false;

    case Step::DONE:
    default:
      step = Step::START;
      return true;
  }
}

// Selector manual de actuadores (KCL, H2O, SAMPLE, DRAIN, MIXER).
// Controles: UP/DOWN para seleccionar; OK = encender; ESC = apagar.
// No usa extern aquí: asume que `lcd`, `pumps` y `Buttons::BTN_*` existen globalmente.
// Llama esta función cuando estés en la vista "MANUAL" dentro de tu FSM.
// Devuelve true cuando el usuario quiere salir (ESC con bomba ya apagada)
static bool ManualPumpsTick() {
  struct Item { const char* name; PumpId id; };
  static const Item items[] = {
    { "KCL",    PumpId::KCL    },
    { "H2O",    PumpId::H2O    },
    { "SAMPLE", PumpId::SAMPLE },
    { "DRAIN",  PumpId::DRAIN  },
    { "MIXER",  PumpId::MIXER  },  // <- usa PumpId::Mixer si renombraste
  };
  static const uint8_t N = sizeof(items) / sizeof(items[0]);

  // Estado local persistente
  static uint8_t cursor      = 0;
  static int8_t  lastCursor  = -1;
  static bool    lastState   = false;
  static bool    needFirstDraw = true;   // <-- fuerza render al entrar

  auto render = [&](bool force=false) {
    bool st = pumps.isOn(items[cursor].id);
    if (force || (int8_t)cursor != lastCursor || st != lastState) {
      char l0[17];
      snprintf(l0, sizeof(l0), "%-10s %s", items[cursor].name, st ? "ON " : "OFF");
      lcd.printAt(0,0, l0);
      lcd.printAt(0,1, "OK:on ESC:off");
      lastCursor = (int8_t)cursor;
      lastState  = st;
    }
  };

  // Si venimos de fuera, limpia y fuerza el primer pintado
  if (needFirstDraw) {
    lcd.clear();
    needFirstDraw = false;
    render(true);
  } else {
    // Si no hay cambios, asegura que al menos una vez dibujamos
    if (lastCursor < 0) render(true);
  }

  // Botones latcheados (ya actualizados por el menú principal)
  if (Buttons::BTN_UP.value) {
    Buttons::BTN_UP.reset();
    cursor = (cursor == 0) ? (N - 1) : (cursor - 1);
    render(true);
    return false;
  }
  if (Buttons::BTN_DOWN.value) {
    Buttons::BTN_DOWN.reset();
    cursor = (cursor + 1) % N;
    render(true);
    return false;
  }
  if (Buttons::BTN_OK.value) {
    Buttons::BTN_OK.reset();
    pumps.on(items[cursor].id);   // encender
    render(true);
    return false;
  }
  if (Buttons::BTN_ESC.value) {
    Buttons::BTN_ESC.reset();
    // si está ON -> apaga y permanece; si ya está OFF -> salir
    if (pumps.isOn(items[cursor].id)) {
      pumps.off(items[cursor].id);
      render(true);
      return false;
    } else {
      // ya estaba apagada: salir al menú anterior
      needFirstDraw = true;       // <-- prepara primer render para próxima entrada
      return true;
    }
  }

  return false; // continuar en manual
}

// Muestra estado de sensores de nivel (H2O/KCL) y sale con ESC.
// Devuelve true cuando se debe volver al menú anterior.
static bool ManualLevelsTick() {
  // estado persistente local
  static bool needFirstDraw = true;
  static int8_t lastH2O = -1;  // -1 = desconocido, 0 = LOW, 1 = HIGH
  static int8_t lastKCL = -1;

  auto readAndRender = [&](bool force=false) {
    bool h2o = pumps.levelH2O();   // true si pin está HIGH (ajusta lógica según hardware)
    bool kcl = pumps.levelKCL();

    int8_t h2o_i = h2o ? 0 : 1;
    int8_t kcl_i = kcl ? 0 : 1;

    if (force || h2o_i != lastH2O || kcl_i != lastKCL) {
      char l0[17], l1[17];
      snprintf(l0, sizeof(l0), "Nivel H2O:%s", h2o ? "BAJO" : "OK");
      snprintf(l1, sizeof(l1), "Nivel KCL:%s", kcl ? "BAJO" : "OK");
      lcd.printAt(0,0, l0);
      lcd.printAt(0,1, l1);
      lastH2O = h2o_i;
      lastKCL = kcl_i;
    }
  };

  if (needFirstDraw) {
    lcd.clear();
    needFirstDraw = false;
    readAndRender(true);
  } else {
    // refresco oportunista si cambia el estado
    readAndRender(false);
  }

  // Controles: ESC = volver, OK = refrescar manual
  if (Buttons::BTN_ESC.value) {
    Buttons::BTN_ESC.reset();
    needFirstDraw = true;   // fuerza redraw en próxima entrada
    return true;            // salir a menú anterior
  }
  if (Buttons::BTN_OK.value) {
    Buttons::BTN_OK.reset();
    readAndRender(true);
  }
  if (Buttons::BTN_UP.value)   Buttons::BTN_UP.reset();   // sin acción
  if (Buttons::BTN_DOWN.value) Buttons::BTN_DOWN.reset(); // sin acción

  return false; // permanecer en esta pantalla
}


static void MenuDemoTick() {
  // --- prototipos externos que usa el menú ---
  extern ConfigStore eeprom;                               // EEPROM (ADC)
  extern bool runADSCalibration_0V_3p31V(uint8_t, uint8_t);// Calibración ADS
  extern bool runPHCalibration_7_4(uint8_t samples);       // Calibración pH
  extern float readADC();                                  // lectura ADC (V)
  extern float readPH();                                   // lectura pH
  extern float readThermo();                               // lectura temperatura
  extern bool ManualPumpsTick();

  // ----- Estado persistente (solo dentro de esta función) -----
  enum class View : uint8_t {
    ROOT,
    MANUAL_MENU,      // << nuevo: submenú Manual
    MANUAL_PUMPS,     // << nuevo: pantalla Bombas (usa ManualPumpsTick)
    MANUAL_LEVELS,    // << nuevo: pantalla Sensores de nivel
    AUTO,
    CONFIG, TEMP,
    CAL_ADS_MENU, CAL_ADS_READ, CAL_ADS_RUN,
    CAL_PH_MENU,  CAL_PH_READ,  CAL_PH_RUN
  };
  static bool     init = false;
  static View     view = View::ROOT;

  // Root
  static const char* rootItems[] = { "Manual", "Automatico", "Configuracion" };
  static const uint8_t ROOT_N = sizeof(rootItems)/sizeof(rootItems[0]);
  static uint8_t rootCursor = 0;

  // --- Manual submenu ---
  static const char* manualItems[] = { "Bombas", "Sensores Nivel" };
  static const uint8_t MAN_N = sizeof(manualItems)/sizeof(manualItems[0]);
  static uint8_t manualCursor = 0;

  // Config
  static const char* cfgItems[]  = { "pH", "O2", "Temperatura", "ADC" };
  static const uint8_t CFG_N = sizeof(cfgItems)/sizeof(cfgItems[0]);
  static uint8_t cfgCursor = 0;

  // Submenú Calibrar ADS
  static const char* adsItems[]  = { "Leer ADC", "Calibrar" };
  static const uint8_t ADS_N = sizeof(adsItems)/sizeof(adsItems[0]);
  static uint8_t adsCursor = 0;

  // Submenú Calibrar pH
  static const char* phItems[]   = { "Leer pH", "Calibrar pH" };
  static const uint8_t PH_N = sizeof(phItems)/sizeof(phItems[0]);
  static uint8_t phCursor = 0;

  // Temperatura
  static int8_t  tempOffset = 0;
  static float   lastTemp   = -1.0f;
  static unsigned long lastReadMs = 0;
  const  unsigned long TEMP_READ_PERIOD = 500; // ms

  // ----- Helpers de UI -----
  auto renderRoot = [&](){
    lcd.printAt(0,0, "Menu");
    String line = ">" + String(rootItems[rootCursor]);
    lcd.printAt(0,1, line);
  };
  auto renderManualMenu = [&](){
    lcd.printAt(0,0, "Manual");
    String line = ">" + String(manualItems[manualCursor]);
    lcd.printAt(0,1, line);
  };
  auto renderSub = [&](const char* title){
    lcd.printAt(0,0, title);
    lcd.printAt(0,1, "ESC: atras");
  };
  auto renderConfig = [&](){
    lcd.printAt(0,0, "Configuracion");
    String line = ">" + String(cfgItems[cfgCursor]);
    lcd.printAt(0,1, line);
  };
  auto renderTemp = [&](){
    String l0 = "T:";
    if (lastTemp >= 0.0f) l0 += " " + String(lastTemp, 1) + " C";
    else                  l0 += " --.- C";
    lcd.printAt(0,0, l0);
    char buf[17]; snprintf(buf, sizeof(buf), "Off:%+d", (int)tempOffset);
    lcd.printAt(0,1, buf);
  };
  auto renderADSMenu = [&](){
    lcd.printAt(0,0, "Calibrar ADS");
    String line = ">" + String(adsItems[adsCursor]);
    lcd.printAt(0,1, line);
  };
  auto renderPHMenu = [&](){
    lcd.printAt(0,0, "Calibrar pH");
    String line = ">" + String(phItems[phCursor]);
    lcd.printAt(0,1, line);
  };

  // Mostrar lectura y cal actual del ADC
  auto renderADSRead = [&](){
    float v = readADC();
    float m,b; eeprom.getADC(m,b);
    char l0[17]; snprintf(l0, sizeof(l0), "ADC: %.3f V", v);
    char l1[17]; snprintf(l1, sizeof(l1), "m%.3f b%.3f", m, b);
    lcd.printAt(0,0, l0);
    lcd.printAt(0,1, l1);
  };

  // Mostrar lectura de pH actual (usa readPH())
  auto renderPHRead = [&](){
    float phv = readPH();
    float V7s, V4s, tCs;
    eeprom.getPH2pt(V7s, V4s, tCs);
    char l0[17]; snprintf(l0, sizeof(l0), "pH: %.02f", phv);
    char l1[17]; snprintf(l1, sizeof(l1), "V7=%.3f V4=%.3f", V7s, V4s);
    lcd.printAt(0,0, l0);
    lcd.printAt(0,1, l1);
  };

  // ----- Init perezoso -----
  if (!init) {
    lcd.clear();
    renderRoot();
    init = true;
  }

  // ----- Actualiza botones (latcheados) -----
  Buttons::testButtons();

  // Lee un único botón activo (prioridad) y lo resetea
  enum class Btn : uint8_t { NONE, OK, ESC, UP, DOWN };
  auto readLatched = []() -> Btn {
    if (Buttons::BTN_OK.value)   { Buttons::BTN_OK.reset();   return Btn::OK; }
    if (Buttons::BTN_ESC.value)  { Buttons::BTN_ESC.reset();  return Btn::ESC; }
    if (Buttons::BTN_UP.value)   { Buttons::BTN_UP.reset();   return Btn::UP; }
    if (Buttons::BTN_DOWN.value) { Buttons::BTN_DOWN.reset(); return Btn::DOWN; }
    return Btn::NONE;
  };

  // --- refresco periódico solo para TEMP ---
  if (view == View::TEMP) {
    unsigned long now = millis();
    if (now - lastReadMs >= TEMP_READ_PERIOD) {
      lastReadMs = now;
      float t = readThermo();
      lastTemp = (t >= 0.0f && !isnan(t)) ? t : -1.0f;
      renderTemp();
    }
  }

  // ----- FSM del menú -----
  switch (view) {
    case View::ROOT: {
      switch (readLatched()) {
        case Btn::UP:
          rootCursor = (rootCursor==0)? (ROOT_N-1) : (rootCursor-1);
          lcd.clear(); renderRoot(); break;
        case Btn::DOWN:
          rootCursor = (rootCursor+1) % ROOT_N;
          lcd.clear(); renderRoot(); break;
        case Btn::OK:
          lcd.clear();
          if      (rootCursor==0) { view = View::MANUAL_MENU; renderManualMenu(); }
          else if (rootCursor==1) { view = View::AUTO;        renderSub("Automatico"); }
          else                    { view = View::CONFIG;      renderConfig(); }
          break;
        default: break;
      }
    } break;

    // --------- Manual: Submenú (Bombas / Sensores Nivel) ----------
    case View::MANUAL_MENU: {
      switch (readLatched()) {
        case Btn::UP:
          manualCursor = (manualCursor==0) ? (MAN_N-1) : (manualCursor-1);
          lcd.clear(); renderManualMenu(); break;
        case Btn::DOWN:
          manualCursor = (manualCursor+1) % MAN_N;
          lcd.clear(); renderManualMenu(); break;
        case Btn::OK:
          lcd.clear();
          if (manualCursor == 0) {
            view = View::MANUAL_PUMPS;
          } else {
            // Entrar a Sensores Nivel
            view = View::MANUAL_LEVELS;
            // pintado inicial
            lcd.printAt(0,0, "Sensores nivel");
            lcd.printAt(0,1, "ESC: atras");
          }
          break;
        case Btn::ESC:
          view = View::ROOT; lcd.clear(); renderRoot(); break;
        default: break;
      }
    } break;

    // --------- Manual: Bombas (usa ManualPumpsTick) ----------
    case View::MANUAL_PUMPS: {
      static bool first = true;
      if (first) { lcd.clear(); first = false; }   // pantalla limpia al entrar
      if (ManualPumpsTick()) {
        view = View::MANUAL_MENU;
        lcd.clear();
        renderManualMenu();
        first = true;                               // para que la próxima entrada limpie
      }
    }  break;

    // --------- Manual: Sensores de nivel ----------
    case View::MANUAL_LEVELS: {
      if (ManualLevelsTick()) {
        view = View::MANUAL_MENU;
        lcd.clear();
        renderManualMenu();
      }
    } break;

    // --------- Automático ----------
    case View::AUTO: {
      static bool autoRunning = false;
      switch (readLatched()) {
        case Btn::OK:
          if (!autoRunning) { autoRunning = true;  lcd.splash("Automatico","Iniciado", 500); }
          else              { autoRunning = false; lcd.splash("Automatico","Detenido", 500); }
          renderSub("Automatico"); break;
        case Btn::ESC:
          view = View::ROOT; lcd.clear(); renderRoot(); break;
        default: break;
      }
    } break;

    // --------- Configuración ----------
    case View::CONFIG: {
      switch (readLatched()) {
        case Btn::UP:
          cfgCursor = (cfgCursor==0)? (CFG_N-1) : (cfgCursor-1);
          lcd.clear(); renderConfig(); break;
        case Btn::DOWN:
          cfgCursor = (cfgCursor+1) % CFG_N;
          lcd.clear(); renderConfig(); break;
        case Btn::OK:
          lcd.clear();
          if      (cfgCursor==0) { view = View::CAL_PH_MENU;  renderPHMenu(); }
          else if (cfgCursor==1) { lcd.splash("Calibrar O2","Pendiente", 700); renderConfig(); }
          else if (cfgCursor==2) { view = View::TEMP;         renderTemp(); }
          else                   { view = View::CAL_ADS_MENU; renderADSMenu(); }
          break;
        case Btn::ESC:
          view = View::ROOT; lcd.clear(); renderRoot(); break;
        default: break;
      }
    } break;

    // ---- Submenú: Calibrar ADS ----
    case View::CAL_ADS_MENU: {
      switch (readLatched()) {
        case Btn::UP:
          adsCursor = (adsCursor==0)? (ADS_N-1) : (adsCursor-1);
          lcd.clear(); renderADSMenu(); break;
        case Btn::DOWN:
          adsCursor = (adsCursor+1) % ADS_N;
          lcd.clear(); renderADSMenu(); break;
        case Btn::OK:
          if (adsCursor == 0) {           // Leer ADC
            lcd.clear(); renderADSRead();
            view = View::CAL_ADS_READ;
          } else {                         // Calibrar
            lcd.clear();
            view = View::CAL_ADS_RUN;
          }
          break;
        case Btn::ESC:
          view = View::CONFIG; lcd.clear(); renderConfig(); break;
        default: break;
      }
    } break;

    // Pantalla de lectura ADC (OK refresca, ESC vuelve)
    case View::CAL_ADS_READ: {
      switch (readLatched()) {
        case Btn::OK:  lcd.clear(); renderADSRead(); break;
        case Btn::ESC: view = View::CAL_ADS_MENU; lcd.clear(); renderADSMenu(); break;
        default: break;
      }
    } break;

    // Rutina de calibración ADS
    case View::CAL_ADS_RUN: {
      if (runADSCalibration_0V_3p31V(/*channel=*/0, /*samples=*/32)) {
        view = View::CAL_ADS_MENU; lcd.clear(); renderADSMenu();
      }
    } break;

    // ---- Submenú: Calibrar pH ----
    case View::CAL_PH_MENU: {
      switch (readLatched()) {
        case Btn::UP:
          phCursor = (phCursor==0)? (PH_N-1) : (phCursor-1);
          lcd.clear(); renderPHMenu(); break;
        case Btn::DOWN:
          phCursor = (phCursor+1) % PH_N;
          lcd.clear(); renderPHMenu(); break;
        case Btn::OK:
          if (phCursor == 0) {            // Leer pH
            lcd.clear(); renderPHRead();
            view = View::CAL_PH_READ;
          } else {                         // Calibrar pH
            lcd.clear();
            view = View::CAL_PH_RUN;
          }
          break;
        case Btn::ESC:
          view = View::CONFIG; lcd.clear(); renderConfig(); break;
        default: break;
      }
    } break;

    // Pantalla de lectura pH (OK refresca, ESC vuelve)
    case View::CAL_PH_READ: {
      switch (readLatched()) {
        case Btn::OK:  lcd.clear(); renderPHRead(); break;
        case Btn::ESC: view = View::CAL_PH_MENU; lcd.clear(); renderPHMenu(); break;
        default: break;
      }
    } break;

    // Rutina de calibración pH
    case View::CAL_PH_RUN: {
      if (runPHCalibration_7_4(/*samples=*/5)) {
        view = View::CAL_PH_MENU; lcd.clear(); renderPHMenu();
      }
    } break;

    case View::TEMP: {
      switch (readLatched()) {
        case Btn::UP:
          if (tempOffset < 10) ++tempOffset;
          lcd.clear(); renderTemp(); break;
        case Btn::DOWN:
          if (tempOffset > -10) --tempOffset;
          lcd.clear(); renderTemp(); break;
        case Btn::OK:
          lcd.splash("Temp offset","Guardado", 600);
          view = View::CONFIG; lcd.clear(); renderConfig(); break;
        case Btn::ESC:
          view = View::CONFIG; lcd.clear(); renderConfig(); break;
        default: break;
      }
    } break;
  }
}
