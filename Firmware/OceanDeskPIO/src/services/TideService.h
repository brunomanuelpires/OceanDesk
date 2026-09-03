#pragma once

#include <Arduino.h>
#include "models/TideData.h"

class TideService
{
public:
    static void begin();
    static void update();

    static TideSnapshot getSnapshot();
    static String formatTime(time_t timestamp);
};
