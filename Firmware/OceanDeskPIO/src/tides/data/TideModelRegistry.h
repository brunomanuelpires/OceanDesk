#pragma once

#include "tides/TideEngine.h"

struct TideRuntimeModel
{
    const char *id;
    const TideHarmonicStation *station;
    const char *displayName;
    double latitude;
    double longitude;
    double meanLevelAboveHydrographicZeroMeters;
};

namespace TideModelRegistry
{
const TideRuntimeModel &defaultModel();
const TideRuntimeModel *find(const char *id);

constexpr double heightAboveHydrographicZero(const TideRuntimeModel &model,
                                              double anomalyMeters)
{
    return anomalyMeters + model.meanLevelAboveHydrographicZeroMeters;
}
}
