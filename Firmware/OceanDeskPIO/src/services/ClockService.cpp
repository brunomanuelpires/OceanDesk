#include "ClockService.h"

#include <time.h>
#include "RtcService.h"

void ClockService::begin()
{
    RtcService::begin();
}

void ClockService::update()
{
    RtcService::update();
}

String ClockService::getTime()
{
    struct tm timeinfo;

    if (!getLocalTime(&timeinfo, 10))
    {
        return "--:--";
    }

    char buffer[6];

    snprintf(
        buffer,
        sizeof(buffer),
        "%02d:%02d",
        timeinfo.tm_hour,
        timeinfo.tm_min
    );

    return String(buffer);
}

String ClockService::getDate()
{
    struct tm timeinfo;

    if (!getLocalTime(&timeinfo, 10))
    {
        return "--";
    }

    const char *months[] =
    {
        "Janeiro",
        "Fevereiro",
        "Março",
        "Abril",
        "Maio",
        "Junho",
        "Julho",
        "Agosto",
        "Setembro",
        "Outubro",
        "Novembro",
        "Dezembro"
    };

    String result =
        String(timeinfo.tm_mday) +
        " " +
        months[timeinfo.tm_mon] +
        " " +
        String(timeinfo.tm_year + 1900);

    return result;
}

String ClockService::getWeekday()
{
    struct tm timeinfo;

    if (!getLocalTime(&timeinfo, 10))
    {
        return "";
    }

    const char *weekdays[] =
    {
        "Domingo",
        "Segunda-feira",
        "Terça-feira",
        "Quarta-feira",
        "Quinta-feira",
        "Sexta-feira",
        "Sábado"
    };

    return String(weekdays[timeinfo.tm_wday]);
}
