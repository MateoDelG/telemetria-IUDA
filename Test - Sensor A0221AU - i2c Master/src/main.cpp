#include <Arduino.h>
#include <Wire.h>
#include "LevelSensorMaster.h"
using namespace LevelSensorMaster;

void showMenu() {
  Serial.println();
  Serial.println(F("=== MENU MASTER I2C - SENSOR DE NIVEL ==="));
  Serial.println(F("1) Leer todo"));
  Serial.println(F("2) Escribir nivel MIN (cm)"));
  Serial.println(F("3) Escribir nivel MAX (cm)"));
  Serial.println(F("-----------------------------------------"));
  Serial.print(F("Seleccione opción (1-3): "));
}

String readLine() {
  while (!Serial.available()) delay(5);
  String s = Serial.readStringUntil('\n');
  s.trim();
  return s;
}

void handleOption(int opt) {
  switch (opt) {
    case 1: {
      float df, dr, mn, mx;
      uint8_t st;
      bool ok = readAll(df, dr, mn, mx, st);

      Serial.println(F("\n------ LECTURA COMPLETA ------"));
      Serial.printf("Filtrado : %.2f cm\n", df);
      Serial.printf("Crudo    : %.2f cm\n", dr);
      Serial.printf("Min      : %.2f cm\n", mn);
      Serial.printf("Max      : %.2f cm\n", mx);
      printStatus(st);
      Serial.println(F("------------------------------"));
      if (!ok) Serial.println(F("⚠ Hubo errores en la lectura."));
      break;
    }

    case 2: {
      Serial.print(F("\nNuevo nivel MIN (cm): "));
      float v = readLine().toFloat();
      if (writeMinLevel(v))
        Serial.println(F("OK: MIN actualizado."));
      else
        Serial.println(F("Error al escribir MIN."));
      break;
    }

    case 3: {
      Serial.print(F("\nNuevo nivel MAX (cm): "));
      float v = readLine().toFloat();
      if (writeMaxLevel(v))
        Serial.println(F("OK: MAX actualizado."));
      else
        Serial.println(F("Error al escribir MAX."));
      break;
    }

    default:
      Serial.println(F("\nOpción no válida."));
      break;
  }
}

void setup() {
  Serial.begin(115200);
  Wire.begin();                      // Master I2C
  LevelSensorMaster::begin(Wire);    // usa dirección por defecto 0x30

  Serial.println(F("\n=== MASTER I2C - SENSOR NIVEL ==="));
  showMenu();
}

void loop() {
  if (Serial.available()) {
    String cmd = readLine();
    if (cmd.length() == 0) { showMenu(); return; }
    int opt = cmd.toInt();
    handleOption(opt);
    showMenu();
  }
}
