#ifndef GLOBALS_H
#define GLOBALS_H
#include "WiFiPortalManager.h"
// #include "LCD_manager.h"



// extern Lcd16x2 lcd;    // Declaración, no creación

#define SW_DOWN 36
#define TELNET_HOSTNAME "ph-remote"
RemoteAccessManager remoteManager(TELNET_HOSTNAME);


#endif