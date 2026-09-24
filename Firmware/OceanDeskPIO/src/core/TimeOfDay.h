#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

constexpr uint16_t minutesPerDay = 24U * 60U;

inline bool formatTimeOfDay(uint16_t minuteOfDay, char *buffer, size_t bufferSize)
{
    return minuteOfDay < minutesPerDay && buffer != nullptr && bufferSize >= 6 &&
           snprintf(buffer, bufferSize, "%02u:%02u", minuteOfDay / 60U, minuteOfDay % 60U) > 0;
}
