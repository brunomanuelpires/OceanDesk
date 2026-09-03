#include "TideService.h"

#include <time.h>

static unsigned long lastUpdate = 0;
static const unsigned long updateInterval = 15UL * 60UL * 1000UL; // 15 minutos

namespace
{
// Horas simuladas no dia-base Unix. Serão substituídas pelo motor harmónico.
constexpr time_t simulatedPreviousHigh = 6 * 60 * 60 + 18 * 60;
constexpr time_t simulatedNextLow = 12 * 60 * 60 + 45 * 60;
constexpr time_t simulatedNextHigh = 18 * 60 * 60 + 32 * 60;
}

void TideService::begin()
{
    lastUpdate = 0;
}

void TideService::update()
{
    unsigned long now = millis();

    if (lastUpdate == 0 || now - lastUpdate >= updateInterval)
    {
        lastUpdate = now;

        // Futuramente:
        // - verificar Wi-Fi
        // - pedir dados reais à API
        // - guardar próxima baixa-mar / preia-mar
        // - atualizar estado da maré
    }
}

TideSnapshot TideService::getSnapshot()
{
    TideSnapshot snapshot;

    snapshot.station.id = "ticon/nazaretg-naz-prt-cmems";
    snapshot.station.name = "Nazaré";
    snapshot.station.latitude = 39.585999;
    snapshot.station.longitude = -9.074;
    snapshot.station.source = "TICON-4";
    snapshot.station.license = "CC BY-NC 4.0";
    snapshot.station.valid = true;

    snapshot.valid = true;
    snapshot.state = TideState::Falling;
    snapshot.currentHeight = 1.42f;

    snapshot.previousEvent.type = TideEventType::High;
    snapshot.previousEvent.timestamp = simulatedPreviousHigh;
    snapshot.previousEvent.height = 2.63f;
    snapshot.previousEvent.valid = true;

    snapshot.nextLow.type = TideEventType::Low;
    snapshot.nextLow.timestamp = simulatedNextLow;
    snapshot.nextLow.height = 0.58f;
    snapshot.nextLow.valid = true;

    snapshot.nextHigh.type = TideEventType::High;
    snapshot.nextHigh.timestamp = simulatedNextHigh;
    snapshot.nextHigh.height = 2.75f;
    snapshot.nextHigh.valid = true;

    return snapshot;
}

String TideService::formatTime(time_t timestamp)
{
    if (timestamp <= 0)
    {
        return "--:--";
    }

    struct tm timeInfo;

    if (localtime_r(&timestamp, &timeInfo) == nullptr)
    {
        return "--:--";
    }

    char buffer[6];
    snprintf(
        buffer,
        sizeof(buffer),
        "%02d:%02d",
        timeInfo.tm_hour,
        timeInfo.tm_min
    );

    return String(buffer);
}
