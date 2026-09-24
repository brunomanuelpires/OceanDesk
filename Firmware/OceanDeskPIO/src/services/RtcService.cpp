#include "RtcService.h"

#include <Arduino.h>
#include <driver/i2c.h>
#include <sys/time.h>
#include <time.h>

namespace
{
constexpr i2c_port_t rtcI2cPort = I2C_NUM_0;
constexpr uint8_t rtcAddress = 0x51;
constexpr uint8_t controlRegister = 0x00;
constexpr uint8_t secondsRegister = 0x04;
constexpr uint8_t stopClockMask = 0x20;
constexpr uint8_t voltageLowMask = 0x80;
constexpr time_t minimumValidTime = 1577836800; // 2020-01-01 UTC
constexpr unsigned long syncInterval = 15UL * 60UL * 1000UL;
constexpr TickType_t i2cTimeout = pdMS_TO_TICKS(100);
bool available = false;
unsigned long lastSyncAt = 0;

bool readRegisters(uint8_t startRegister, uint8_t *data, size_t length)
{
    return i2c_master_write_read_device(rtcI2cPort, rtcAddress, &startRegister, 1,
                                        data, length, i2cTimeout) == ESP_OK;
}

bool writeRegisters(uint8_t startRegister, const uint8_t *data, size_t length)
{
    uint8_t buffer[8];
    if (length > sizeof(buffer) - 1) return false;
    buffer[0] = startRegister;
    memcpy(buffer + 1, data, length);
    return i2c_master_write_to_device(rtcI2cPort, rtcAddress, buffer, length + 1,
                                      i2cTimeout) == ESP_OK;
}

uint8_t toBcd(uint8_t value)
{
    return static_cast<uint8_t>((value / 10U) << 4U | (value % 10U));
}

bool fromBcd(uint8_t value, uint8_t &result)
{
    const uint8_t tens = value >> 4U;
    const uint8_t units = value & 0x0FU;
    if (tens > 9 || units > 9) return false;
    result = tens * 10U + units;
    return true;
}

bool isLeapYear(uint16_t year)
{
    return year % 4 == 0 && (year % 100 != 0 || year % 400 == 0);
}

uint8_t daysInMonth(uint16_t year, uint8_t month)
{
    constexpr uint8_t days[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    return month == 2 && isLeapYear(year) ? 29 : days[month - 1];
}

bool utcTimestamp(const struct tm &utc, time_t &timestamp)
{
    const uint16_t year = utc.tm_year + 1900;
    const uint8_t month = utc.tm_mon + 1;
    if (year < 2020 || year > 2069 || month == 0 || month > 12 || utc.tm_mday == 0 ||
        utc.tm_mday > daysInMonth(year, month))
    {
        return false;
    }

    uint32_t days = 0;
    for (uint16_t currentYear = 1970; currentYear < year; ++currentYear)
    {
        days += isLeapYear(currentYear) ? 366U : 365U;
    }
    for (uint8_t currentMonth = 1; currentMonth < month; ++currentMonth)
    {
        days += daysInMonth(year, currentMonth);
    }
    days += utc.tm_mday - 1;
    timestamp = static_cast<time_t>(days * 86400UL + utc.tm_hour * 3600UL +
                                    utc.tm_min * 60UL + utc.tm_sec);
    return true;
}

bool readTime(time_t &timestamp)
{
    uint8_t value[7];
    if (!readRegisters(secondsRegister, value, sizeof(value)) ||
        (value[0] & voltageLowMask) != 0)
    {
        return false;
    }

    struct tm utc = {};
    uint8_t second, minute, hour, day, weekday, month, year;
    if (!fromBcd(value[0] & 0x7F, second) || !fromBcd(value[1] & 0x7F, minute) ||
        !fromBcd(value[2] & 0x3F, hour) || !fromBcd(value[3] & 0x3F, day) ||
        !fromBcd(value[4] & 0x07, weekday) || !fromBcd(value[5] & 0x1F, month) ||
        !fromBcd(value[6], year) || second > 59 || minute > 59 || hour > 23 || day == 0 ||
        month == 0 || month > 12 || weekday > 6)
    {
        return false;
    }

    utc.tm_sec = second;
    utc.tm_min = minute;
    utc.tm_hour = hour;
    utc.tm_mday = day;
    utc.tm_mon = month - 1;
    utc.tm_year = year + 70;
    if (!utcTimestamp(utc, timestamp) || timestamp < minimumValidTime) return false;

    struct tm verification;
    gmtime_r(&timestamp, &verification);
    return verification.tm_year == utc.tm_year && verification.tm_mon == utc.tm_mon &&
           verification.tm_mday == utc.tm_mday && verification.tm_hour == utc.tm_hour &&
           verification.tm_min == utc.tm_min && verification.tm_sec == utc.tm_sec;
}

bool writeTime(time_t timestamp)
{
    struct tm utc;
    if (gmtime_r(&timestamp, &utc) == nullptr || utc.tm_year < 70 || utc.tm_year > 169) return false;

    const uint8_t values[] = {
        toBcd(utc.tm_sec),
        toBcd(utc.tm_min),
        toBcd(utc.tm_hour),
        toBcd(utc.tm_mday),
        toBcd(utc.tm_wday),
        toBcd(utc.tm_mon + 1),
        toBcd(utc.tm_year - 70)
    };
    return writeRegisters(secondsRegister, values, sizeof(values));
}
}

void RtcService::begin()
{
    uint8_t control = 0;
    if (!readRegisters(controlRegister, &control, 1))
    {
        Serial.println("RTC: chip not found");
        return;
    }
    available = true;

    if ((control & stopClockMask) != 0)
    {
        control &= ~stopClockMask;
        writeRegisters(controlRegister, &control, 1);
    }

    time_t timestamp;
    if (!readTime(timestamp))
    {
        Serial.println("RTC: no valid retained time");
        return;
    }

    const timeval systemTime = {timestamp, 0};
    settimeofday(&systemTime, nullptr);
    Serial.println("RTC: restored retained time");
}

void RtcService::update()
{
    if (!available || (lastSyncAt != 0 && millis() - lastSyncAt < syncInterval)) return;

    const time_t timestamp = time(nullptr);
    if (timestamp < minimumValidTime) return;
    if (writeTime(timestamp))
    {
        lastSyncAt = millis();
        Serial.println("RTC: time saved");
    }
}
