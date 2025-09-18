#include <Arduino.h>

constexpr int RX_PIN = 16;  // ajusta a los mismos pines que conectaste
constexpr int TX_PIN = 17;  // TX del cliente -> RX del equipo, y viceversa

void sendJson(const char* msg) {
  Serial2.println(msg);   // envía JSON + '\n' (importante para NDJSON)
  Serial.print("[TX] ");
  Serial.println(msg);
}

void setup() {
  Serial.begin(115200);   // debug por USB
  Serial2.begin(115200, SERIAL_8N1, RX_PIN, TX_PIN);

  delay(5000);
  Serial.println("Cliente UART listo.");
  // 1) Preguntar estado
  sendJson("{\"op\":\"get_status\"}");
  delay(1000);
  // 2) Solicitar una medición
  sendJson("{\"op\":\"auto_measure\"}");
  delay(1000);
  // 3) Intentar pedir otra medición (debe dar BUSY)
  sendJson("{\"op\":\"auto_measure\"}");
  delay(1000);
  // 4) Preguntar última medición
  sendJson("{\"op\":\"get_last\"}");
}

void loop() {
  Serial2.println("hola");
  Serial.println("hola");
  // Imprime cualquier cosa que responda el equipo
  while (Serial2.available()) {
    String line = Serial2.readStringUntil('\n');
    if (line.length() > 0) {
      Serial.print("[RX] ");
      Serial.println(line);
    }
  }
  delay(1000);
}
