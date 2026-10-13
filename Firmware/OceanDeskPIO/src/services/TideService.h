#pragma once

#include <Arduino.h>
#include "models/TideData.h"

class TideService
{
public:
    static void begin();
    // True only when the retained snapshot was recomputed.
    static bool update();

    static TideSnapshot getSnapshot();
    static String formatTime(time_t timestamp);
};
