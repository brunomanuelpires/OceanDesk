#include "tides/TideEngine.h"
#include "tides/TidePrediction.h"
#include "tides/data/NazareTideData.h"
#include "tides/data/NazareTideDatum.h"

#include <cassert>
#include <cmath>
#include <iostream>

int main()
{
    const TideEngine engine(TideDevelopmentData::nazare);
    const int64_t timestampUtc = 1767225600; // 2026-01-01 00:00 UTC
    const TidePrediction prediction = calculateTidePrediction(engine, timestampUtc);

    assert(prediction.valid);
    assert(std::isfinite(prediction.currentHeightMeters));
    assert(std::abs(TideDatum::nazareHeightAboveHydrographicZero(
                        prediction.currentHeightMeters) -
                    3.104126) < 0.00001);
    assert(prediction.rising);
    assert(prediction.previous.timestampUtc < timestampUtc);
    assert(prediction.nextHigh.type == TideExtremumType::High);
    assert(prediction.nextLow.type == TideExtremumType::Low);
    assert(prediction.nextHigh.timestampUtc > timestampUtc);
    assert(prediction.nextLow.timestampUtc > prediction.nextHigh.timestampUtc);

    // These reference extrema are independently sourced from the IH 2026
    // Peniche/Nazaré concordance also used by the external engine validation.
    assert(std::abs(prediction.nextHigh.timestampUtc - 1767226488) <= 15 * 60);
    assert(std::abs(prediction.nextLow.timestampUtc - 1767249300) <= 15 * 60);

    // Crossing midnight must not discard the latest event from the previous
    // calendar day. Event selection is chronological, not grouped by date.
    const TidePrediction beforeMidnight =
        calculateTidePrediction(engine, timestampUtc - 60);
    const TidePrediction afterMidnight =
        calculateTidePrediction(engine, timestampUtc + 60);
    assert(beforeMidnight.valid);
    assert(afterMidnight.valid);
    assert(afterMidnight.previous.valid);
    assert(afterMidnight.previous.timestampUtc < timestampUtc);
    assert(beforeMidnight.previous.timestampUtc == afterMidnight.previous.timestampUtc);
    assert(beforeMidnight.previous.type == afterMidnight.previous.type);
    assert(beforeMidnight.nextLow.timestampUtc == afterMidnight.nextLow.timestampUtc);
    assert(beforeMidnight.nextHigh.timestampUtc == afterMidnight.nextHigh.timestampUtc);

    const TideHarmonicStation empty = {"empty", "Empty", "test", "test", nullptr, 0, true};
    const TidePrediction invalid = calculateTidePrediction(TideEngine(empty), timestampUtc);
    assert(!invalid.valid);

    std::cout << "TidePrediction event tests passed.\n";
    return 0;
}
