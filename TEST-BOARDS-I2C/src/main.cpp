/*
  ESP32 + PCF8574 (Renzo Mischianti v2.3.7)
  - Leer todos los pines como ENTRADAS directas (1 = alto, 0 = bajo)
  - Probar salidas (enciende/apaga cada pin)
  - Sin interrupciones ni pull-ups
*/

#include <Arduino.h>
#include <Wire.h>
#include "PCF8574.h"

// ---------- Configuración I2C ----------
#define SDA_PIN   21
#define SCL_PIN   22
#define I2C_FREQ  100000
#define PCF_ADDR  0x26  // Ajusta según tu módulo (0x20–0x27 / 0x38–0x3F)

// Instancia
PCF8574 pcf(PCF_ADDR, SDA_PIN, SCL_PIN);

// ---------- Escáner I2C ----------
void i2cScan(TwoWire &bus) {
  Serial.println("\n--- Escáner I2C ---");
  uint8_t found = 0;
  for (uint8_t addr = 0x03; addr <= 0x77; addr++) {
    bus.beginTransmission(addr);
    uint8_t err = bus.endTransmission();
    if (err == 0) {
      Serial.printf("  ✔ 0x%02X", addr);
      if ((addr >= 0x20 && addr <= 0x27) || (addr >= 0x38 && addr <= 0x3F))
        Serial.print("  (posible PCF8574/8574A)");
      Serial.println();
      found++;
    }
    delay(2);
  }
  if (!found) Serial.println("  (sin dispositivos)");
  Serial.println("--------------------\n");
}

// ---------- Empaquetar p0..p7 ----------
uint8_t pack(const PCF8574::DigitalInput &in) {
  uint8_t b = 0;
  b |= (in.p0 ? 1 : 0) << 0;
  b |= (in.p1 ? 1 : 0) << 1;
  b |= (in.p2 ? 1 : 0) << 2;
  b |= (in.p3 ? 1 : 0) << 3;
  b |= (in.p4 ? 1 : 0) << 4;
  b |= (in.p5 ? 1 : 0) << 5;
  b |= (in.p6 ? 1 : 0) << 6;
  b |= (in.p7 ? 1 : 0) << 7;
  return b;
}

// ---------- Prueba de entradas ----------
void testInputs() {
  if (Serial.available()) {
    char c = Serial.read();
    if (c == 's' || c == 'S') i2cScan(Wire);
  }

  static uint8_t last = 0xFF;
  PCF8574::DigitalInput in = pcf.digitalReadAll();
  uint8_t now = pack(in);

  if (now != last) {
    Serial.print("Cambio P7..P0: ");
    for (int i = 7; i >= 0; i--) Serial.print((now >> i) & 0x01);
    Serial.println();

    // Detalle por pin (1=alto, 0=bajo)
    const uint8_t vals[8] = {in.p0,in.p1,in.p2,in.p3,in.p4,in.p5,in.p6,in.p7};
    for (uint8_t i = 0; i < 8; i++) {
      Serial.printf("  P%u -> %s\n", i, vals[i] ? "ALTO (1)" : "BAJO (0)");
    }
    Serial.println();

    last = now;
  }

  delay(20);
}

// ---------- Prueba de salidas ----------
void testOutputs(uint16_t delay_ms = 500) {
  Serial.println("\nIniciando prueba de SALIDAS...");

  // Configurar todos como salidas
  for (uint8_t pin = 0; pin < 8; pin++) {
    pcf.pinMode(pin, OUTPUT);
    pcf.digitalWrite(pin, LOW);
  }

  delay(500);

  // Encender cada pin uno por uno
  for (uint8_t pin = 0; pin < 8; pin++) {
    Serial.printf("Activando P%u -> HIGH\n", pin);
    pcf.digitalWrite(pin, HIGH);
    delay(delay_ms);
    pcf.digitalWrite(pin, LOW);
  }

  // Encender todos simultáneamente
  Serial.println("Encendiendo TODOS los pines...");
  for (uint8_t pin = 0; pin < 8; pin++) {
    pcf.digitalWrite(pin, HIGH);
  }
  delay(delay_ms);

  // Apagar todos
  Serial.println("Apagando TODOS los pines...");
  for (uint8_t pin = 0; pin < 8; pin++) {
    pcf.digitalWrite(pin, LOW);
  }

  Serial.println("✅ Prueba de salidas finalizada.\n");
}

void setup() {
  Serial.begin(115200);
  while (!Serial) {}
  Serial.println("\nESP32 + PCF8574 (entradas/salidas)");

  Wire.begin(SDA_PIN, SCL_PIN);
  Wire.setClock(I2C_FREQ);
  i2cScan(Wire);

    // ✅ CLAVE: configura pines ANTES de begin()
  // Por defecto todos como ENTRADA (sin pull-up, como pediste)
  for (uint8_t pin = 0; pin < 8; pin++) {
    pcf.pinMode(pin, INPUT);
  }

  Serial.print("Init PCF8574... ");
  if (pcf.begin()) Serial.println("OK");
  else { Serial.println("KO");
     while (true) delay(100);
     }

  Serial.println("\nPresiona:");
  Serial.println("  help           → ver comandos");
  Serial.println("  read | i       → leer pines P7..P0");
  Serial.println("  test | o       → prueba secuencial de salidas");
  Serial.println("  scan | s       → escanear I2C");
  Serial.println("  on N / off N   → activar/desactivar pin N (0..7)");
  Serial.println("  toggle N       → alternar pin N");
  Serial.println("  in N           → poner pin N como ENTRADA");
  Serial.println("  all on | off   → todos HIGH/LOW");
  Serial.println("  Atajos: aN, dN, tN, rN\n");
}

void loop() {
  // Intérprete sencillo de comandos por Serial
  static String cmd;
  while (Serial.available()) {
    char ch = Serial.read();
    if (ch == '\r') continue;  // ignora CR
    if (ch == '\n') {
      cmd.trim();
      if (cmd.length()) {
        String low = cmd; low.toLowerCase();

        // Comandos de una palabra
        if (low == "help" || low == "ayuda" || low == "h") {
          Serial.println("\nComandos:");
          Serial.println("  on N      -> Pn alto (0..7)");
          Serial.println("  off N     -> Pn bajo (0..7)");
          Serial.println("  toggle N  -> alterna Pn");
          Serial.println("  in N      -> Pn como ENTRADA");
          Serial.println("  all on    -> todos alto");
          Serial.println("  all off   -> todos bajo");
          Serial.println("  scan      -> escanear I2C");
          Serial.println("  test      -> prueba secuencial de salidas");
          Serial.println("  read      -> leer pines P7..P0\n");
        } else if (low == "scan" || low == "s") {
          i2cScan(Wire);
        } else if (low == "test" || low == "o") {
          testOutputs(500);
        } else if (low == "read" || low == "i") {
          // Lectura rápida del estado de entradas
          PCF8574::DigitalInput in = pcf.digitalReadAll();
          uint8_t now = pack(in);
          Serial.print("P7..P0: ");
          for (int i = 7; i >= 0; i--) Serial.print((now >> i) & 0x01);
          Serial.println();
        } else {
          // Comandos con parámetros
          // Soporta: on N, off N, toggle N, in N, all on, all off
          int spaceIdx = low.indexOf(' ');
          String a = (spaceIdx == -1) ? low : low.substring(0, spaceIdx);
          String b = (spaceIdx == -1) ? "" : low.substring(spaceIdx + 1);

          auto parsePin = [](const String &s) -> int {
            String t = s; t.trim();
            if (!t.length()) return -1;
            for (size_t i = 0; i < t.length(); ++i) if (!isDigit(t[i])) return -1;
            long v = t.toInt();
            if (v < 0 || v > 7) return -1;
            return (int)v;
          };

          bool handled = false;
          if ((a == "all" || a == "todos") && (b == "on" || b == "off")) {
            for (uint8_t pin = 0; pin < 8; pin++) {
              pcf.pinMode(pin, OUTPUT);
              pcf.digitalWrite(pin, (b == "on") ? HIGH : LOW);
            }
            Serial.printf("OK: todos -> %s\n", (b == "on") ? "HIGH" : "LOW");
            handled = true;
          } else if (a == "on" || a == "off" || a == "toggle" || a == "in") {
            int pin = parsePin(b);
            if (pin < 0) {
              Serial.println("Error: especifica pin 0..7 (ej: 'on 3')");
              handled = true; // ya respondió con error
            } else {
              if (a == "in") {
                pcf.pinMode((uint8_t)pin, INPUT);
                Serial.printf("OK: P%u -> INPUT\n", (uint8_t)pin);
              } else if (a == "toggle") {
                // Leer estado actual via digitalRead (como salida)
                // No hay lectura directa del latch, se asume modo OUTPUT y se invierte
                pcf.pinMode((uint8_t)pin, OUTPUT);
                // Intento de lectura del bus para estimar estado
                PCF8574::DigitalInput in = pcf.digitalReadAll();
                uint8_t now = ((pack(in) >> pin) & 0x01);
                uint8_t next = now ? LOW : HIGH;
                pcf.digitalWrite((uint8_t)pin, next);
                Serial.printf("OK: P%u -> %s\n", (uint8_t)pin, next == HIGH ? "HIGH" : "LOW");
              } else {
                pcf.pinMode((uint8_t)pin, OUTPUT);
                pcf.digitalWrite((uint8_t)pin, (a == "on") ? HIGH : LOW);
                Serial.printf("OK: P%u -> %s\n", (uint8_t)pin, (a == "on") ? "HIGH" : "LOW");
              }
              handled = true;
            }
          }

          if (!handled) {
            // Atajos: aN (activa), dN (desactiva), tN (toggle), rN (input)
            if (low.length() >= 2) {
              char op = low[0];
              String rest = low.substring(1); rest.trim();
              int pin = parsePin(rest);
              if (pin >= 0) {
                if (op == 'a') { pcf.pinMode((uint8_t)pin, OUTPUT); pcf.digitalWrite((uint8_t)pin, HIGH); Serial.printf("OK: P%u -> HIGH\n", (uint8_t)pin); }
                else if (op == 'd') { pcf.pinMode((uint8_t)pin, OUTPUT); pcf.digitalWrite((uint8_t)pin, LOW);  Serial.printf("OK: P%u -> LOW\n",  (uint8_t)pin); }
                else if (op == 't') {
                  pcf.pinMode((uint8_t)pin, OUTPUT);
                  PCF8574::DigitalInput in = pcf.digitalReadAll();
                  uint8_t now = ((pack(in) >> pin) & 0x01);
                  uint8_t next = now ? LOW : HIGH;
                  pcf.digitalWrite((uint8_t)pin, next);
                  Serial.printf("OK: P%u -> %s\n", (uint8_t)pin, next == HIGH ? "HIGH" : "LOW");
                }
                else if (op == 'r') { pcf.pinMode((uint8_t)pin, INPUT); Serial.printf("OK: P%u -> INPUT\n", (uint8_t)pin); }
                else { Serial.println("Comando desconocido. Escribe 'help'."); }
                handled = true;
              }
            }

            if (!handled) {
              Serial.println("Comando no reconocido. Escribe 'help'.");
            }
          }
        }
      }
      cmd = ""; // limpia buffer
    } else {
      cmd += ch;
    }
  }
}
