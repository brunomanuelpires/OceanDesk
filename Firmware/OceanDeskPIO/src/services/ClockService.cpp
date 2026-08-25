#include "ClockService.h"

void ClockService::begin()
{
    // Futuramente:
    // - WiFi
    // - NTP
    // - Timezone
}

void ClockService::update()
{
    // Futuramente será atualizado uma vez por segundo.
}

String ClockService::getTime()
{
    return "23:42";
}

String ClockService::getDate()
{
    return "24 Agosto 2026";
}

String ClockService::getWeekday()
{
    return "Segunda-feira";
}