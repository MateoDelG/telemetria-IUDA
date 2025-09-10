#ifndef TEST_BOARD_H
#define TEST_BOARD_H
#include <Arduino.h>

namespace TestBoard {
    void testPumps(int pump1Pin, int pump2Pin, int pump3Pin, int pump4Pin, int mixerPin);
}

#endif // TEST_BOARD_H