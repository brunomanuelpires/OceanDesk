#pragma once

class AlarmService
{
public:
    static void begin();
    static void update();
    static void requestDismiss();
    static void suspendUntilRestart();
    static bool isRinging();
};
