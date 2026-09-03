#include "tides/TideEngine.h"
#include "tides/data/NazareTideData.h"

#include <algorithm>
#include <cassert>
#include <cmath>
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
    assert(engine.station().constituentCount == 8);
    assert(engine.station().developmentOnly);

    // Fixed UTC instants protect the phase and astronomical-argument convention.
    expectNear(engine.predictHeight(1767225600), 1.146152, 0.00001);  // 2026-01-01 00:00:00Z
    expectNear(engine.predictHeight(1767247200), -1.104048, 0.00001); // 2026-01-01 06:00:00Z
    expectNear(engine.predictHeight(1767268800), 1.015833, 0.00001);  // 2026-01-01 12:00:00Z

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

    std::cout << "TideEngine sanity tests passed (8 Nazaré development constituents).\n";
    return 0;
}
