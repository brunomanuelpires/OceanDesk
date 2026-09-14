#include "tides/TideEngine.h"
#include "tides/data/NazareTideData.h"
#include "tides/data/PenicheTideData.h"
#include "tides/data/TideModelRegistry.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstring>
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
    const double expectedUncorrectedMajor8[] = {1.146152, -1.104048, 1.015833};
    const double expectedUncorrectedFull50[] = {1.155426, -1.174631, 1.054071};
    // Independently calculated with @neaps/tide-predictor@0.11.0 using its
    // default IHO fundamentals evaluated at each exact UTC timestamp.
    const double expectedCorrectedMajor8[] = {1.094114, -1.057498, 0.949918};
    const double expectedCorrectedFull50[] = {1.104126, -1.125210, 0.987875};
    double largestFixedDifference = 0.0;
    for (size_t index = 0; index < 3; ++index)
    {
        const double uncorrectedMajor8 =
            major8Engine.predictHeightWithoutNodalCorrections(fixedTimestamps[index]);
        const double uncorrectedFull50 =
            engine.predictHeightWithoutNodalCorrections(fixedTimestamps[index]);
        const double major8 = major8Engine.predictHeight(fixedTimestamps[index]);
        const double full50 = engine.predictHeight(fixedTimestamps[index]);
        expectNear(uncorrectedMajor8, expectedUncorrectedMajor8[index], 0.00001);
        expectNear(uncorrectedFull50, expectedUncorrectedFull50[index], 0.00001);
        expectNear(major8, expectedCorrectedMajor8[index], 0.00001);
        expectNear(full50, expectedCorrectedFull50[index], 0.00001);
        const double nodalDifference = full50 - uncorrectedFull50;
        largestFixedDifference = std::max(largestFixedDifference, std::abs(nodalDifference));
        std::cout << "nodal correction at " << fixedTimestamps[index] << ": "
                  << std::fixed << std::setprecision(6) << uncorrectedFull50 << " m -> " << full50
                  << " m (delta " << nodalDifference << " m; corrected 8: " << major8 << " m)\n";
    }
    assert(largestFixedDifference > 0.05);

    const TideEngine penicheEngine(TideDevelopmentData::peniche);
    assert(penicheEngine.valid());
    assert(penicheEngine.station().constituentCount == 50);
    assert(penicheEngine.station().developmentOnly);

    const TideHarmonicStation penicheMajor8Station = {
        "peniche-major-8-test-baseline",
        "Peniche major 8 test baseline",
        penicheEngine.station().source,
        penicheEngine.station().license,
        penicheEngine.station().constituents,
        8,
        true,
    };
    const TideEngine penicheMajor8Engine(penicheMajor8Station);
    const double expectedPenicheUncorrectedMajor8[] = {1.203909, -1.156815, 1.065102};
    const double expectedPenicheUncorrectedFull50[] = {1.221074, -1.221579, 1.116695};
    const double expectedPenicheCorrectedMajor8[] = {1.149510, -1.108754, 0.996447};
    const double expectedPenicheCorrectedFull50[] = {1.163083, -1.170339, 1.043924};
    for (size_t index = 0; index < 3; ++index)
    {
        expectNear(penicheMajor8Engine.predictHeightWithoutNodalCorrections(fixedTimestamps[index]),
                   expectedPenicheUncorrectedMajor8[index], 0.00001);
        expectNear(penicheEngine.predictHeightWithoutNodalCorrections(fixedTimestamps[index]),
                   expectedPenicheUncorrectedFull50[index], 0.00001);
        expectNear(penicheMajor8Engine.predictHeight(fixedTimestamps[index]),
                   expectedPenicheCorrectedMajor8[index], 0.00001);
        expectNear(penicheEngine.predictHeight(fixedTimestamps[index]),
                   expectedPenicheCorrectedFull50[index], 0.00001);
    }

    // Direct f/u checks keep the nodal calculation testable independently of
    // the harmonic sum and cover simple, nonlinear and compound corrections.
    const TideNodalCorrection m2 = engine.nodalCorrection(0, fixedTimestamps[0]);
    const TideNodalCorrection k2 = engine.nodalCorrection(3, fixedTimestamps[0]);
    const TideNodalCorrection m1 = engine.nodalCorrection(10, fixedTimestamps[0]);
    const TideNodalCorrection l2 = engine.nodalCorrection(18, fixedTimestamps[0]);
    const TideNodalCorrection m3 = engine.nodalCorrection(24, fixedTimestamps[0]);
    const TideNodalCorrection mks2 = engine.nodalCorrection(31, fixedTimestamps[0]);
    const TideNodalCorrection oo1 = engine.nodalCorrection(36, fixedTimestamps[0]);
    expectNear(m2.amplitudeFactor, 0.965354186128, 0.0000001);
    expectNear(m2.phaseDegrees, 0.655280065452, 0.0000001);
    expectNear(k2.amplitudeFactor, 1.302998844632, 0.0000001);
    expectNear(k2.phaseDegrees, 5.067802877059, 0.0000001);
    expectNear(m1.amplitudeFactor, 1.491908124306, 0.0000001);
    expectNear(m1.phaseDegrees, 46.018473292334, 0.0000001);
    expectNear(l2.amplitudeFactor, 1.230427719886, 0.0000001);
    expectNear(l2.phaseDegrees, -12.948722930824, 0.0000001);
    expectNear(m3.amplitudeFactor, 0.948484037371, 0.0000001);
    expectNear(m3.phaseDegrees, 0.982920098178, 0.0000001);
    expectNear(mks2.amplitudeFactor, 1.257855389185, 0.0000001);
    expectNear(mks2.phaseDegrees, 5.723082942511, 0.0000001);
    expectNear(oo1.amplitudeFactor, 1.542450062078, 0.0000001);
    expectNear(oo1.phaseDegrees, 7.746328646291, 0.0000001);

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

    double penicheMinimum = penicheEngine.predictHeight(1767225600);
    double penicheMaximum = penicheMinimum;
    double penichePrevious = penicheMinimum;
    for (int step = 1; step <= 48 * 12; ++step)
    {
        const double height = penicheEngine.predictHeight(1767225600 + step * 300);
        assert(std::isfinite(height));
        assert(std::abs(height - penichePrevious) < 0.25);
        penicheMinimum = std::min(penicheMinimum, height);
        penicheMaximum = std::max(penicheMaximum, height);
        penichePrevious = height;
    }
    assert(penicheMinimum > -2.0);
    assert(penicheMaximum < 2.0);
    assert(penicheMaximum - penicheMinimum > 1.5);

    const TideRuntimeModel *penicheModel = TideModelRegistry::find("peniche");
    assert(penicheModel != nullptr);
    assert(penicheModel->station == &TideDevelopmentData::peniche);
    assert(std::strcmp(penicheModel->station->id, "ticon/penichetg-pen-prt-cmems") == 0);
    expectNear(TideModelRegistry::heightAboveHydrographicZero(*penicheModel, 1.0), 3.0, 0.0);
    assert(TideModelRegistry::find("unsupported") == nullptr);
    assert(TideModelRegistry::defaultModel().station == &TideDevelopmentData::nazare);

    const TideHarmonicStation empty = {"empty", "Empty", "test", "test", nullptr, 0, true};
    const TideEngine invalidEngine(empty);
    assert(!invalidEngine.valid());
    assert(std::isnan(invalidEngine.predictHeight(1767225600)));
    assert(std::isnan(invalidEngine.predictHeightWithoutNodalCorrections(1767225600)));
    const TideNodalCorrection invalidCorrection = invalidEngine.nodalCorrection(0, 1767225600);
    expectNear(invalidCorrection.amplitudeFactor, 1.0, 0.0);
    expectNear(invalidCorrection.phaseDegrees, 0.0, 0.0);

    std::cout << "TideEngine sanity tests passed (50 Nazaré + 50 Peniche development constituents, IHO nodal corrections).\n";
    return 0;
}
