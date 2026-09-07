#pragma once

#include <Arduino.h>
#include <time.h>

enum class TideEventType
{
    Unknown,
    Low,
    High
};

struct TideEvent
{
    TideEventType type = TideEventType::Unknown;

    time_t timestamp = 0;

    // Metres relative to the station's published height datum.
    float height = 0.0f;

    bool valid = false;
};

enum class TideState
{
    Unknown,
    Rising,
    Falling
};

struct TideStation
{
    String id;
    String name;

    double latitude = 0.0;
    double longitude = 0.0;

    String source;
    String license;

    bool valid = false;
};

struct TideSnapshot
{
    TideStation station;

    TideState state = TideState::Unknown;
    // Metres relative to the station's published height datum.
    float currentHeight = 0.0f;

    TideEvent previousEvent;
    TideEvent nextLow;
    TideEvent nextHigh;

    bool valid = false;
};
