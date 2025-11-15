#include "WiFiConfigManager.h"
#include <WiFi.h>

void WiFiConfigManager::begin(const char* apName, const char* apPass, uint32_t apTimeoutMs) {
    // En tu arquitectura actual, el AP REAL lo levanta ConfigPortal::startAP().
    // Aquí simplemente configuramos el temporizador para apagarlo luego.
    _apTimeoutMs = apTimeoutMs;
    _apStartMs   = millis();
    _state       = APState::ACTIVE;

    Serial.println("[WiFiConfig] Temporizador de AP iniciado");
    Serial.printf("[WiFiConfig] AP se apagará en %lu ms (si no se cambia la lógica)\n",
                  (unsigned long)_apTimeoutMs);

    // Si en algún momento quieres que WiFiConfigManager levante el AP él mismo,
    // podrías descomentar esto y quitar el softAP del ConfigPortal:
    //
    // WiFi.mode(WIFI_AP);
    // WiFi.softAP(apName, apPass);
    // Serial.print("[WiFiConfig] AP iniciado con SSID: ");
    // Serial.println(apName);
}

void WiFiConfigManager::update() {
    if (_state == APState::OFF) {
        return;
    }

    unsigned long now = millis();
    if (now - _apStartMs >= _apTimeoutMs) {
        Serial.println("[WiFiConfig] Timeout alcanzado → apagando AP");
        stopAP();
    }
}

void WiFiConfigManager::stopAP() {
    if (_state == APState::OFF) {
        return;
    }

    Serial.println("[WiFiConfig] Desconectando softAP y apagando WiFi...");

    // Desconecta el AP (borra clientes y apaga beacon)
    WiFi.softAPdisconnect(true);
    // Apaga completamente el WiFi
    WiFi.mode(WIFI_OFF);

    _state = APState::OFF;
    Serial.println("[WiFiConfig] AP apagado");
}
