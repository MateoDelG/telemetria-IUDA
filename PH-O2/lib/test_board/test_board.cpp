#include "test_board.h"

namespace TestBoard {
    void testPumps(int pump1Pin, int pump2Pin, int pump3Pin, int pump4Pin, int mixerPin) {
        digitalWrite(pump1Pin, HIGH);
        digitalWrite(pump2Pin, HIGH);
        digitalWrite(pump3Pin, HIGH);
        digitalWrite(pump4Pin, HIGH);
        digitalWrite(mixerPin, HIGH);
        delay(2000);
        digitalWrite(pump1Pin, LOW);
        digitalWrite(pump2Pin, LOW);
        digitalWrite(pump3Pin, LOW);
        digitalWrite(pump4Pin, LOW);
        digitalWrite(mixerPin, LOW);
        delay(2000);
    }
}