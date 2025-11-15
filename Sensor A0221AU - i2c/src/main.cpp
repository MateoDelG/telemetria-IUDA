#include <Arduino.h>
#include "UltrasonicA02YYUW.h"
#include "WiFiConfigManager.h"
#include "ConfigPortal.h"
#include "Globals.h"
#include "Storage.h"


// ----------------------------
// Pines
// ----------------------------
#define PIN_LEVEL_MIN   6   // salida nivel bajo
#define PIN_LEVEL_MAX   7   // salida nivel alto

// Sensor UART
UltrasonicA02YYUW sensor(Serial1, 3, 4);

// Manejo WiFi / Portal
WiFiConfigManager wifiCfg;
ConfigPortal portal;

// Estado de configuración actualmente aplicada
static float g_kalMeaApplied = 3.0f;
static float g_kalEstApplied = 8.0f;
static float g_kalQApplied   = 0.08f;
static int   g_samplePeriodApplied = 200;   // ms

static float g_minLevelApplied = 0.0f;
static float g_maxLevelApplied = 100.0f;
static uint8_t g_i2cAddressApplied = 0x30;

// Tiempo para muestreo
static unsigned long g_lastSampleMs = 0;

// Prototipos
void setupDistance();
void readDistance();
void updateLevelOutputs();
void monitorConfigChanges();

void setup() {
  Serial.begin(115200);
  delay(300);

  // Salidas
  pinMode(PIN_LEVEL_MIN, OUTPUT);
  pinMode(PIN_LEVEL_MAX, OUTPUT);
  digitalWrite(PIN_LEVEL_MIN, LOW);
  digitalWrite(PIN_LEVEL_MAX, LOW);

  // Inicializar sensor
  setupDistance();

  // Tiempo de configuración: 3 minutos = 180000 ms
  wifiCfg.begin("SensorNivel", "12345678", 180000);

  // Levantar el portal (AP + DNS + Web)
  portal.begin();

  Serial.println("\n[MAIN] Sistema iniciado.");
}

void loop() {
  portal.loop();      // WebServer + DNS
  wifiCfg.update();

  // Ver si cambió algo en la configuración (Globals)
  monitorConfigChanges();

  // Muestreo no bloqueante según samplePeriod
  unsigned long now = millis();
  int sp = g_samplePeriodApplied;
  if (sp <= 0) sp = 200;             // fallback por seguridad

  if (now - g_lastSampleMs >= (unsigned long)sp) {
    g_lastSampleMs = now;
    readDistance();                  // actualizar Globals con lecturas
    updateLevelOutputs();            // actualizar salidas digitales
  }
}

// ------------------------------------------
// Configuración inicial del sensor y Kalman
// ------------------------------------------
void setupDistance() {
  sensor.begin(9600);

  // Leer valores iniciales desde Globals (por si ya vienen de EEPROM)
  g_kalMeaApplied       = Globals::getKalMea();
  g_kalEstApplied       = Globals::getKalEst();
  g_kalQApplied         = Globals::getKalQ();
  g_samplePeriodApplied = Globals::getSamplePeriod();
  g_minLevelApplied     = Globals::getMinLevel();
  g_maxLevelApplied     = Globals::getMaxLevel();

  if (g_samplePeriodApplied <= 0) g_samplePeriodApplied = 200;

  // Aplicar a sensor
  sensor.setMeasurementError(g_kalMeaApplied);
  sensor.setEstimateError(g_kalEstApplied);
  sensor.setProcessNoise(g_kalQApplied);

  Serial.println("[MAIN] Sensor A02 listo.");
  Serial.printf("[MAIN] Kalman: mea=%.3f est=%.3f q=%.4f, sample=%d ms\n",
                g_kalMeaApplied, g_kalEstApplied, g_kalQApplied, g_samplePeriodApplied);
}

// ------------------------------------------
// Lectura del sensor + envío a Globals
// ------------------------------------------
void readDistance() {
  sensor.update();

  if (sensor.isValid()) {
    float raw  = sensor.getDistanceRaw();
    float filt = sensor.getDistance();

    Globals::setDistanceRaw(raw);
    Globals::setDistanceFiltered(filt);

    // Debug si quieres
    // Serial.printf(">Crudo: %.2f  | Filtrado: %.2f\n", raw, filt);
  }
}

// ------------------------------------------
// Activar salidas según niveles configurados
// ------------------------------------------
void updateLevelOutputs() {
  float d = Globals::getDistanceFiltered();
  if (isnan(d)) return;

  float minLevel = Globals::getMinLevel();
  float maxLevel = Globals::getMaxLevel();

  // Nivel mínimo superado (distancia pequeña → tanque lleno)
  digitalWrite(PIN_LEVEL_MIN, d <= minLevel ? HIGH : LOW);

  // Nivel máximo superado (distancia grande → tanque vacío)
  digitalWrite(PIN_LEVEL_MAX, d >= maxLevel ? HIGH : LOW);
}

// ------------------------------------------
// Monitorea Globals y aplica cambios al sensor
// ------------------------------------------
void monitorConfigChanges() {
  bool changed = false;

  // ------------------ Kalman: measurement error ------------------
  float km = Globals::getKalMea();
  if (km != g_kalMeaApplied) {
    g_kalMeaApplied = km;
    sensor.setMeasurementError(km);
    Serial.printf("[CFG] Nuevo kalMea = %.3f\n", km);
    changed = true;
  }

  // ------------------ Kalman: estimate error ------------------
  float ke = Globals::getKalEst();
  if (ke != g_kalEstApplied) {
    g_kalEstApplied = ke;
    sensor.setEstimateError(ke);
    Serial.printf("[CFG] Nuevo kalEst = %.3f\n", ke);
    changed = true;
  }

  // ------------------ Kalman: process noise ------------------
  float kq = Globals::getKalQ();
  if (kq != g_kalQApplied) {
    g_kalQApplied = kq;
    sensor.setProcessNoise(kq);
    Serial.printf("[CFG] Nuevo kalQ = %.4f\n", kq);
    changed = true;
  }

  // ------------------ Periodo de muestreo ------------------
  int sp = Globals::getSamplePeriod();
  if (sp != g_samplePeriodApplied && sp > 0) {
    g_samplePeriodApplied = sp;
    Serial.printf("[CFG] Nuevo samplePeriod = %d ms\n", sp);
    changed = true;
  }

  // ------------------ Niveles ------------------
  float minL = Globals::getMinLevel();
  if (minL != g_minLevelApplied) {
    g_minLevelApplied = minL;
    Serial.printf("[CFG] Nuevo nivel MIN = %.2f cm\n", minL);
    changed = true;
  }

  float maxL = Globals::getMaxLevel();
  if (maxL != g_maxLevelApplied) {
    g_maxLevelApplied = maxL;
    Serial.printf("[CFG] Nuevo nivel MAX = %.2f cm\n", maxL);
    changed = true;
  }

  // ------------------ Dirección I2C ------------------
  uint8_t ia = Globals::getI2CAddress();
  if (ia != g_i2cAddressApplied) {
    g_i2cAddressApplied = ia;
    Serial.printf("[CFG] Nueva dirección I2C = %u (0x%02X)\n", ia, ia);
    changed = true;
  }

  // ------------------ Guardar en NVS si algo cambió ------------------
  if (changed) {
    ConfigData cfg;
    cfg.minLevel     = Globals::getMinLevel();
    cfg.maxLevel     = Globals::getMaxLevel();
    cfg.samplePeriod = Globals::getSamplePeriod();
    cfg.kalMea       = Globals::getKalMea();
    cfg.kalEst       = Globals::getKalEst();
    cfg.kalQ         = Globals::getKalQ();
    cfg.i2cAddress   = Globals::getI2CAddress();

    Storage::saveConfig(cfg);
    Serial.println("[CFG] Config guardada en NVS");
  }
}

