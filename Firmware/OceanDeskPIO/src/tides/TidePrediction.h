#pragma once

#include "TideEngine.h"

#include <cstdint>

enum class TideExtremumType
{
    Unknown,
    Low,
    High
};

struct TideExtremum
{
    TideExtremumType type = TideExtremumType::Unknown;
    int64_t timestampUtc = 0;
    double heightMeters = 0.0;
    bool valid = false;
};

struct TidePrediction
{
    double currentHeightMeters = 0.0;
    bool rising = false;
    TideExtremum previous;
    TideExtremum nextLow;
    TideExtremum nextHigh;
    bool valid = false;
};

// Builds the UI-oriented tide values without changing TideEngine's public API.
TidePrediction calculateTidePrediction(const TideEngine &engine, int64_t timestampUtc);
