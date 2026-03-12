#include "WiFiPortalManager.h"
#include <WiFi.h>
#include <ESPmDNS.h>


WiFiPortalManager::WiFiPortalManager(const char* apName, const char* apPassword, int apTriggerPin)
    : _apName(apName), _apPassword(apPassword), _apTriggerPin(apTriggerPin) {}

    void WiFiPortalManager::begin() {
        // pinMode(_apTriggerPin, INPUT_PULLUP);  // Se asume pulsador con pull-up
        // if (_apTriggerPin != -1) {
        // }
    
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
    
            wm.setConfigPortalBlocking(true);
            wm.startConfigPortal(_apName, _apPassword);  // Se queda aquí sirviendo el portal

    
            // No se ejecuta nada más
            return;
        }
    
        // Si el pin está en HIGH: intentar conectar normalmente
        WiFi.mode(WIFI_STA);
        wm.setConfigPortalBlocking(false);
        bool connected = wm.autoConnect();
    
        if (connected) {
            Serial.println("✅ Conectado a red WiFi: " + WiFi.SSID());
    
            if (MDNS.begin("esp32")) {
                Serial.println("🔍 mDNS activo: http://esp32.local");
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
    if (_logHook) {
        _logHook(message);
    }
    if (_telnetClient && _telnetClient.connected()) {
        _telnetClient.println(message);
    }
}

void RemoteAccessManager::setLogHook(LogHook hook) {
    _logHook = hook;
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

    if (_telnetClient && _telnetClient.available()) {
        String command = _telnetClient.readStringUntil('\n');
        command.trim();
        if (command == "reboot") {
            _telnetClient.println("♻️ Reiniciando...");
            delay(1000);
            ESP.restart();
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
