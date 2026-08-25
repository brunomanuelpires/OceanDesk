#pragma once

#include <stddef.h>

class ClockService
{
public:
    static void begin();

    static void getTime(char *buffer, size_t size);
    static void getDate(char *buffer, size_t size);

private:
    static void setTimeFromBuild();
};