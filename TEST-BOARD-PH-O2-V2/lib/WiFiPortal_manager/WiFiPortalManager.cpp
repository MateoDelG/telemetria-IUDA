#include "WiFiPortalManager.h"
#include <WiFi.h>
#include <ESPmDNS.h>


WiFiPortalManager::WiFiPortalManager(const char* apName, const char* apPassword, int apTriggerPin)
    : _apName(apName), _apPassword(apPassword), _apTriggerPin(apTriggerPin) {}

    void WiFiPortalManager::begin() {
        if (_apTriggerPin != -1) {
            pinMode(_apTriggerPin, INPUT_PULLUP);  // Se asume pulsador con pull-up (LOW activa portal)
        }
    
        // Si el pin está en LOW: solo activar modo AP y servir el portal
        if (_apTriggerPin == -1 || digitalRead(_apTriggerPin) == LOW) {
            // lcd.clear();
            // lcd.centerPrint(0, "Iniciando AP");
            // lcd.centerPrint(1, _apName);
           
            _apModeEnabled = true;

            WiFi.mode(WIFI_AP);  // Solo AP
            WiFi.softAP(_apName, _apPassword);
            Serial.print("📡 Modo configuración: AP activo en IP ");
            Serial.println(WiFi.softAPIP());
    
            wm.setConfigPortalBlocking(false);
            wm.startConfigPortal(_apName, _apPassword);  // No bloquea, el loop hará wm.process()

    
            // No se ejecuta nada más
            return;
        }
    
        // Si el pin está en HIGH: intentar conectar normalmente
        WiFi.mode(WIFI_STA);
        wm.setConfigPortalBlocking(false);
        bool connected = wm.autoConnect();
    
        if (connected) {
            Serial.println("✅ Conectado a red WiFi: " + WiFi.SSID());
    
            if (MDNS.begin("ph-remote")) {
                Serial.println("🔍 mDNS activo: http://ph-remote.local");
            } else {
                Serial.println("⚠️ Error iniciando mDNS");
            }
    
        } else {
            Serial.println("⚠️ No se conectó a ninguna red");
        }
    }
    

void WiFiPortalManager::loop() {
    wm.process();
}

bool WiFiPortalManager::isConnected() {
    return WiFi.status() == WL_CONNECTED;
}

IPAddress WiFiPortalManager::getLocalIP() {
    return WiFi.localIP();
}

IPAddress WiFiPortalManager::getAPIP() {
    return WiFi.softAPIP();
}


//Remote access manager implementation

RemoteAccessManager::RemoteAccessManager(const char* hostname)
    : _hostname(hostname), _telnetServer(23) {}

void RemoteAccessManager::begin() {
    setupTelnet();
    setupOTA();
}

void RemoteAccessManager::handle() {
    ArduinoOTA.handle();
    handleTelnet();
}

void RemoteAccessManager::log(const String& message) {
    Serial.println(message);
    if (_telnetClient && _telnetClient.connected()) {
        _telnetClient.println(message);
    }
}

void RemoteAccessManager::setupTelnet() {
    _telnetServer.begin();
    _telnetServer.setNoDelay(true);
    Serial.println("📡 Telnet iniciado en puerto 23");
}

void RemoteAccessManager::handleTelnet() {
    if (_telnetServer.hasClient()) {
        if (_telnetClient && _telnetClient.connected()) {
            WiFiClient newClient = _telnetServer.available();
            newClient.println("❌ Solo se permite una conexión Telnet.");
            newClient.stop();
        } else {
            _telnetClient = _telnetServer.available();
            _telnetClient.println("✅ Conexión Telnet aceptada.");
        }
    }
}

void RemoteAccessManager::setupOTA() {
    ArduinoOTA.setHostname(_hostname);
    ArduinoOTA.onStart([]() {
        Serial.println("🔄 OTA iniciada...");
    });
    ArduinoOTA.onEnd([]() {
        Serial.println("✅ OTA completada");
    });
    ArduinoOTA.onError([](ota_error_t error) {
        Serial.printf("❌ Error OTA: %u\n", error);
    });
    ArduinoOTA.begin();
    Serial.println("🚀 OTA habilitado");
}

// ---- Interactive helpers ----
bool RemoteAccessManager::connected() {
    return _telnetClient.connected();
}

int RemoteAccessManager::available() {
    if (!connected()) return 0;
    return _telnetClient.available();
}

int RemoteAccessManager::read() {
    if (!connected()) return -1;
    return _telnetClient.read();
}

String RemoteAccessManager::readLine() {
    if (!connected()) return String("");
    String line = _telnetClient.readStringUntil('\n');
    line.trim();
    return line;
}

void RemoteAccessManager::print(const String& message) {
    Serial.print(message);
    if (connected()) {
        _telnetClient.print(message);
    }
}

// Define a global instance for external use
RemoteAccessManager remoteManager;
