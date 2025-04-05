#include "TimeManager.h"
#include <time.h>

TimeManager::TimeManager(const char* ntpServer, int gmtOffsetSec, int daylightOffsetSec)
  : _ntpServer(ntpServer), _gmtOffsetSec(gmtOffsetSec), _daylightOffsetSec(daylightOffsetSec) {}

void TimeManager::begin() {
  configTime(_gmtOffsetSec, _daylightOffsetSec, _ntpServer);
  Serial.println("Sincronizando hora con NTP...");

  struct tm timeInfo;
  while (!getLocalTime(&timeInfo)) {
    Serial.print(".");
    delay(500);
  }

  Serial.println("\nHora sincronizada correctamente.");
}

String TimeManager::getTime() {
  struct tm timeInfo;
  if (!getLocalTime(&timeInfo)) return "00:00:00";

  char buffer[10];
  strftime(buffer, sizeof(buffer), "%H:%M:%S", &timeInfo);
  return String(buffer);
}

String TimeManager::getDate() {
  struct tm timeInfo;
  if (!getLocalTime(&timeInfo)) return "1970-01-01";

  char buffer[11];
  strftime(buffer, sizeof(buffer), "%Y-%m-%d", &timeInfo);
  return String(buffer);
}

String TimeManager::getDateTime() {
  struct tm timeInfo;
  if (!getLocalTime(&timeInfo)) return "1970-01-01 00:00:00";

  char buffer[20];
  strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &timeInfo);
  return String(buffer);
}

time_t TimeManager::getRawTime() {
  time_t now;
  time(&now);
  return now;
}
