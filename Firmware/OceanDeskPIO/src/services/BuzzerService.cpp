#include "BuzzerService.h"

#include <Arduino.h>
#include <esp_io_expander.hpp>

namespace
{
// CH422G OC0 is pin 8. On the Waveshare board, LOW turns on the
// optocoupler and makes the external DO0 terminal sink current.
constexpr uint8_t buzzerOutputPin = 8;
constexpr uint8_t buzzerOnLevel = LOW;
constexpr uint8_t buzzerOffLevel = HIGH;
constexpr uint8_t testBeepCount = 3;
constexpr unsigned long beepOnDurationMs = 200;
constexpr unsigned long beepOffDurationMs = 200;
constexpr unsigned long alarmBeepOnDurationMs = 250;
constexpr unsigned long alarmBeepOffDurationMs = 250;
constexpr unsigned long alarmGroupPauseMs = 1500;
constexpr unsigned long alarmMaximumDurationMs = 60UL * 1000UL;

enum class BuzzerMode
{
    Idle,
    ManualTest,
    Alarm
};

esp_expander::Base *ioExpander = nullptr;
BuzzerMode mode = BuzzerMode::Idle;
bool outputOn = false;
bool stopping = false;
uint8_t beepsStarted = 0;
unsigned long nextTransitionAt = 0;
unsigned long alarmStartedAt = 0;

bool setOutput(bool enabled)
{
    if (ioExpander == nullptr)
    {
        return false;
    }

    return ioExpander->digitalWrite(
        buzzerOutputPin,
        enabled ? buzzerOnLevel : buzzerOffLevel);
}

void finishPattern()
{
    outputOn = false;
    stopping = false;
    mode = BuzzerMode::Idle;
    beepsStarted = 0;
    nextTransitionAt = 0;
    alarmStartedAt = 0;
}

bool armPattern(BuzzerMode requestedMode)
{
    if (ioExpander == nullptr || mode != BuzzerMode::Idle || !setOutput(false))
    {
        return false;
    }

    mode = requestedMode;
    outputOn = false;
    stopping = false;
    beepsStarted = 0;
    nextTransitionAt = millis();
    alarmStartedAt = requestedMode == BuzzerMode::Alarm ? nextTransitionAt : 0;
    return true;
}
}

bool BuzzerService::begin(esp_expander::Base *expander)
{
    ioExpander = expander;
    finishPattern();

    const bool off = setOutput(false);
    Serial.println(off ? "Buzzer: ready on DO0 (OFF)"
                       : "Buzzer: failed to set DO0 OFF");
    return off;
}

BuzzerService::StartResult BuzzerService::startTest()
{
    if (ioExpander == nullptr)
    {
        return StartResult::Unavailable;
    }
    if (mode != BuzzerMode::Idle)
    {
        return StartResult::Busy;
    }

    // Validate that DO0 can be held OFF, then only arm the test here. The
    // first ON transition happens in update(), after the HTTP response has
    // been sent, so a slow network client cannot lengthen the first beep.
    if (!armPattern(BuzzerMode::ManualTest))
    {
        return StartResult::Unavailable;
    }

    Serial.println("Buzzer: manual test armed");
    return StartResult::Started;
}

bool BuzzerService::startAlarm()
{
    if (!armPattern(BuzzerMode::Alarm))
    {
        return false;
    }

    Serial.println("Buzzer: alarm started");
    return true;
}

void BuzzerService::stopAlarm()
{
    if (mode == BuzzerMode::Alarm)
    {
        stopping = true;
        nextTransitionAt = millis();
    }
}

void BuzzerService::update()
{
    if (mode == BuzzerMode::Idle)
    {
        return;
    }

    const unsigned long now = millis();
    if (mode == BuzzerMode::Alarm && now - alarmStartedAt >= alarmMaximumDurationMs)
    {
        stopping = true;
        nextTransitionAt = now;
    }

    if (stopping)
    {
        if (static_cast<long>(now - nextTransitionAt) < 0)
        {
            return;
        }
        if (!setOutput(false))
        {
            nextTransitionAt = now + 50;
            return;
        }

        const bool wasAlarm = mode == BuzzerMode::Alarm;
        finishPattern();
        Serial.println(wasAlarm ? "Buzzer: alarm stopped (OFF)"
                                : "Buzzer: pattern stopped (OFF)");
        return;
    }

    if (static_cast<long>(now - nextTransitionAt) < 0)
    {
        return;
    }

    if (outputOn)
    {
        // Do not advance or unlock until DO0 has been returned to OFF.
        // If I2C is temporarily unavailable, retry on subsequent updates.
        if (!setOutput(false))
        {
            nextTransitionAt = now + 50;
            return;
        }

        outputOn = false;
        if (mode == BuzzerMode::ManualTest && beepsStarted >= testBeepCount)
        {
            finishPattern();
            Serial.println("Buzzer: manual test finished (OFF)");
            return;
        }

        if (mode == BuzzerMode::Alarm && beepsStarted >= testBeepCount)
        {
            beepsStarted = 0;
            nextTransitionAt = now + alarmGroupPauseMs;
        }
        else
        {
            nextTransitionAt = now +
                (mode == BuzzerMode::Alarm ? alarmBeepOffDurationMs : beepOffDurationMs);
        }
        return;
    }

    if (!setOutput(true))
    {
        // Keep the output OFF and stop this test if a later ON transition
        // fails. A new explicit button press is required to try again.
        setOutput(false);
        const bool wasAlarm = mode == BuzzerMode::Alarm;
        finishPattern();
        Serial.println(wasAlarm ? "Buzzer: alarm stopped after output error"
                                : "Buzzer: manual test stopped after output error");
        return;
    }

    outputOn = true;
    ++beepsStarted;
    nextTransitionAt = now +
        (mode == BuzzerMode::Alarm ? alarmBeepOnDurationMs : beepOnDurationMs);
}

bool BuzzerService::isAlarmActive()
{
    return mode == BuzzerMode::Alarm;
}

bool BuzzerService::isActive()
{
    return mode != BuzzerMode::Idle;
}
