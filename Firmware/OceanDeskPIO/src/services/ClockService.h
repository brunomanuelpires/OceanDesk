#pragma once

#include <Arduino.h>

class ClockService
{
public:
    static void begin();
    static void update();

    static String getTime();
    static String getDate();
    static String getWeekday();
};