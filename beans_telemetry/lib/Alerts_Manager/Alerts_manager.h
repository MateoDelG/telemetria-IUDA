#ifndef ALERTS_MANAGER_H
#define ALERTS_MANAGER_H


#include <Arduino.h>
#include <Adafruit_NeoPixel.h>

#define LED_COUNT 2
#define LED_PIN 15  // Cambia según tu conexión


// Estructura de estado para cada LED
struct LedState {
    uint32_t startColor = 0;
    uint32_t targetColor = 0;
    uint32_t startTime = 0;
    uint16_t duration = 0;
    bool fading = false;
};

class DebugLeds {
public:
    void begin();
    void update();

    void setColor(uint8_t ledIndex, uint8_t r, uint8_t g, uint8_t b);
    void setFade(uint8_t ledIndex, uint8_t r, uint8_t g, uint8_t b, uint16_t durationMs);
    void stopFade(uint8_t ledIndex);

    void signalOk(uint8_t ledIndex);

private:
    Adafruit_NeoPixel strip = Adafruit_NeoPixel(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);
    LedState ledStates[LED_COUNT];

    uint8_t interpolate(uint8_t start, uint8_t end, float progress);
    uint32_t blendColors(uint32_t fromColor, uint32_t toColor, float progress);
};

#endif
