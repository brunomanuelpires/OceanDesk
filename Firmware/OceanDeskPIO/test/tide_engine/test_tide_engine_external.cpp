#include "tides/TideEngine.h"
#include "tides/data/NazareTideData.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <iomanip>
#include <iostream>

namespace
{
constexpr double nazareMeanLevelAboveHydrographicZeroMeters = 2.00;
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
// - Peniche predictions on page 2-43 (PDF page 65);
// - Nazare concordance on page 3-2 (PDF page 194);
// - times are Fuso 0 (TU), heights are relative to Zero Hidrografico (ZH).
// Nazare is subordinate to Peniche. Its published concordance adds 0.07 m to
// high waters and 0.09 m to low waters. Low-water times are three minutes
// earlier. Section 111.4 requires time corrections to be linearly interpolated
// too; interpolating from -4 minutes at mean neaps to -6 minutes at mean
// springs gives the high-water reference timestamps below (nearest second).
constexpr ReferenceEvent referenceEvents[] = {
    {1767226488, 3.17, EventType::High}, // 2026-01-01 00:14:48 UTC
    {1767249300, 0.89, EventType::Low},  // 2026-01-01 06:35 UTC
    {1767271548, 3.17, EventType::High}, // 2026-01-01 12:45:48 UTC
    {1767293700, 0.89, EventType::Low},  // 2026-01-01 18:55 UTC
    {1767316155, 3.37, EventType::High}, // 2026-01-02 01:09:15 UTC
    {1767339060, 0.69, EventType::Low},  // 2026-01-02 07:31 UTC
    {1767361232, 3.27, EventType::High}, // 2026-01-02 13:40:32 UTC
    {1767383220, 0.79, EventType::Low},  // 2026-01-02 19:47 UTC
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

    // Refine the one-minute grid maximum/minimum by fitting a parabola through
    // the winning sample and its immediate neighbours.
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
        refinedHeight + nazareMeanLevelAboveHydrographicZeroMeters,
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
    const TideEngine engine(TideDevelopmentData::nazare);
    assert(engine.valid());
    assert(engine.station().developmentOnly);

    constexpr size_t eventCount = sizeof(referenceEvents) / sizeof(referenceEvents[0]);
    double timingErrorsMinutes[eventCount] = {};
    double heightErrorsMeters[eventCount] = {};

    std::cout << "External validation: OceanDesk Nazare vs IH 2026 Peniche/Nazare concordance\n";
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

    // The official primary-port heights are truncated to 0.1 m. These limits
    // catch material phase/datum regressions without claiming centimetric
    // agreement that the source itself cannot support.
    assert(timing.maximumAbsolute <= 15.0);
    assert(height.maximumAbsolute <= 0.15);
    assert(height.rmse <= 0.10);

    return 0;
}
