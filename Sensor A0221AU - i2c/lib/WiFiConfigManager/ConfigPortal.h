#pragma once
#include <Arduino.h>
#include <WiFi.h>
#include <DNSServer.h>
#include <WebServer.h>

class ConfigPortal {
public:
  ConfigPortal() : _server(80) {}

  void begin();
  void loop();

  bool   isActive() const { return _active; }
  String getApName() const { return _apName; }

private:
  DNSServer _dns;
  WebServer _server;
  bool _active = false;
  String _apName;

  void startAP();
  String makeApName();
  String macSuffix();
  void setupRoutes();
};
