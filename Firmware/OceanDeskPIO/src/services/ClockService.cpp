#include "ClockService.h"

#include <Arduino.h>
#include <time.h>
#include <sys/time.h>
#include <string.h>
#include <stdio.h>

void ClockService::begin()
{
    // Portugal continental:
    // UTC no inverno / UTC+1 no verão
    setenv("TZ", "WET0WEST,M3.5.0/1,M10.5.0/2", 1);
    tzset();

    time_t now = time(nullptr);

    // Se o ESP32 ainda não tiver uma hora válida,
    // usa a data/hora da compilação do firmware.
    if (now < 1700000000)
    {
        setTimeFromBuild();
    }
}

void ClockService::setTimeFromBuild()
{
    const char *months = "JanFebMarAprMayJunJulAugSepOctNovDec";

    char monthText[4] = {};
    int day = 1;
    int year = 2026;
    int hour = 0;
    int minute = 0;
    int second = 0;

    sscanf(__DATE__, "%3s %d %d", monthText, &day, &year);
    sscanf(__TIME__, "%d:%d:%d", &hour, &minute, &second);

    int month = 0;

    for (int i = 0; i < 12; i++)
    {
        if (strncmp(monthText, months + (i * 3), 3) == 0)
        {
            month = i;
            break;
        }
    }

    struct tm buildTime = {};

    buildTime.tm_year = year - 1900;
    buildTime.tm_mon = month;
    buildTime.tm_mday = day;
    buildTime.tm_hour = hour;
    buildTime.tm_min = minute;
    buildTime.tm_sec = second;
    buildTime.tm_isdst = -1;

    time_t timestamp = mktime(&buildTime);

    struct timeval tv = {};
    tv.tv_sec = timestamp;
    tv.tv_usec = 0;

    settimeofday(&tv, nullptr);
}

void ClockService::getTime(char *buffer, size_t size)
{
    time_t now = time(nullptr);

    struct tm timeInfo = {};
    localtime_r(&now, &timeInfo);

    snprintf(
        buffer,
        size,
        "%02d:%02d",
        timeInfo.tm_hour,
        timeInfo.tm_min
    );
}

void ClockService::getDate(char *buffer, size_t size)
{
    static const char *weekdays[] =
    {
        "domingo",
        "segunda-feira",
        "terça-feira",
        "quarta-feira",
        "quinta-feira",
        "sexta-feira",
        "sábado"
    };

    static const char *months[] =
    {
        "janeiro",
        "fevereiro",
        "março",
        "abril",
        "maio",
        "junho",
        "julho",
        "agosto",
        "setembro",
        "outubro",
        "novembro",
        "dezembro"
    };

    time_t now = time(nullptr);

    struct tm timeInfo = {};
    localtime_r(&now, &timeInfo);

    snprintf(
        buffer,
        size,
        "%s, %d %s",
        weekdays[timeInfo.tm_wday],
        timeInfo.tm_mday,
        months[timeInfo.tm_mon]
    );
}