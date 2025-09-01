#include "alerts_manager.h"


void DebugLeds::begin() {
    strip.begin();
    strip.setBrightness(50); // Ajusta el brillo de los LEDs (0-255)
    strip.show(); // Apagar todos los LEDs al inicio
}

void DebugLeds::update() {
    uint32_t now = millis();
    for (uint8_t i = 0; i < LED_COUNT; i++) {
        if (ledStates[i].fading) {
            uint32_t elapsed = now - ledStates[i].startTime;
            if (elapsed >= ledStates[i].duration) {
                strip.setPixelColor(i, ledStates[i].targetColor);
                ledStates[i].fading = false;
            } else {
                float progress = (float)elapsed / ledStates[i].duration;
                uint32_t blended = blendColors(ledStates[i].startColor, ledStates[i].targetColor, progress);
                strip.setPixelColor(i, blended);
            }
        }
    }
    strip.show();
}

void DebugLeds::setColor(uint8_t ledIndex, uint8_t r, uint8_t g, uint8_t b) {
    if (ledIndex >= LED_COUNT) return;
    uint32_t color = strip.Color(r, g, b);
    strip.setPixelColor(ledIndex, color);
    strip.show();
}

void DebugLeds::setFade(uint8_t ledIndex, uint8_t r, uint8_t g, uint8_t b, uint16_t durationMs) {
    if (ledIndex >= LED_COUNT) return;
    uint32_t color = strip.Color(r, g, b);
    ledStates[ledIndex].startColor = strip.getPixelColor(ledIndex);
    ledStates[ledIndex].targetColor = color;
    ledStates[ledIndex].startTime = millis();
    ledStates[ledIndex].duration = durationMs;
    ledStates[ledIndex].fading = true;
}

void DebugLeds::stopFade(uint8_t ledIndex) {
    if (ledIndex >= LED_COUNT) return;
    ledStates[ledIndex].fading = false;
    // Mantiene el color actual sin alterarlo
}

void DebugLeds::signalOk(uint8_t ledIndex) {
    setFade(ledIndex, 0, 150, 0, 300); // Verde
}

uint8_t DebugLeds::interpolate(uint8_t start, uint8_t end, float progress) {
    return start + (end - start) * progress;
}

uint32_t DebugLeds::blendColors(uint32_t fromColor, uint32_t toColor, float progress) {
    uint8_t fr = (fromColor >> 16) & 0xFF;
    uint8_t fg = (fromColor >> 8) & 0xFF;
    uint8_t fb = fromColor & 0xFF;

    uint8_t tr = (toColor >> 16) & 0xFF;
    uint8_t tg = (toColor >> 8) & 0xFF;
    uint8_t tb = toColor & 0xFF;

    uint8_t r = interpolate(fr, tr, progress);
    uint8_t g = interpolate(fg, tg, progress);
    uint8_t b = interpolate(fb, tb, progress);

    return strip.Color(r, g, b);
}
