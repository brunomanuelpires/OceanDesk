#pragma once

#include <cstddef>
#include <cstdint>

struct TideConstituent
{
    float amplitudeMeters;
    float phaseDegreesUtc;
    int8_t doodson[7];
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

    bool valid() const;
    const TideHarmonicStation &station() const;

private:
    const TideHarmonicStation &station_;
};
