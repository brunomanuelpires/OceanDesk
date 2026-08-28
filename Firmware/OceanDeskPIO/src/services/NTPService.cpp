#include "NTPService.h"

#include <WiFi.h>
#include <time.h>

void NTPService::begin()
{
    configTzTime(
        "WET0WEST,M3.5.0/1,M10.5.0",
        "pool.ntp.org",
        "time.nist.gov"
    );
}

void NTPService::update()
{
    // Por agora não precisa de fazer nada.
    // O ESP32 trata da sincronização NTP automaticamente
    // depois de o Wi-Fi estar ligado.
}

bool NTPService::isSynced()
{
    struct tm timeinfo;

    bool synced = getLocalTime(&timeinfo, 10);

    if (synced)
    {
        Serial.printf(
            "NTP OK: %02d:%02d:%02d\n",
            timeinfo.tm_hour,
            timeinfo.tm_min,
            timeinfo.tm_sec
        );
    }
    else
    {
        Serial.println("NTP not synchronized");
    }

    return synced;
}