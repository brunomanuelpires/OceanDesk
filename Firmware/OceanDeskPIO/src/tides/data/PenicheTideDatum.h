#pragma once

namespace TideDatum
{
// Instituto Hidrografico, Tabela de Mares 2026, Volume I, primary-port
// elements for Peniche: mean sea level is 2.00 m above Hydrographic Zero (ZH).
constexpr double penicheMeanLevelAboveHydrographicZeroMeters = 2.00;

constexpr double penicheHeightAboveHydrographicZero(double anomalyMeters)
{
    return anomalyMeters + penicheMeanLevelAboveHydrographicZeroMeters;
}
}
