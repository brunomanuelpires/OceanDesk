#include "tides/TideEngine.h"
#include "tides/data/PenicheTideData.h"
#include "tides/data/PenicheTideDatum.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <iomanip>
#include <iostream>

namespace
{
constexpr int64_t searchHalfWindowSeconds = 3 * 60 * 60;
constexpr int64_t searchStepSeconds = 60;

enum class EventType
{
    High,
    Low,
};

struct ReferenceEvent
{
    int64_t timestampUtc;
    double heightZhMeters;
    EventType type;
};

struct PredictedEvent
{
    double timestampUtc;
    double heightZhMeters;
};

struct ErrorMetrics
{
    double mean;
    double maximumAbsolute;
    double rmse;
};

// Instituto Hidrografico, Tabela de Mares 2026, Volume I:
// - Peniche primary-port predictions on page 2-43 (PDF page 65);
// - times are Fuso 0 (TU), heights are relative to Zero Hidrografico (ZH).
// These are the eight published high/low events for 1-2 January 2026.
constexpr ReferenceEvent referenceEvents[] = {
    {1767226800, 3.10, EventType::High}, // 2026-01-01 00:20 UTC
    {1767249480, 0.80, EventType::Low},  // 2026-01-01 06:38 UTC
    {1767271860, 3.10, EventType::High}, // 2026-01-01 12:51 UTC
    {1767293880, 0.80, EventType::Low},  // 2026-01-01 18:58 UTC
    {1767316500, 3.30, EventType::High}, // 2026-01-02 01:15 UTC
    {1767339240, 0.60, EventType::Low},  // 2026-01-02 07:34 UTC
    {1767361560, 3.20, EventType::High}, // 2026-01-02 13:46 UTC
    {1767383400, 0.70, EventType::Low},  // 2026-01-02 19:50 UTC
};

double anomaly(const TideEngine &engine, int64_t timestampUtc)
{
    return engine.predictHeight(timestampUtc);
}

PredictedEvent findNearbyExtremum(const TideEngine &engine, const ReferenceEvent &reference)
{
    int64_t bestTimestamp = reference.timestampUtc - searchHalfWindowSeconds;
    double bestHeight = anomaly(engine, bestTimestamp);

    for (int64_t timestamp = bestTimestamp + searchStepSeconds;
         timestamp <= reference.timestampUtc + searchHalfWindowSeconds;
         timestamp += searchStepSeconds)
    {
        const double height = anomaly(engine, timestamp);
        const bool isBetter = reference.type == EventType::High ? height > bestHeight : height < bestHeight;
        if (isBetter)
        {
            bestTimestamp = timestamp;
            bestHeight = height;
        }
    }

    const double left = anomaly(engine, bestTimestamp - searchStepSeconds);
    const double center = bestHeight;
    const double right = anomaly(engine, bestTimestamp + searchStepSeconds);
    const double denominator = left - 2.0 * center + right;
    const double offsetSteps = std::abs(denominator) > 1e-12
                                   ? 0.5 * (left - right) / denominator
                                   : 0.0;
    const double boundedOffset = std::clamp(offsetSteps, -1.0, 1.0);
    const double refinedTimestamp =
        static_cast<double>(bestTimestamp) + boundedOffset * searchStepSeconds;
    const double refinedHeight = center - 0.25 * (left - right) * boundedOffset;

    return {
        refinedTimestamp,
        TideDatum::penicheHeightAboveHydrographicZero(refinedHeight),
    };
}

ErrorMetrics metrics(const double *errors, size_t count)
{
    double sum = 0.0;
    double sumSquares = 0.0;
    double maximumAbsolute = 0.0;
    for (size_t index = 0; index < count; ++index)
    {
        sum += errors[index];
        sumSquares += errors[index] * errors[index];
        maximumAbsolute = std::max(maximumAbsolute, std::abs(errors[index]));
    }
    return {sum / count, maximumAbsolute, std::sqrt(sumSquares / count)};
}
} // namespace

int main()
{
    const TideEngine engine(TideDevelopmentData::peniche);
    assert(engine.valid());
    assert(engine.station().developmentOnly);

    constexpr size_t eventCount = sizeof(referenceEvents) / sizeof(referenceEvents[0]);
    double timingErrorsMinutes[eventCount] = {};
    double heightErrorsMeters[eventCount] = {};

    std::cout << "External validation: OceanDesk Peniche vs IH 2026 Peniche primary-port table\n";
    std::cout << "type,reference_utc,predicted_epoch_s,time_error_min,reference_zh_m,predicted_zh_m,height_error_m\n";
    for (size_t index = 0; index < eventCount; ++index)
    {
        const ReferenceEvent &reference = referenceEvents[index];
        const PredictedEvent predicted = findNearbyExtremum(engine, reference);
        timingErrorsMinutes[index] =
            (predicted.timestampUtc - static_cast<double>(reference.timestampUtc)) / 60.0;
        heightErrorsMeters[index] = predicted.heightZhMeters - reference.heightZhMeters;

        std::cout << (reference.type == EventType::High ? "high" : "low") << ','
                  << reference.timestampUtc << ',' << std::fixed << std::setprecision(1)
                  << predicted.timestampUtc << ',' << std::setprecision(2)
                  << timingErrorsMinutes[index] << ',' << reference.heightZhMeters << ','
                  << predicted.heightZhMeters << ',' << heightErrorsMeters[index] << '\n';
    }

    const ErrorMetrics timing = metrics(timingErrorsMinutes, eventCount);
    const ErrorMetrics height = metrics(heightErrorsMeters, eventCount);
    std::cout << "timing_minutes: mean=" << timing.mean
              << ", max_abs=" << timing.maximumAbsolute << ", rmse=" << timing.rmse << '\n';
    std::cout << "height_meters: mean=" << height.mean
              << ", max_abs=" << height.maximumAbsolute << ", rmse=" << height.rmse << '\n';

    // Match the established Nazaré validation gates. The official heights are
    // truncated to 0.1 m, so tighter claims would exceed source precision.
    assert(timing.maximumAbsolute <= 15.0);
    assert(height.maximumAbsolute <= 0.15);
    assert(height.rmse <= 0.10);

    return 0;
}
