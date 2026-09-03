#include "NazareTideData.h"

namespace
{
// Development-only subset copied from tools/tides/generated/portugal-stations.json.
// Doodson coefficients are decoded from the IHO XDO definitions used by Neaps.
constexpr TideConstituent constituents[] = {
    // amplitude (m), phase UTC (degrees), tau/s/h/p/N'/p'/90 coefficients
    {0.98384380f, 70.351053f, {2, 0, 0, 0, 0, 0, 0}},  // M2
    {0.21418306f, 52.312497f, {2, -1, 0, 1, 0, 0, 0}}, // N2
    {0.34218614f, 98.583018f, {2, 2, -2, 0, 0, 0, 0}}, // S2
    {0.09034627f, 92.522527f, {2, 2, 0, 0, 0, 0, 0}},  // K2
    {0.06771514f, 60.363795f, {1, 1, 0, 0, 0, 0, -1}}, // K1
    {0.02130731f, 48.719153f, {1, 1, -2, 0, 0, 0, 1}}, // P1
    {0.06058316f, 318.159164f, {1, -1, 0, 0, 0, 0, 1}}, // O1
    {0.01904336f, 269.046716f, {1, -2, 0, 1, 0, 0, 1}}, // Q1
};
} // namespace

namespace TideDevelopmentData
{
const TideHarmonicStation nazare = {
    "ticon/nazaretg-naz-prt-cmems",
    "Nazare",
    "TICON-4 via Neaps Tide Database",
    "CC BY-NC 4.0",
    constituents,
    sizeof(constituents) / sizeof(constituents[0]),
    true,
};
}
