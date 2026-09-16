#include "AlarmService.h"

#include <Arduino.h>
#include <atomic>
#include <time.h>
#include "BuzzerService.h"
#include "ConfigService.h"

namespace
{
constexpr time_t minimumSynchronizedTime = 1577836800; // 2020-01-01 UTC

std::atomic_bool ringing(false);
std::atomic_bool dismissRequested(false);
bool initialized = false;
bool suspended = false;
time_t lastCheckedSecond = 0;

uint32_t localDayKey(const tm &localTime)
{
    return static_cast<uint32_t>(localTime.tm_year + 1900) * 10000UL +
           static_cast<uint32_t>(localTime.tm_mon + 1) * 100UL +
           static_cast<uint32_t>(localTime.tm_mday);
}
}

void AlarmService::begin()
{
    ringing.store(false);
    dismissRequested.store(false);
    suspended = false;
    lastCheckedSecond = 0;
    initialized = true;
    Serial.println("Alarm: ready");
}

void AlarmService::update()
{
    if (!initialized)
    {
        return;
    }

    if (dismissRequested.exchange(false))
    {
        BuzzerService::stopAlarm();
    }
    if (ringing.load() && !BuzzerService::isAlarmActive())
    {
        ringing.store(false);
    }

    if (suspended)
    {
        if (BuzzerService::isAlarmActive())
        {
            BuzzerService::stopAlarm();
        }
        return;
    }

    const DeviceConfig &config = ConfigService::get();
    if (!config.alarmEnabled)
    {
        if (BuzzerService::isAlarmActive())
        {
            BuzzerService::stopAlarm();
        }
        return;
    }
    if (ringing.load())
    {
        return;
    }

    const time_t now = time(nullptr);
    if (now < minimumSynchronizedTime || now == lastCheckedSecond)
    {
        return;
    }
    lastCheckedSecond = now;

    tm localTime;
    if (localtime_r(&now, &localTime) == nullptr ||
        localTime.tm_hour != config.alarmHour ||
        localTime.tm_min != config.alarmMinute)
    {
        return;
    }

    const uint32_t day = localDayKey(localTime);
    const uint16_t scheduledMinute =
        static_cast<uint16_t>(config.alarmHour * 60U + config.alarmMinute);
    if (ConfigService::wasAlarmTriggered(day, scheduledMinute))
    {
        return;
    }

    if (BuzzerService::startAlarm())
    {
        ringing.store(true);
        if (!ConfigService::markAlarmTriggered(day, scheduledMinute))
        {
            Serial.println("Alarm: failed to save trigger state");
        }
        Serial.printf("Alarm: triggered at %02u:%02u\n",
                      config.alarmHour, config.alarmMinute);
    }
}

void AlarmService::requestDismiss()
{
    dismissRequested.store(true);
}

void AlarmService::suspendUntilRestart()
{
    suspended = true;
    dismissRequested.store(true);
}

bool AlarmService::isRinging()
{
    return ringing.load();
}
