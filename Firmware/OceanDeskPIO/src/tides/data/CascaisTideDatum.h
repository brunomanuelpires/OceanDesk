#pragma once

namespace TideDatum
{
// Instituto Hidrografico, Tabela de Mares 2026, Volume I, Cascais notes.
constexpr double cascaisMeanLevelAboveHydrographicZeroMeters = 2.08;

constexpr double cascaisHeightAboveHydrographicZero(double anomalyMeters)
{
    return anomalyMeters + cascaisMeanLevelAboveHydrographicZeroMeters;
}
} // namespace TideDatum
