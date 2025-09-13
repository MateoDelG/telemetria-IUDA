#include <Arduino.h>
#include "test_board.h"
#include "WiFiPortalManager.h"
#include "pH_manager.h"
#include "menu_manager.h"
#include <globals.h>

TaskHandle_t TaskCore0;
TaskHandle_t TaskCore1;


WiFiPortalManager wifiManager(TELNET_HOSTNAME, "12345678", SW_DOWN);

PHManager ph(&ads, /*channel=*/0, /*avgSamples=*/8);


void initDualCore();
void initHardware();
void initWiFi();
void initThermo();
void initLCD();
void initADS();
void initPH();
float readThermo();
void readADS();
void readPH();

void APIUI();
static void MenuDemoTick();

void setup() {
  Serial.begin(115200);
  initHardware();
  initDualCore();
  initWiFi();
  initThermo();
  initLCD();
  initADS();
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
  pinMode(SENSOR_LEVEL_H2O, INPUT);
  pinMode(SENSOR_LEVEL_KCL, INPUT);
  pinMode(BUZZER, OUTPUT);
  pinMode(PUMP_1, OUTPUT);
  pinMode(PUMP_2, OUTPUT);
  pinMode(PUMP_3, OUTPUT);
  pinMode(PUMP_4, OUTPUT);
  pinMode(MIXER, OUTPUT);

  digitalWrite(BUZZER, LOW);
  digitalWrite(PUMP_1, LOW);
  digitalWrite(PUMP_2, LOW);
  digitalWrite(PUMP_3, LOW);
  digitalWrite(PUMP_4, LOW);
  digitalWrite(MIXER, LOW);
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

void initADS() {
  if (!ads.begin()) {
    remoteManager.log(String("ADS1115 no responde: ") + ads.lastError());
    return;
  }
  // Rango y tasa recomendados (tu versión usa uint16_t en set/getDataRate)
  ads.setGain(GAIN_ONE);                   // ±4.096V (1 LSB ≈ 0.125 mV)
  ads.setDataRate(RATE_ADS1115_250SPS);    // RATE_ADS1115_250SPS 250 SPS (uint16_t)
  ads.setAveraging(4);                     // suaviza ruido promediando 4 lecturas
  ads.setCalibration(1.281f, -0.0075f);     

  remoteManager.log("ADS1115 listo (GAIN_ONE, 250SPS)");
}

void initPH(){
  ph.begin();
  // --- Calibración de 2 puntos (a 25 °C, por ejemplo) ---
// Mide y anota el voltaje en buffer pH 7.00 y pH 4.00:
  float V7 =  0.719307;  // <-- ejemplo, mide de verdad con ads.readSingle(0, V7)
  float V4 = 1.7748;  // <-- ejemplo
  ph.setTwoPointCalibration(7.00, V7, 4.00, V4, /*tCalC=*/28.81);
}

void readADS() {
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
      } else {
        remoteManager.log(String("ADS err A0")  + ": " + ads.lastError());
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

void readPH(){
float tC = readThermo();  
float phValue, volts;
if (ph.readPH(tC, phValue, &volts)) {
  remoteManager.log("pH = " + String(phValue, 3) + "   V = " + String(volts, 4));
} else {
  remoteManager.log(String("pH ERR: ") + ph.lastError());
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

  static float v_meas_0 = 0.0f;
  static float v_meas_ref = 0.0f;

  static constexpr float V_ACT_0   = 0.00f;
  static constexpr float V_ACT_REF = 3.31f;
  static constexpr float kEps      = 1e-6f;   // <— nombre distinto a EPS

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
  auto showError = [](const char* msg1, const char* msg2 = "") {
    lcd.splash(msg1, msg2, 900);
  };

  Buttons::testButtons();

  switch (step) {
    case Step::START: {
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
      v_meas_0 = v;
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
      v_meas_ref = v;
      remoteManager.log(String("ADS cal: V_measRef=") + String(v_meas_ref, 6));
      step = Step::APPLY;
      return false;
    }

    case Step::APPLY: {
      const float denom = (v_meas_ref - v_meas_0);
      if (fabsf(denom) < kEps) {   // <— usar kEps
        showError("Error cal ADS", "Denominador ~0");
        step = Step::CANCEL;
        return false;
      }
      const float scale  = (V_ACT_REF - V_ACT_0) / denom;
      const float offset = V_ACT_0 - scale * v_meas_0;

      ads.setCalibration(scale, offset);

      remoteManager.log(String("ADS cal aplicada: scale=") + String(scale, 6) +
                        " offset=" + String(offset, 6));
      char l2[17];
      snprintf(l2, sizeof(l2), "m=%.3f b=%.3f", scale, offset);
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
      step = Step::START;
      return true;
  }
}


static void MenuDemoTick() {
  // ----- Estado persistente (solo dentro de esta función) -----
  enum class View : uint8_t { ROOT, MANUAL, AUTO, CONFIG, TEMP, CAL_ADS };
  static bool     init = false;
  static View     view = View::ROOT;

  // Root
  static const char* rootItems[] = { "Manual", "Automatico", "Configuracion" };
  static const uint8_t ROOT_N = sizeof(rootItems)/sizeof(rootItems[0]);
  static uint8_t rootCursor = 0;

  // Config
  static const char* cfgItems[]  = { "Calibrar pH", "Calibrar O2", "Temperatura", "Calibrar ADS" };
  static const uint8_t CFG_N = sizeof(cfgItems)/sizeof(cfgItems[0]);
  static uint8_t cfgCursor = 0;

  // Temperatura
  static int8_t  tempOffset = 0;          // offset editable [-10..+10]
  static float   lastTemp   = -1.0f;      // última lectura válida o -1 si error
  static unsigned long lastReadMs = 0;    // para espaciar lecturas
  const  unsigned long TEMP_READ_PERIOD = 500; // ms

  // ----- Helpers de UI -----
  auto renderRoot = [&](){
    lcd.printAt(0,0, "Menu");
    String line = ">" + String(rootItems[rootCursor]);
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
    // Línea 0: temperatura actual
    String l0 = "T:";
    if (lastTemp >= 0.0f) l0 += " " + String(lastTemp, 1) + " C";
    else                  l0 += " --.- C";
    lcd.printAt(0,0, l0);

    // Línea 1: offset con signo
    char buf[17];
    snprintf(buf, sizeof(buf), "Off:%+d", (int)tempOffset);
    lcd.printAt(0,1, buf); // UP/DOWN cambian, OK guarda, ESC vuelve
  };

  // Acciones demo (puedes reemplazar por tus rutinas reales)
  auto doCalPH = [&](){
    lcd.printAt(0,0, "Calibrar pH");
    lcd.printAt(0,1, "pH7 -> pH4/10");
    delay(600);
    for (int i=0;i<=100;i+=20){ lcd.progressBar(1,i); delay(120); }
    lcd.splash("pH calibrado","OK", 650);
  };
  auto doCalO2 = [&](){
    lcd.printAt(0,0, "Calibrar O2");
    lcd.printAt(0,1, "Aire 100%");
    delay(600);
    for (int i=0;i<=100;i+=20){ lcd.progressBar(1,i); delay(120); }
    lcd.splash("O2 calibrado","OK", 650);
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

  // ----- Si estamos en TEMP, refrescar lectura periódicamente -----
  if (view == View::TEMP) {
    unsigned long now = millis();
    if (now - lastReadMs >= TEMP_READ_PERIOD) {
      lastReadMs = now;
      float t = readThermo();                        // tu función de lectura
      lastTemp = (t >= 0.0f && !isnan(t)) ? t : -1.0f;
      renderTemp();                                  // repinta con el nuevo valor
    }
  }

  // ----- FSM del menú -----
  switch (view) {
    case View::ROOT: {
      switch (readLatched()) {
        case Btn::UP:
          rootCursor = (rootCursor==0)? (ROOT_N-1) : (rootCursor-1);
          lcd.clear(); renderRoot();
          break;
        case Btn::DOWN:
          rootCursor = (rootCursor+1) % ROOT_N;
          lcd.clear(); renderRoot();
          break;
        case Btn::OK:
          lcd.clear();
          if      (rootCursor==0) { view = View::MANUAL; renderSub("Manual"); }
          else if (rootCursor==1) { view = View::AUTO;   renderSub("Automatico"); }
          else                    { view = View::CONFIG; renderConfig(); }
          break;
        default: break;
      }
    } break;

    case View::MANUAL: {
      switch (readLatched()) {
        case Btn::OK:
          // Aquí tu función de medición manual
          lcd.splash("Manual","Medicion...", 600);
          renderSub("Manual");
          break;
        case Btn::ESC:
          view = View::ROOT; lcd.clear(); renderRoot();
          break;
        default: break;
      }
    } break;

    case View::AUTO: {
      static bool autoRunning = false;
      switch (readLatched()) {
        case Btn::OK:
          if (!autoRunning) {
            // iniciar automático
            autoRunning = true;
            lcd.splash("Automatico","Iniciado", 500);
          } else {
            // detener automático
            autoRunning = false;
            lcd.splash("Automatico","Detenido", 500);
          }
          renderSub("Automatico");
          break;
        case Btn::ESC:
          view = View::ROOT; lcd.clear(); renderRoot();
          break;
        default: break;
      }
    } break;

    case View::CONFIG: {
      switch (readLatched()) {
        case Btn::UP:
          cfgCursor = (cfgCursor==0)? (CFG_N-1) : (cfgCursor-1);
          lcd.clear(); renderConfig();
          break;
        case Btn::DOWN:
          cfgCursor = (cfgCursor+1) % CFG_N;
          lcd.clear(); renderConfig();
          break;
        case Btn::OK:
          lcd.clear();
          if      (cfgCursor==0) { doCalPH();  renderConfig(); }
          else if (cfgCursor==1) { doCalO2();  renderConfig(); }
          else if (cfgCursor==2) { view = View::TEMP; renderTemp(); }
          else                   { view = View::CAL_ADS; /* la vista maneja su UI */ }
          break;
        case Btn::ESC:
          view = View::ROOT; lcd.clear(); renderRoot();
          break;
        default: break;
      }
    } break;

    case View::TEMP: {
      switch (readLatched()) {
        case Btn::UP:
          if (tempOffset < 10) ++tempOffset;
          lcd.clear(); renderTemp();
          break;
        case Btn::DOWN:
          if (tempOffset > -10) --tempOffset;
          lcd.clear(); renderTemp();
          break;
        case Btn::OK:
          // Guardar offset (EEPROM/NVS) si aplica
          lcd.splash("Temp offset","Guardado", 600);
          view = View::CONFIG; lcd.clear(); renderConfig();
          break;
        case Btn::ESC:
          view = View::CONFIG; lcd.clear(); renderConfig();
          break;
        default: break;
      }
    } break;

    case View::CAL_ADS: {  // --- Calibración ADS interactiva a 0 V y 3.31 V ---
      // Todo el estado de la calibración vive aquí
      enum class Step : uint8_t { START, WAIT_ZERO, CAPT_ZERO, WAIT_VREF, CAPT_VREF, APPLY, DONE, CANCEL };
      static Step step = Step::START;

      // Medidas sin calibración
      static float v_meas_0   = 0.0f;
      static float v_meas_ref = 0.0f;

      // Parámetros de calibración
      static constexpr float V_ACT_0   = 0.00f;
      static constexpr float V_ACT_REF = 3.31f;
      static constexpr float kEps      = 1e-6f;
      static const uint8_t   kChannel  = 0;
      static const uint8_t   kSamples  = 32;

      auto showStep0 = [](){
        lcd.printAt(0,0, "Calibrar ADS");
        lcd.printAt(0,1, "0V: GND y OK");
      };
      auto showStepRef = [](){
        lcd.printAt(0,0, "Calibrar ADS");
        lcd.printAt(0,1, "Vref=3.31V y OK");
      };
      auto showBusy = [](const char* msg){
        lcd.printAt(0,0, "Calibrar ADS");
        lcd.printAt(0,1, msg);
      };
      auto showError = [](const char* m1, const char* m2=""){
        lcd.splash(m1, m2, 900);
      };

      // Procesa inputs en esta vista
      Btn btn = readLatched();

      switch (step) {
        case Step::START: {
          ads.setCalibration(1.0f, 0.0f);   // medir "crudo" en voltios sin cal previa
          ads.setAveraging(kSamples);
          lcd.clear(); showStep0();
          remoteManager.log("ADS cal: START -> 0V");
          step = Step::WAIT_ZERO;
        } break;

        case Step::WAIT_ZERO: {
          if (btn == Btn::ESC) { step = Step::CANCEL; }
          else if (btn == Btn::OK) { step = Step::CAPT_ZERO; }
        } break;

        case Step::CAPT_ZERO: {
          showBusy("Midiendo 0V...");
          float v;
          if (!ads.readSingle(kChannel, v)) {
            remoteManager.log("ADS cal: fallo lectura 0V");
            showError("Error lectura", "0V");
            step = Step::CANCEL;
            break;
          }
          v_meas_0 = v;
          remoteManager.log(String("ADS cal: V_meas0=") + String(v_meas_0, 6));
          lcd.splash("0V capturado","", 500);
          lcd.clear(); showStepRef();
          step = Step::WAIT_VREF;
        } break;

        case Step::WAIT_VREF: {
          if (btn == Btn::ESC) { step = Step::CANCEL; }
          else if (btn == Btn::OK) { step = Step::CAPT_VREF; }
        } break;

        case Step::CAPT_VREF: {
          showBusy("Mid. 3.31V...");
          float v;
          if (!ads.readSingle(kChannel, v)) {
            remoteManager.log("ADS cal: fallo lectura Vref");
            showError("Error lectura", "Vref");
            step = Step::CANCEL;
            break;
          }
          v_meas_ref = v;
          remoteManager.log(String("ADS cal: V_measRef=") + String(v_meas_ref, 6));
          step = Step::APPLY;
        } break;

        case Step::APPLY: {
          const float denom = (v_meas_ref - v_meas_0);
          if (fabsf(denom) < kEps) {
            showError("Error cal ADS", "Denominador ~0");
            step = Step::CANCEL;
            break;
          }
          const float scale  = (V_ACT_REF - V_ACT_0) / denom;
          const float offset = V_ACT_0 - scale * v_meas_0;

          ads.setCalibration(scale, offset);

          remoteManager.log(String("ADS cal aplicada: scale=") + String(scale, 6) +
                            " offset=" + String(offset, 6));
          char l2[17]; snprintf(l2, sizeof(l2), "m=%.3f b=%.3f", scale, offset);
          lcd.splash("ADS calibrado", l2, 800);

          step = Step::DONE;
        } break;

        case Step::CANCEL: {
          remoteManager.log("ADS cal: CANCEL");
          showError("Calibracion", "Cancelada");
          step = Step::DONE;
        } break;

        case Step::DONE:
        default: {
          // Volver a Configuración y preparar para próxima vez
          step = Step::START;
          view = View::CONFIG; lcd.clear(); renderConfig();
        } break;
      }
    } break;
  }
}
