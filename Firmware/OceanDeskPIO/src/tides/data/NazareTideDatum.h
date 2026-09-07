#pragma once

namespace TideDatum
{
// Instituto Hidrografico, Tabela de Mares 2026, Volume I, concordance
// for Nazare: mean sea level is 2.00 m above Hydrographic Zero (ZH).
constexpr double nazareMeanLevelAboveHydrographicZeroMeters = 2.00;

constexpr double nazareHeightAboveHydrographicZero(double anomalyMeters)
{
    return anomalyMeters + nazareMeanLevelAboveHydrographicZeroMeters;
}
}
