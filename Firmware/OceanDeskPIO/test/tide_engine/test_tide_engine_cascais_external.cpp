#include "tides/TideEngine.h"
#include "tides/data/CascaisTideData.h"
#include "tides/data/CascaisTideDatum.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <iomanip>
#include <iostream>

namespace
{
constexpr int64_t halfWindow = 3 * 60 * 60;
constexpr int64_t step = 60;
enum class EventType { High, Low };
struct ReferenceEvent { int64_t timestampUtc; double heightZhMeters; EventType type; };
struct PredictedEvent { double timestampUtc; double heightZhMeters; };
struct Metrics { double mean; double maxAbs; double rmse; };

// Instituto Hidrografico, Tabela de Mares 2026, Volume I, page 2-48
// (PDF page 70). Cascais is a primary port; times are Fuso 0 (TU) and
// heights are relative to Zero Hidrografico (ZH).
constexpr ReferenceEvent referenceEvents[] = {
    {1767226140, 3.10, EventType::High}, {1767248760, 0.90, EventType::Low},
    {1767271140, 3.10, EventType::High}, {1767293220, 0.90, EventType::Low},
    {1767315840, 3.30, EventType::High}, {1767338520, 0.70, EventType::Low},
    {1767360960, 3.20, EventType::High}, {1767382680, 0.80, EventType::Low},
};

PredictedEvent findExtremum(const TideEngine &engine, const ReferenceEvent &reference)
{
    int64_t bestTime = reference.timestampUtc - halfWindow;
    double bestHeight = engine.predictHeight(bestTime);
    for (int64_t time = bestTime + step; time <= reference.timestampUtc + halfWindow; time += step) {
        const double height = engine.predictHeight(time);
        if (reference.type == EventType::High ? height > bestHeight : height < bestHeight) {
            bestTime = time; bestHeight = height;
        }
    }
    const double left = engine.predictHeight(bestTime - step);
    const double right = engine.predictHeight(bestTime + step);
    const double denominator = left - 2.0 * bestHeight + right;
    const double offset = std::abs(denominator) > 1e-12 ? std::clamp(0.5 * (left - right) / denominator, -1.0, 1.0) : 0.0;
    return {bestTime + offset * step, TideDatum::cascaisHeightAboveHydrographicZero(bestHeight - 0.25 * (left - right) * offset)};
}

Metrics metrics(const double *errors, size_t count)
{
    double sum = 0, squares = 0, maxAbs = 0;
    for (size_t index = 0; index < count; ++index) { sum += errors[index]; squares += errors[index] * errors[index]; maxAbs = std::max(maxAbs, std::abs(errors[index])); }
    return {sum / count, maxAbs, std::sqrt(squares / count)};
}
} // namespace

int main()
{
    const TideEngine engine(TideDevelopmentData::cascais);
    assert(engine.valid() && engine.station().developmentOnly);
    constexpr size_t count = sizeof(referenceEvents) / sizeof(referenceEvents[0]);
    double timeErrors[count] = {}, heightErrors[count] = {};
    std::cout << "External validation: OceanDesk Cascais vs IH 2026 Cascais primary-port table\n";
    std::cout << "type,reference_utc,predicted_epoch_s,time_error_min,reference_zh_m,predicted_zh_m,height_error_m\n";
    for (size_t index = 0; index < count; ++index) {
        const PredictedEvent predicted = findExtremum(engine, referenceEvents[index]);
        timeErrors[index] = (predicted.timestampUtc - referenceEvents[index].timestampUtc) / 60.0;
        heightErrors[index] = predicted.heightZhMeters - referenceEvents[index].heightZhMeters;
        std::cout << (referenceEvents[index].type == EventType::High ? "high" : "low") << ',' << referenceEvents[index].timestampUtc << ',' << std::fixed << std::setprecision(1) << predicted.timestampUtc << ',' << std::setprecision(2) << timeErrors[index] << ',' << referenceEvents[index].heightZhMeters << ',' << predicted.heightZhMeters << ',' << heightErrors[index] << '\n';
    }
    const Metrics timing = metrics(timeErrors, count), height = metrics(heightErrors, count);
    std::cout << "timing_minutes: mean=" << timing.mean << ", max_abs=" << timing.maxAbs << ", rmse=" << timing.rmse << '\n';
    std::cout << "height_meters: mean=" << height.mean << ", max_abs=" << height.maxAbs << ", rmse=" << height.rmse << '\n';
    assert(timing.maxAbs <= 15.0);
    assert(height.maxAbs <= 0.15);
    assert(height.rmse <= 0.10);
}
