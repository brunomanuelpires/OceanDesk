#include "tides/TideEngine.h"
#include "tides/data/NazareTideData.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <iomanip>
#include <iostream>

namespace
{
void expectNear(double actual, double expected, double tolerance)
{
    if (std::abs(actual - expected) > tolerance)
    {
        std::cerr << "Expected " << expected << " +/- " << tolerance << ", got " << actual << '\n';
        std::abort();
    }
}
}

int main()
{
    const TideEngine engine(TideDevelopmentData::nazare);
    assert(engine.valid());
    assert(engine.station().constituentCount == 50);
    assert(engine.station().developmentOnly);

    // The generator keeps these historical major constituents first so this
    // station can quantify the effect of expanding the same dataset to all 50.
    const TideHarmonicStation major8Station = {
        "nazare-major-8-test-baseline",
        "Nazare major 8 test baseline",
        engine.station().source,
        engine.station().license,
        engine.station().constituents,
        8,
        true,
    };
    const TideEngine major8Engine(major8Station);

    // Fixed UTC instants protect the phase and astronomical-argument convention.
    const int64_t fixedTimestamps[] = {1767225600, 1767247200, 1767268800};
    const double expectedMajor8[] = {1.146152, -1.104048, 1.015833};
    const double expectedFull50[] = {1.155426, -1.174631, 1.05407};
    double largestFixedDifference = 0.0;
    for (size_t index = 0; index < 3; ++index)
    {
        const double major8 = major8Engine.predictHeight(fixedTimestamps[index]);
        const double full50 = engine.predictHeight(fixedTimestamps[index]);
        expectNear(major8, expectedMajor8[index], 0.00001);
        expectNear(full50, expectedFull50[index], 0.00001);
        const double difference = full50 - major8;
        largestFixedDifference = std::max(largestFixedDifference, std::abs(difference));
        std::cout << "8 vs 50 at " << fixedTimestamps[index] << ": "
                  << std::fixed << std::setprecision(6) << major8 << " m -> " << full50
                  << " m (delta " << difference << " m)\n";
    }
    assert(largestFixedDifference > 0.01);

    double minimum = engine.predictHeight(1767225600);
    double maximum = minimum;
    double previous = minimum;
    for (int step = 1; step <= 48 * 12; ++step)
    {
        const double height = engine.predictHeight(1767225600 + step * 300);
        assert(std::isfinite(height));
        assert(std::abs(height - previous) < 0.25);
        minimum = std::min(minimum, height);
        maximum = std::max(maximum, height);
        previous = height;
    }

    assert(minimum > -2.0);
    assert(maximum < 2.0);
    assert(maximum - minimum > 1.5);

    const TideHarmonicStation empty = {"empty", "Empty", "test", "test", nullptr, 0, true};
    const TideEngine invalidEngine(empty);
    assert(!invalidEngine.valid());
    assert(std::isnan(invalidEngine.predictHeight(1767225600)));

    std::cout << "TideEngine sanity tests passed (50 Nazaré development constituents).\n";
    return 0;
}
