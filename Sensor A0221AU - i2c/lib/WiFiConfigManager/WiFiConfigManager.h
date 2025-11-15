#pragma once
#include <Arduino.h>
#include <WiFi.h>

class WiFiConfigManager {
public:

    // Estado simple del AP
    enum class APState : uint8_t {
        OFF,
        ACTIVE
    };

    WiFiConfigManager() {}

    // Inicia AP con timeout
    void begin(const char* apName, const char* apPass, uint32_t apTimeoutMs);

    // Llamado en loop()
    void update();

    // Apaga AP
    void stopAP();

    // Saber si el AP está activo
    bool isActive() const { return _state == APState::ACTIVE; }

private:
    APState _state = APState::OFF;

    uint32_t _apTimeoutMs = 0;   // Tiempo antes de apagar AP
    uint32_t _apStartMs = 0;     // Marca de tiempo cuando inició el AP
};
