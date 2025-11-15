#include <Arduino.h>
#include <Wire.h>
#include "PCF8574.h"
#include <Adafruit_ADS1X15.h>
#include "LiquidCrystal_I2C.h"
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Adafruit_ADS1X15.h>

static const int SW_DOWN_PIN = 36;
static const int SW_UP_PIN   = 39;
static const int SW_SEL_PIN  = 35;
static const int SW_ESC_PIN  = 34;
static const int BUZZER = 5;

static const int LVL_H2O =33;
static const int LVL_O2  = 26;
static const int LVL_PH  = 27;
static const int LVL_KCL = 32;

// Ajusta según tu cableado: true = active-low (pull-up + botón a GND)
static const bool ACTIVE_LOW = true;

// Buzzer config
// Buzzer: solo ON/OFF

// Pines I2C (ESP32 por defecto: SDA=21, SCL=22)
static const int I2C_SDA_PIN = 21;
static const int I2C_SCL_PIN = 22;

// PCF8574 (8-bit I/O expander) - ajustar si usas otra dirección
static const uint8_t PCF_ADDR = 0x20; // A2..A0 = 000 -> 0x20

// LVL (entradas digitales) - AJUSTA ESTOS PINES SEGÚN TU PCB
// Usa -1 para deshabilitar una posición. Ejemplo con dos entradas en 32 y 33:
// static const int LVL_PINS[] = {32, 33, -1, -1, -1, -1, -1, -1};
static const int LVL_PINS[] = {LVL_H2O, LVL_O2, LVL_PH, LVL_KCL, -1, -1, -1, -1};
static const bool LVL_ACTIVE_LOW = false; // true si LVL está activo en 0 (pull-up + sensor a GND)
static const int I2C_FREQ = 100000;

// Instancia
PCF8574 pcf(PCF_ADDR, I2C_SDA_PIN, I2C_SCL_PIN);

// LCD 16x2 (I2C)
static const uint8_t LCD_ADDR = 0x27; // ajusta si tu modulo usa otra
static const uint8_t LCD_COLS = 16;
static const uint8_t LCD_ROWS = 2;
static LiquidCrystal_I2C lcd(LCD_ADDR, LCD_COLS, LCD_ROWS);

// DS18B20 (1-Wire)
static const uint8_t DS18B20_PIN = 19;  // GPIO donde está el bus 1-Wire
static const uint8_t DS18B20_RES = 12;  // resolución en bits
static OneWire oneWire(DS18B20_PIN);
static DallasTemperature dallas(&oneWire);

struct SwitchDef {
  const char* name;
  uint8_t pin;
  bool lastState;
};

SwitchDef switches[] = {
  {"SW_DOWN", SW_DOWN_PIN, false},
  {"SW_UP",   SW_UP_PIN,   false},
  {"SW_SEL",  SW_SEL_PIN,  false},
  {"SW_ESC",  SW_ESC_PIN,  false},
};

void printMenu() {
  Serial.println();
  Serial.println("=== MENU DE PRUEBAS PCB ===");
  Serial.println("1) Probar switches (SW_DOWN, SW_UP, SW_SEL, SW_ESC)");
  Serial.println("2) Probar buzzer");
  Serial.println("3) Scan I2C");
  Serial.println("4) Probar PCF (salidas)");
  Serial.println("5) Probar LVL (entradas)");
  Serial.println("6) Probar LCD 16x2 (I2C)");
  Serial.println("7) Probar DS18B20 (Temp)");
  Serial.println("8) Probar ADS1115 (A0-A3)");
  Serial.println("m) Mostrar menu");
  Serial.println("---------------------------");
  Serial.println("En la prueba: 'q' para salir");
  Serial.println();
}

bool readLogical(uint8_t pin) {
  int raw = digitalRead(pin);
  return ACTIVE_LOW ? (raw == LOW) : (raw == HIGH);
}

void testSwitches() {
  Serial.println();
  Serial.println("Prueba de switches iniciada.");
  Serial.println("Pulsa los botones. Presiona 'q' para volver al menu.");

  // Inicializa estados
  for (auto& s : switches) {
    s.lastState = readLogical(s.pin);
  }

  while (true) {
    if (Serial.available()) {
      int c = Serial.read();
      if (c == 'q' || c == 'Q') {
        Serial.println("Saliendo de la prueba de switches...");
        break;
      } else if (c == 'm' || c == 'M') {
        printMenu();
      }
    }

    for (auto& s : switches) {
      bool state = readLogical(s.pin);
      if (state != s.lastState) {
        delay(15); // debounce simple
        bool confirm = readLogical(s.pin);
        if (confirm == state) {
          s.lastState = state;
          Serial.print("[");
          Serial.print(s.name);
          Serial.print("] ");
          Serial.println(state ? "PRESSED" : "RELEASED");
        }
      }
    }

    delay(10);
  }
}

// -------------------- Buzzer: ON/OFF simple --------------------
static inline void buzzerOn()  { digitalWrite(BUZZER, HIGH); }
static inline void buzzerOff() { digitalWrite(BUZZER, LOW); }

void testBuzzer() {
  Serial.println();
  Serial.print("Prueba de buzzer en GPIO ");
  Serial.println(BUZZER);
  pinMode(BUZZER, OUTPUT);
  buzzerOff();

  Serial.println("Comandos: o=ON, f=OFF, q=salir");

  bool running = true;
  while (running) {
    if (Serial.available()) {
      int c = Serial.read();
      switch (c) {
        case 'o':
        case 'O':
          buzzerOn();
          Serial.println("Buzzer ON.");
          break;
        case 'f':
        case 'F':
          buzzerOff();
          Serial.println("Buzzer OFF.");
          break;
        case 'q':
        case 'Q':
          running = false;
          break;
        default:
          break;
      }
    }
    delay(5);
  }

  buzzerOff();
  Serial.println("Saliendo de la prueba de buzzer...");
}

// -------------------- PCF8574: prueba de salidas --------------------
static bool pcfWrite8(uint8_t value) {
  Wire.beginTransmission(PCF_ADDR);
  Wire.write(value);
  return Wire.endTransmission(true) == 0;
}

static uint8_t pcfRead8() {
  uint8_t value = 0xFF;
  Wire.requestFrom((int)PCF_ADDR, 1);
  if (Wire.available()) value = Wire.read();
  return value;
}

static void pcfPrintState(uint8_t value) {
  Serial.print("Estado 0b");
  for (int i = 7; i >= 0; --i) Serial.print((value >> i) & 1);
  Serial.print(" (0x");
  if (value < 16) Serial.print('0');
  Serial.print(value, HEX);
  Serial.println(")");
}

static void testPCFOutputs() {
  Serial.println();
  Serial.print("PCF8574 @ 0x");
  Serial.print(PCF_ADDR, HEX);
  Serial.print(" en SDA=");
  Serial.print(I2C_SDA_PIN);
  Serial.print(" SCL=");
  Serial.println(I2C_SCL_PIN);

  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
  Wire.setClock(100000);

  // Valor inicial alto (1) libera salidas en PCF8574 (quasi-bidireccional)
  uint8_t state = 0xFF;
  if (!pcfWrite8(state)) {
    Serial.println("ERROR: No se pudo comunicar con el PCF.");
  }
  Serial.println("Comandos: 0-7=toggle bit, a=todo 0, A=todo 1, p=leer, w=escribir HEX (00-FF) + Enter, q=salir");
  pcfPrintState(pcfRead8());

  bool running = true;
  while (running) {
    if (Serial.available()) {
      int c = Serial.read();
      if (c >= '0' && c <= '7') {
        int bit = c - '0';
        state ^= (1U << bit);
        if (pcfWrite8(state)) {
          Serial.print("Toggled bit "); Serial.println(bit);
          pcfPrintState(pcfRead8());
        } else {
          Serial.println("Fallo al escribir en PCF");
        }
      } else {
        switch (c) {
          case 'a':
            state = 0x00;
            if (pcfWrite8(state)) { Serial.println("Todos a 0"); pcfPrintState(pcfRead8()); }
            break;
          case 'A':
            state = 0xFF;
            if (pcfWrite8(state)) { Serial.println("Todos a 1"); pcfPrintState(pcfRead8()); }
            break;
          case 'p':
          case 'P':
            pcfPrintState(pcfRead8());
            break;
          case 'w':
          case 'W': {
            Serial.println("Ingrese valor HEX (00-FF) y Enter:");
            String line = Serial.readStringUntil('\n');
            line.trim();
            if (line.length() > 0) {
              char *endptr = nullptr;
              long val = strtol(line.c_str(), &endptr, 16);
              if (endptr && *endptr == '\0' && val >= 0 && val <= 255) {
                state = (uint8_t)val;
                if (pcfWrite8(state)) { Serial.println("Escrito."); pcfPrintState(pcfRead8()); }
              } else {
                Serial.println("HEX inválido.");
              }
            }
            break;
          }
          case 'q':
          case 'Q':
            running = false;
            break;
          default:
            break;
        }
      }
    }
    delay(5);
  }

  Serial.println("Saliendo de la prueba de PCF...");
}

// -------------------- LVL (entradas digitales) --------------------
static bool lvlReadLogical(int pin) {
  int raw = digitalRead(pin);
  return LVL_ACTIVE_LOW ? (raw == LOW) : (raw == HIGH);
}

static void testLVLInputs() {
  Serial.println();
  Serial.println("Prueba LVL (entradas digitales)");

  // Construye la lista de pines habilitados
  const int maxN = (int)(sizeof(LVL_PINS) / sizeof(LVL_PINS[0]));
  int pins[8];
  bool last[8];
  int n = 0;
  for (int i = 0; i < maxN && n < 8; ++i) {
    if (LVL_PINS[i] >= 0) {
      pins[n] = LVL_PINS[i];
      ++n;
    }
  }

  if (n == 0) {
    Serial.println("No hay pines LVL configurados. Edita LVL_PINS en el codigo.");
    Serial.println("Saliendo de la prueba LVL...");
    return;
  }

  Serial.print("Entradas configuradas (" ); Serial.print(n); Serial.println("):");
  for (int i = 0; i < n; ++i) {
    Serial.print("  LVL["); Serial.print(i); Serial.print("] -> GPIO "); Serial.println(pins[i]);
    pinMode(pins[i], INPUT); // cambia a INPUT_PULLUP si corresponde a tu hardware
    last[i] = lvlReadLogical(pins[i]);
  }

  Serial.println("Pulsa 'q' para salir, 'm' para reimprimir estados.");

  // Imprime estado inicial
  for (int i = 0; i < n; ++i) {
    Serial.print("[LVL"); Serial.print(i); Serial.print("] ");
    Serial.println(last[i] ? "ACTIVE" : "INACTIVE");
  }

  bool running = true;
  while (running) {
    if (Serial.available()) {
      int c = Serial.read();
      if (c == 'q' || c == 'Q') {
        running = false;
      } else if (c == 'm' || c == 'M') {
        for (int i = 0; i < n; ++i) {
          Serial.print("[LVL"); Serial.print(i); Serial.print("] ");
          Serial.println(last[i] ? "ACTIVE" : "INACTIVE");
        }
      }
    }

    for (int i = 0; i < n; ++i) {
      bool s = lvlReadLogical(pins[i]);
      if (s != last[i]) {
        delay(15); // debounce simple
        bool confirm = lvlReadLogical(pins[i]);
        if (confirm == s) {
          last[i] = s;
          Serial.print("[LVL"); Serial.print(i); Serial.print("] ");
          Serial.println(s ? "ACTIVE" : "INACTIVE");
        }
      }
    }

    delay(10);
  }

  Serial.println("Saliendo de la prueba LVL...");
}
// -------------------- I2C Scan --------------------
static void i2cDoScanOnce() {
  int found = 0;
  Serial.println("Escaneando direcciones 0x03 a 0x77...");
  for (uint8_t address = 0x03; address <= 0x77; ++address) {
    Wire.beginTransmission(address);
    uint8_t error = Wire.endTransmission(true);
    if (error == 0) {
      Serial.print(" - Encontrado dispositivo en 0x");
      if (address < 16) Serial.print('0');
      Serial.println(address, HEX);
      found++;
    } else if (error == 4) {
      Serial.print(" - Error desconocido en 0x");
      if (address < 16) Serial.print('0');
      Serial.println(address, HEX);
    }
    delay(2);
  }
  if (found == 0) {
    Serial.println("No se detectaron dispositivos I2C.");
  } else {
    Serial.print("Total encontrados: ");
    Serial.println(found);
  }
}

static void testI2CScan() {
  Serial.println();
  Serial.print("I2C Scan en SDA=");
  Serial.print(I2C_SDA_PIN);
  Serial.print(" SCL=");
  Serial.println(I2C_SCL_PIN);

  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
  Wire.setClock(100000);

  i2cDoScanOnce();
  Serial.println("Comandos: r=re-escanear, q=salir");

  bool running = true;
  while (running) {
    if (Serial.available()) {
      int c = Serial.read();
      switch (c) {
        case 'r':
        case 'R':
          i2cDoScanOnce();
          break;
        case 'q':
        case 'Q':
          running = false;
          break;
        default:
          break;
      }
    }
    delay(5);
  }

  Serial.println("Saliendo del escaneo I2C...");
}


// -------------------- LCD 16x2 (I2C) --------------------
static void testLCD16x2() {
  Serial.println();
  Serial.print("LCD 16x2 @ 0x");
  Serial.print(LCD_ADDR, HEX);
  Serial.print(" en SDA=");
  Serial.print(I2C_SDA_PIN);
  Serial.print(" SCL=");
  Serial.println(I2C_SCL_PIN);

  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
  Wire.setClock(I2C_FREQ);

  lcd.init();
  lcd.backlight();
  lcd.clear();

  const char* L1 = "PRUEBA LCD L1"; // Ajusta si deseas otro texto
  const char* L2 = "PRUEBA LCD L2";

  // Mostrar textos al entrar
  lcd.setCursor(0, 0);
  lcd.print(L1);
  lcd.setCursor(0, 1);
  lcd.print(L2);

  Serial.println("Comandos: s=mostrar, c=limpiar, q=salir");

  bool running = true;
  while (running) {
    if (Serial.available()) {
      int c = Serial.read();
      switch (c) {
        case 's':
        case 'S':
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print(L1);
          lcd.setCursor(0, 1);
          lcd.print(L2);
          break;
        case 'c':
        case 'C':
          lcd.clear();
          break;
        case 'q':
        case 'Q':
          running = false;
          break;
        default:
          break;
      }
    }
    delay(5);
  }

  Serial.println("Saliendo de la prueba LCD...");
}

// -------------------- DS18B20 (temperatura) --------------------
static void ds18b20PrintOnce() {
  dallas.requestTemperatures();
  float c = dallas.getTempCByIndex(0);
  if (c == DEVICE_DISCONNECTED_C) {
    Serial.println("Sensor no detectado (DEVICE_DISCONNECTED)");
    return;
  }
  Serial.print("T = ");
  Serial.print(c, 2);
  Serial.println("  C");
}

static void testDS18B20() {
  Serial.println();
  Serial.print("DS18B20 en GPIO ");
  Serial.println(DS18B20_PIN);

  dallas.begin();
  dallas.setResolution(DS18B20_RES);

  int count = dallas.getDeviceCount();
  Serial.print("Dispositivos detectados: ");
  Serial.println(count);
  if (count == 0) {
    Serial.println("Conecta el sensor y presiona 'r' para reintentar, 'q' para salir.");
  }

  ds18b20PrintOnce();
  Serial.println("Comandos: r=leer una vez, a=auto cada 1s, s=stop, q=salir");

  bool running = true;
  bool autoMode = false;
  unsigned long last = 0;
  while (running) {
    if (Serial.available()) {
      int c = Serial.read();
      switch (c) {
        case 'r':
        case 'R':
          ds18b20PrintOnce();
          break;
        case 'a':
        case 'A':
          autoMode = true;
          Serial.println("Auto ON (1s)");
          break;
        case 's':
        case 'S':
          autoMode = false;
          Serial.println("Auto OFF");
          break;
        case 'q':
        case 'Q':
          running = false;
          break;
        default:
          break;
      }
    }

    if (autoMode && (millis() - last >= 1000)) {
      last = millis();
      ds18b20PrintOnce();
    }

    delay(5);
  }

  Serial.println("Saliendo de la prueba DS18B20...");
}

// -------------------- ADS1115 (A0..A3) --------------------
static void ads1115PrintAll(Adafruit_ADS1115 &ads) {
  for (uint8_t ch = 0; ch < 4; ++ch) {
    int16_t raw = ads.readADC_SingleEnded(ch);
    float v = ads.computeVolts(raw);
    Serial.print("A"); Serial.print(ch); Serial.print(": ");
    Serial.print(v, 6); Serial.println(" V");
  }
}

static void testADS1115() {
  Serial.println();
  Serial.println("Prueba ADS1115 (single-ended A0..A3)");
  Serial.println("Addr=0x48, I2C 100k, GAIN=2/3 (+/-6.144V), RATE=128SPS");

  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
  Wire.setClock(I2C_FREQ);

  Adafruit_ADS1115 ads;
  bool connected = ads.begin(0x48, &Wire);
  if (!connected) {
    Serial.println("No se pudo inicializar ADS1115 @0x48");
    Serial.println("Pulsa 'q' para salir o 'r' para reintentar.");
  } else {
    // Config opcional
    ads.setGain(GAIN_TWOTHIRDS);       // +/- 6.144 V
    #ifdef RATE_ADS1115_128SPS
    ads.setDataRate(RATE_ADS1115_128SPS);
    #endif
    ads1115PrintAll(ads);
  }
  Serial.println("Comandos: r=leer todo, a=auto 500ms, s=stop, q=salir");

  bool running = true;
  bool autoMode = false;
  unsigned long last = 0;
  while (running) {
    if (Serial.available()) {
      int c = Serial.read();
      switch (c) {
        case 'r':
        case 'R':
          if (!connected) {
            connected = ads.begin(0x48, &Wire);
            if (!connected) Serial.println("Reintento fallido.");
            if (connected) {
              ads.setGain(GAIN_TWOTHIRDS);
              #ifdef RATE_ADS1115_128SPS
              ads.setDataRate(RATE_ADS1115_128SPS);
              #endif
            }
          }
          if (connected) ads1115PrintAll(ads);
          break;
        case 'a':
        case 'A':
          autoMode = true;
          Serial.println("Auto ON (500ms)");
          break;
        case 's':
        case 'S':
          autoMode = false;
          Serial.println("Auto OFF");
          break;
        case 'q':
        case 'Q':
          running = false;
          break;
        default:
          break;
      }
    }

    if (autoMode && (millis() - last >= 500)) {
      last = millis();
      if (!connected) {
        connected = ads.begin(0x48, &Wire);
        if (connected) {
          ads.setGain(GAIN_TWOTHIRDS);
          #ifdef RATE_ADS1115_128SPS
          ads.setDataRate(RATE_ADS1115_128SPS);
          #endif
        }
      }
      if (connected) ads1115PrintAll(ads);
    }

    delay(5);
  }

  Serial.println("Saliendo de la prueba ADS1115...");
}

void setup() {
  Serial.begin(115200);
  // Pines 34/35/36/39 no tienen pull-ups internos
  pinMode(SW_DOWN_PIN, INPUT);
  pinMode(SW_UP_PIN,   INPUT);
  pinMode(SW_SEL_PIN,  INPUT);
  pinMode(SW_ESC_PIN,  INPUT);
  pinMode(BUZZER, OUTPUT);
  buzzerOff();
  lcd.init();
  lcd.backlight();
  lcd.clear();

  Serial.println();
  Serial.println("Inicializando pruebas PCB (ESP32 DevKit V1)...");
  Serial.println("Nota: GPIO 34-39 requieren resistencias externas de pull-up/down.");
  printMenu();
}

void loop() {
  if (Serial.available()) {
    int c = Serial.read();
    switch (c) {
      case '1':
        testSwitches();
        printMenu();
        break;
      case '2':
        testBuzzer();
        printMenu();
        break;
      case '3':
        testI2CScan();
        printMenu();
        break;
      case '4':
        testPCFOutputs();
        printMenu();
        break;
      case '5':
        testLVLInputs();
        printMenu();
        break;
      case '6':
        testLCD16x2();
        printMenu();
        break;
      case '7':
        testDS18B20();
        printMenu();
        break;
      case '8':
        testADS1115();
        printMenu();
        break;
      case 'm':
      case 'M':
        printMenu();
        break;
      default:
        // Ignorar otros caracteres
        break;
    }
  }
}
