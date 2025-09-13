#include <Arduino.h>
#include "test_board.h"
#include "WiFiPortalManager.h"
#include "pH_manager.h"
#include "menu_manager.h"
#include "eeprom_manager.h"
#include <globals.h>

TaskHandle_t TaskCore0;
TaskHandle_t TaskCore1;


WiFiPortalManager wifiManager(TELNET_HOSTNAME, "12345678", SW_DOWN);

PHManager ph(&ads, /*channel=*/0, /*avgSamples=*/24);
ConfigStore eeprom;



void initDualCore();
void initHardware();
void initWiFi();
void initThermo();
void initLCD();
void initADC();
void initPH();
void initEEPROM();
float readThermo();
float readADC();
float readPH();

void APIUI();
static void MenuDemoTick();

void setup() {
  Serial.begin(115200);
  initHardware();
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

// Lee pH hasta que 3 medidas consecutivas queden dentro de un rango de 0.5 pH.
// Si la ventana de 3 es inestable, la “rearma” con el promedio de las tres
// anteriores siguiendo el patrón solicitado:
//   [a,b,c] -> avg0=(a+b+c)/3
//   1ª inestabilidad: [avg0, avg0, nueva]
//   2ª inestabilidad: [avg_prev, avg0, nueva]
// Devuelve el promedio de las 3 lecturas estables, o el último válido si no se logra.
// float readPH() {
//   const float    THRESH_PH       = 0.5f;    // rango máx entre 3 lecturas
//   const uint8_t  NEED            = 3;       // tamaño de ventana
//   const uint8_t  MAX_TRIES       = 12;      // límite de intentos
//   const uint16_t SAMPLE_DELAY_MS = 50;    // espera entre lecturas

//   float phBuf[NEED] = { NAN, NAN, NAN };
//   float vBuf [NEED] = { NAN, NAN, NAN };
//   uint8_t have = 0;

//   float lastValidPh = NAN, lastValidV = NAN;
//   float lastAvg = NAN;  // guarda el último promedio usado para rearmar ventana
//   char  lastErr[64] = {0};

//   for (uint8_t i = 0; i < MAX_TRIES; ++i) {
//     // Temperatura con fallback
//     float tC = readThermo();
//     bool  tOk = (!isnan(tC) && tC != -1.0f && tC > -40.0f && tC < 125.0f);
//     float tUsed = tOk ? tC : 25.0f;

//     // Lectura pH + volts
//     float phValue = NAN, volts = NAN;
//     if (!ph.readPH(tUsed, phValue, &volts)) {
//       strncpy(lastErr, ph.lastError(), sizeof(lastErr)-1);
//       delay(SAMPLE_DELAY_MS);
//       continue;
//     }

//     lastValidPh = phValue;
//     lastValidV  = volts;

//     // Ventana deslizante (si estamos rearmando, 'have' puede ser 2)
//     if (have < NEED) {
//       phBuf[have] = phValue;
//       vBuf [have] = volts;
//       have++;
//     } else {
//       phBuf[0] = phBuf[1];
//       phBuf[1] = phBuf[2];
//       phBuf[2] = phValue;

//       vBuf[0]  = vBuf[1];
//       vBuf[1]  = vBuf[2];
//       vBuf[2]  = volts;
//     }

//     if (have == NEED) {
//       // Evalúa estabilidad
//       float minPh = phBuf[0], maxPh = phBuf[0];
//       for (uint8_t k = 1; k < NEED; ++k) {
//         if (phBuf[k] < minPh) minPh = phBuf[k];
//         if (phBuf[k] > maxPh) maxPh = phBuf[k];
//       }
//       float span = maxPh - minPh;

//       if (span <= THRESH_PH) {
//         float phStable = (phBuf[0] + phBuf[1] + phBuf[2]) / 3.0f;
//         float vStable  = (vBuf [0] + vBuf [1] + vBuf [2]) / 3.0f;
//         remoteManager.log("pH = " + String(phStable, 3) +
//                           "   V = " + String(vStable, 4) +
//                           "   (estable x3, span=" + String(span, 2) + ")");
//         return phStable;
//       } else {
//         // Inestable: rearmar ventana con promedio de las 3 previas
//         float avgPh = (phBuf[0] + phBuf[1] + phBuf[2]) / 3.0f;
//         float avgV  = (vBuf [0] + vBuf [1] + vBuf [2]) / 3.0f;

//         // Patrón:
//         // - Si no hay promedio anterior, usar avg actual dos veces: [avg, avg, nueva]
//         // - Si ya hubo un promedio anterior, usar [lastAvg, avg, nueva]
//         if (isnan(lastAvg)) {
//           phBuf[0] = avgPh;
//           phBuf[1] = avgPh;
//           vBuf [0] = avgV;
//           vBuf [1] = avgV;
//         } else {
//           phBuf[0] = lastAvg;
//           phBuf[1] = avgPh;
//           vBuf [0] = lastAvg;  // aproximación: no tenemos V_avg_prev separado
//           vBuf [1] = avgV;
//         }
//         // Dejar espacio para la próxima “nueva” muestra
//         have = 2;
//         lastAvg = avgPh;

//         remoteManager.log("pH inestable(3) span=" + String(span,2) +
//                           " -> ventana=[ " + String(phBuf[0],2) + ", " +
//                                            String(phBuf[1],2) + ", nueva ]");
//       }
//     }

//     delay(SAMPLE_DELAY_MS);
//   }

//   if (!isnan(lastValidPh)) {
//     remoteManager.log("pH sin estabilidad x3, devuelvo ultimo = " +
//                       String(lastValidPh, 3) + " (V=" + String(lastValidV, 4) + ")");
//     return lastValidPh;
//   }

//   remoteManager.log(String("pH ERR: ") + (lastErr[0] ? lastErr : "sin lectura valida"));
//   return -99.0f;
// }

// // Promedio móvil de 3 muestras (estado interno persistente por llamada).
// static float ma3_update(float x, bool reset = false) {
//   static float buf[3] = {NAN, NAN, NAN};
//   static uint8_t i = 0, n = 0;
//   static float sum = 0.0f;

//   if (reset) { i = n = 0; sum = 0; buf[0] = buf[1] = buf[2] = NAN; return NAN; }

//   if (n < 3) {                // llenando
//     buf[i] = x; sum += x; i = (i + 1) % 3; n++;
//     return sum / n;
//   } else {                    // ventana llena
//     sum -= buf[i];
//     buf[i] = x;
//     sum += x;
//     i = (i + 1) % 3;
//     return sum / 3.0f;
//   }
// }

// Cada llamada empieza con un MA(3) limpio.
// Lee hasta que la diferencia entre dos MA3 consecutivos sea < 0.5 pH.
// Devuelve el MA3 estable; si no se logra, devuelve el último MA3 “full” válido,
// o -99 si nunca hubo lectura válida.
// float readPH() {
//   const float    THRESH_PH         = 0.5f;   // criterio de estabilidad
//   const uint8_t  MAX_TRIES         = 15;     // evita bucles infinitos
//   const uint16_t SAMPLE_DELAY_MS   = 1;   // espera entre lecturas

//   // ---- Promedio móvil de 3 (estado local → se “resetea” en cada llamada) ----
//   float buf[3] = {NAN, NAN, NAN};
//   float sum = 0.0f;
//   uint8_t n = 0, idx = 0;
//   auto ma3_push = [&](float x, float& ma_out, bool& full_out) {
//     if (n < 3) {
//       buf[idx] = x; sum += x; idx = (idx + 1) % 3; n++;
//       ma_out = sum / n; full_out = (n == 3);
//     } else {
//       sum -= buf[idx];
//       buf[idx] = x;
//       sum += x;
//       idx = (idx + 1) % 3;
//       ma_out = sum / 3.0f; full_out = true;
//     }
//   };

//   float lastMA = NAN;       // último MA3 “full” (para comparar Δ)
//   float lastValidMA = NAN;  // por si no se alcanza estabilidad
//   char  lastErr[64] = {0};

//   for (uint8_t t = 0; t < MAX_TRIES; ++t) {
//     // Temperatura con fallback a 25 °C si inválida
//     float tC = readThermo();
//     bool  tOk = (!isnan(tC) && tC != -1.0f && tC > -40.0f && tC < 125.0f);
//     float tUsed = tOk ? tC : 25.0f;

//     // Lectura cruda
//     float phValue = NAN, volts = NAN;
//     if (!ph.readPH(tUsed, phValue, &volts)) {
//       strncpy(lastErr, ph.lastError(), sizeof(lastErr)-1);
//       delay(SAMPLE_DELAY_MS);
//       continue;
//     }

//     // Actualiza MA3
//     float ma = NAN; bool full = false;
//     ma3_push(phValue, ma, full);

//     // Log
//     remoteManager.log("pH=" + String(phValue, 3) +
//                       "  MA3=" + String(ma, 3) +
//                       "  V=" + String(volts, 4) +
//                       "  T=" + String(tUsed, 1) +
//                       (full ? " [full]" : " [fill]"));

//     // Condición de estabilidad: dos MA3 “full” consecutivos cercanos
//     if (full) {
//       if (!isnan(lastMA)) {
//         float diff = fabsf(ma - lastMA);
//         if (diff < THRESH_PH) {
//           remoteManager.log("MA3 estable: " + String(ma, 3) +
//                             " (Δ=" + String(diff, 2) + ")");
//           return ma;
//         } else {
//           remoteManager.log("MA3 inestable: Δ=" + String(diff, 2));
//         }
//       }
//       lastMA = ma;
//       lastValidMA = ma;
//     }

//     delay(SAMPLE_DELAY_MS);
//   }

//   if (!isnan(lastValidMA)) {
//     remoteManager.log("pH sin estabilidad (<" + String(THRESH_PH,1) +
//                       "), devuelvo ultimo MA3=" + String(lastValidMA,3));
//     return lastValidMA;
//   }

//   remoteManager.log(String("pH ERR: ") + (lastErr[0] ? lastErr : "sin lectura valida"));
//   return -99.0f;
// }

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

static void MenuDemoTick() {
  // --- prototipos externos que usa el menú ---
  extern ConfigStore eeprom;                               // EEPROM (ADC)
  extern bool runADSCalibration_0V_3p31V(uint8_t, uint8_t);// Calibración ADS
  extern bool runPHCalibration_7_4(uint8_t samples);       // ← Calibración pH (ya implementada)
  extern float readADC();                                  // lectura ADC (V)
  extern float readPH();                                   // lectura pH
  extern float readThermo();                               // lectura temperatura

  // ----- Estado persistente (solo dentro de esta función) -----
  enum class View : uint8_t {
    ROOT, MANUAL, AUTO, CONFIG, TEMP,
    CAL_ADS_MENU, CAL_ADS_READ, CAL_ADS_RUN,
    CAL_PH_MENU,  CAL_PH_READ,  CAL_PH_RUN
  };
  static bool     init = false;
  static View     view = View::ROOT;

  // Root
  static const char* rootItems[] = { "Manual", "Automatico", "Configuracion" };
  static const uint8_t ROOT_N = sizeof(rootItems)/sizeof(rootItems[0]);
  static uint8_t rootCursor = 0;

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

  // ----- Si estamos en TEMP, refrescar lectura periódicamente -----
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
          lcd.splash("Manual","Medicion...", 600);
          renderSub("Manual");
          break;
        case Btn::ESC:
          view = View::ROOT; lcd.clear(); renderRoot(); break;
        default: break;
      }
    } break;

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
            view = View::CAL_ADS_RUN;      // delega a la rutina interactiva
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
        case Btn::OK:  lcd.clear(); renderADSRead(); break;  // refrescar
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
          } else {                         // Calibrar pH (7.00 y 4.00)
            lcd.clear();
            view = View::CAL_PH_RUN;      // delega a la rutina interactiva
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
        case Btn::OK:  lcd.clear(); renderPHRead(); break;  // refrescar lectura
        case Btn::ESC: view = View::CAL_PH_MENU; lcd.clear(); renderPHMenu(); break;
        default: break;
      }
    } break;

    // Rutina de calibración pH (usa runPHCalibration_7_4)
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
