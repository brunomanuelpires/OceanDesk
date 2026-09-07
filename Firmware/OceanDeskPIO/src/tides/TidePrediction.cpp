#include "TidePrediction.h"

#include <algorithm>
#include <cmath>

namespace
{
constexpr int64_t sampleSeconds = 5 * 60;
constexpr int64_t pastWindowSeconds = 13 * 60 * 60;
constexpr int64_t futureWindowSeconds = 26 * 60 * 60;

TideExtremum refineExtremum(const TideEngine &engine,
                            TideExtremumType type,
                            int64_t timestampUtc,
                            double previousHeight,
                            double height,
                            double nextHeight)
{
    const double curvature = previousHeight - 2.0 * height + nextHeight;
    double offsetSeconds = 0.0;
    if (std::abs(curvature) > 1e-12)
    {
        offsetSeconds = 0.5 * (previousHeight - nextHeight) / curvature * sampleSeconds;
        offsetSeconds = std::max(-static_cast<double>(sampleSeconds),
                                 std::min(static_cast<double>(sampleSeconds), offsetSeconds));
    }

    const int64_t refinedTimestamp =
        timestampUtc + static_cast<int64_t>(std::llround(offsetSeconds));
    return {type, refinedTimestamp, engine.predictHeight(refinedTimestamp), true};
}
}

TidePrediction calculateTidePrediction(const TideEngine &engine, int64_t timestampUtc)
{
    TidePrediction prediction;
    if (!engine.valid() || timestampUtc <= 0)
    {
        return prediction;
    }

    prediction.currentHeightMeters = engine.predictHeight(timestampUtc);
    if (!std::isfinite(prediction.currentHeightMeters))
    {
        return prediction;
    }

    const double earlierHeight = engine.predictHeight(timestampUtc - sampleSeconds);
    const double laterHeight = engine.predictHeight(timestampUtc + sampleSeconds);
    prediction.rising = laterHeight > earlierHeight;

    int64_t middleTimestamp = timestampUtc - pastWindowSeconds;
    double previousHeight = engine.predictHeight(middleTimestamp - sampleSeconds);
    double middleHeight = engine.predictHeight(middleTimestamp);

    const int64_t endTimestamp = timestampUtc + futureWindowSeconds;
    for (; middleTimestamp <= endTimestamp; middleTimestamp += sampleSeconds)
    {
        const double nextHeight = engine.predictHeight(middleTimestamp + sampleSeconds);
        TideExtremumType type = TideExtremumType::Unknown;
        if (middleHeight <= previousHeight && middleHeight < nextHeight)
        {
            type = TideExtremumType::Low;
        }
        else if (middleHeight >= previousHeight && middleHeight > nextHeight)
        {
            type = TideExtremumType::High;
        }

        if (type != TideExtremumType::Unknown)
        {
            const TideExtremum extremum = refineExtremum(
                engine, type, middleTimestamp, previousHeight, middleHeight, nextHeight);
            if (extremum.timestampUtc <= timestampUtc)
            {
                if (!prediction.previous.valid ||
                    extremum.timestampUtc > prediction.previous.timestampUtc)
                {
                    prediction.previous = extremum;
                }
            }
            else if (type == TideExtremumType::Low && !prediction.nextLow.valid)
            {
                prediction.nextLow = extremum;
            }
            else if (type == TideExtremumType::High && !prediction.nextHigh.valid)
            {
                prediction.nextHigh = extremum;
            }
        }

        previousHeight = middleHeight;
        middleHeight = nextHeight;
    }

    prediction.valid = prediction.previous.valid &&
                       prediction.nextLow.valid &&
                       prediction.nextHigh.valid;
    return prediction;
}
