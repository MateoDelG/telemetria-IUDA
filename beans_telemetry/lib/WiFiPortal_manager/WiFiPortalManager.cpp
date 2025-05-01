#include "WiFiPortalManager.h"
#include <WiFi.h>
#include <ESPmDNS.h>

WiFiPortalManager::WiFiPortalManager(const char* apName, const char* apPassword, int apTriggerPin)
    : _apName(apName), _apPassword(apPassword), _apTriggerPin(apTriggerPin) {}

    void WiFiPortalManager::begin() {
        if (_apTriggerPin != -1) {
            pinMode(_apTriggerPin, INPUT_PULLUP);  // Se asume pulsador con pull-up
        }
    
        // Si el pin está en LOW: solo activar modo AP y servir el portal
        if (_apTriggerPin == -1 || digitalRead(_apTriggerPin) == LOW) {
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
