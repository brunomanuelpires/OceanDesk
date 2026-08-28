#pragma once

class NTPService
{
public:
    static void begin();
    static void update();
    static bool isSynced();
};