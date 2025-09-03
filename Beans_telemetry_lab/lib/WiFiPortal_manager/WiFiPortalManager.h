#ifndef WIFI_PORTAL_MANAGER_H
#define WIFI_PORTAL_MANAGER_H

#include <WiFiManager.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiServer.h>
#include <ArduinoOTA.h>
#include <globals.h>


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

class RemoteAccessManager {
public:
    RemoteAccessManager(const char* hostname = "BeansIOT_LAB");

    void begin();
    void handle();
    void log(const String& message);

private:
    const char* _hostname;

    WiFiServer _telnetServer;
    WiFiClient _telnetClient;

    void setupTelnet();
    void setupOTA();
    void handleTelnet();
};



#endif
