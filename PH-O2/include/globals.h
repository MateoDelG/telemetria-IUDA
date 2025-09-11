#ifndef GLOBALS_H
#define GLOBALS_H
#include "WiFiPortalManager.h"
// #include "LCD_manager.h"

#define SW_DOWN 36
#define SW_UP 39
#define SW_OK 35
#define SW_ESC 34
#define SENSOR_LEVEL_H2O 33
#define SENSOR_LEVEL_KCL 32

#define BUZZER 5
#define PUMP_1 13
#define PUMP_2 12
#define PUMP_3 14 
#define PUMP_4 27
#define MIXER 26

#define TEMP_SENSOR 19

#define TELNET_HOSTNAME "ph-remote"

// extern Lcd16x2 lcd;    // Declaración, no creación


extern RemoteAccessManager remoteManager;


#endif