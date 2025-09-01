#ifndef UBIDOTSMANAGER_H
#define UBIDOTSMANAGER_H

#include <UbidotsEsp32Mqtt.h>

class UbidotsManager {
  public:
    UbidotsManager(const char* token, const char* ssid, const char* pass, const char* deviceLabel, unsigned long interval);
    void begin();
    void update();

  private:
    const char* _ssid;
    const char* _pass;
    const char* _token;
    const char* _deviceLabel;
    unsigned long _interval;
    unsigned long _lastTime;

    Ubidots _ubidots;

    static void callback(char* topic, byte* payload, unsigned int length);
    void reconnect();
    void sendData();
};

#endif
