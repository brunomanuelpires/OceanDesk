#pragma once

#include <cstddef>
#include <cstdint>

enum class TideNodalFundamental : uint8_t
{
    None,
    Mm,
    Mf,
    O1,
    K1,
    J1,
    M1,
    M2,
    K2,
    M3,
    L2,
};

struct TideNodalTerm
{
    TideNodalFundamental fundamental;
    float factor;
};

struct TideNodalCorrection
{
    double amplitudeFactor;
    double phaseDegrees;
};

constexpr size_t tideNodalTermCapacity = 2;

struct TideConstituent
{
    float amplitudeMeters;
    float phaseDegreesUtc;
    int8_t doodson[7];
    TideNodalTerm nodalTerms[tideNodalTermCapacity];
    uint8_t nodalTermCount;
};

struct TideHarmonicStation
{
    const char *id;
    const char *name;
    const char *source;
    const char *license;
    const TideConstituent *constituents;
    size_t constituentCount;
    bool developmentOnly;
};

class TideEngine
{
public:
    explicit TideEngine(const TideHarmonicStation &station);

    // Returns the astronomical water-level anomaly in metres relative to MSL.
    // The timestamp is Unix time in UTC. Returns NaN for an invalid station.
    double predictHeight(int64_t timestampUtc) const;

    // Baseline calculation retained for regression comparisons. It uses the
    // same harmonic data but applies identity nodal corrections (f=1, u=0).
    double predictHeightWithoutNodalCorrections(int64_t timestampUtc) const;

    // Returns the IHO/Neaps nodal amplitude (f) and phase (u) correction for
    // one constituent. An out-of-range index returns the identity correction.
    TideNodalCorrection nodalCorrection(size_t constituentIndex, int64_t timestampUtc) const;

    bool valid() const;
    const TideHarmonicStation &station() const;

private:
    double predictHeightInternal(int64_t timestampUtc, bool applyNodalCorrections) const;

    const TideHarmonicStation &station_;
};
