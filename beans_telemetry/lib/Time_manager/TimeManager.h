#ifndef TIMEMANAGER_H
#define TIMEMANAGER_H

#include <Arduino.h>

class TimeManager {
  public:
    TimeManager(const char* ntpServer = "pool.ntp.org", int gmtOffsetSec = 0, int daylightOffsetSec = 0);
    void begin();
    String getTime();     // Retorna la hora HH:MM:SS
    String getDate();     // Retorna la fecha YYYY-MM-DD
    String getDateTime(); // Retorna YYYY-MM-DD HH:MM:SS
    time_t getRawTime();  // Retorna timestamp tipo time_t

  private:
    const char* _ntpServer;
    int _gmtOffsetSec;
    int _daylightOffsetSec;
};

#endif
