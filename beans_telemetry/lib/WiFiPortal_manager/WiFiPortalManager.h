#ifndef WIFI_PORTAL_MANAGER_H
#define WIFI_PORTAL_MANAGER_H

#include <WiFiManager.h>

class WiFiPortalManager {
public:
    WiFiPortalManager(const char* apName = "ESP32-Config", const char* apPassword = "clave123", int apTriggerPin = -1);

    void begin();
    void loop();

    bool isConnected();
    IPAddress getLocalIP();
    IPAddress getAPIP();

private:
    WiFiManager wm;
    const char* _apName;
    const char* _apPassword;
    int _apTriggerPin;
    bool _apModeEnabled = false;
};

#endif
