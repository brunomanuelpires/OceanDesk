#pragma once

namespace esp_expander
{
class Base;
}

class BuzzerService
{
public:
    enum class StartResult
    {
        Started,
        Busy,
        Unavailable
    };

    static bool begin(esp_expander::Base *expander);
    static StartResult startTest();
    static bool startAlarm();
    static void stopAlarm();
    static void update();
    static bool isAlarmActive();
    static bool isActive();
};
