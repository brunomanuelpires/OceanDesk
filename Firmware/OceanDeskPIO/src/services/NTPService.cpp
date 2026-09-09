#include "NTPService.h"

#include <WiFi.h>
#include <time.h>

namespace
{
bool wasConnected = false;

void requestTimeSynchronization()
{
    configTzTime(
        "WET0WEST,M3.5.0/1,M10.5.0",
        "pool.ntp.org",
        "time.nist.gov"
    );
}
}

void NTPService::begin()
{
    wasConnected = WiFi.status() == WL_CONNECTED;
    requestTimeSynchronization();
}

void NTPService::update()
{
    const bool connected = WiFi.status() == WL_CONNECTED;
    if (connected && !wasConnected)
    {
        Serial.println("NTP: network available, requesting synchronization...");
        requestTimeSynchronization();
    }

    wasConnected = connected;
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
