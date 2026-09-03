#include "TideEngine.h"

#include <cmath>
#include <limits>

namespace
{
constexpr double degreesToRadians = 3.14159265358979323846 / 180.0;
constexpr double secondsPerDay = 86400.0;
constexpr double unixEpochJulianDate = 2440587.5;
constexpr double j2000JulianDate = 2451545.0;
constexpr double daysPerJulianCentury = 36525.0;

double wrapDegrees(double value)
{
    value = std::fmod(value, 360.0);
    return value < 0.0 ? value + 360.0 : value;
}

double polynomial(const double *coefficients, size_t count, double argument)
{
    double result = 0.0;
    for (size_t index = count; index > 0; --index)
    {
        result = result * argument + coefficients[index - 1];
    }
    return result;
}

struct AstronomicalArguments
{
    double tau;
    double lunarLongitude;
    double solarLongitude;
    double lunarPerigee;
    double lunarNodePrime;
    double solarPerigee;
};

AstronomicalArguments astronomicalArguments(int64_t timestampUtc)
{
    constexpr double lunarLongitude[] = {
        218.3164591,
        481267.88134236,
        -0.0013268,
        1.0 / 538841.0 - 1.0 / 65194000.0,
    };
    constexpr double solarLongitude[] = {280.46645, 36000.76983, 0.0003032};
    constexpr double lunarPerigee[] = {
        83.353243,
        4069.0137111,
        -0.0103238,
        -1.0 / 80053.0,
        1.0 / 18999000.0,
    };
    constexpr double lunarNode[] = {
        125.044555,
        -1934.1361849,
        0.0020762,
        1.0 / 467410.0,
        -1.0 / 60616000.0,
    };
    constexpr double solarPerigee[] = {
        280.46645 - 357.5291,
        36000.76932 - 35999.0503,
        0.0003032 + 0.0001559,
        0.00000048,
    };

    const double julianDate = static_cast<double>(timestampUtc) / secondsPerDay + unixEpochJulianDate;
    const double julianCenturies = (julianDate - j2000JulianDate) / daysPerJulianCentury;
    const double s = wrapDegrees(polynomial(lunarLongitude, 4, julianCenturies));
    const double h = wrapDegrees(polynomial(solarLongitude, 3, julianCenturies));
    const double p = wrapDegrees(polynomial(lunarPerigee, 5, julianCenturies));
    const double node = wrapDegrees(polynomial(lunarNode, 5, julianCenturies));
    const double pp = wrapDegrees(polynomial(solarPerigee, 4, julianCenturies));
    const double hourAngle = (julianDate - std::floor(julianDate)) * 360.0;

    return {
        wrapDegrees(hourAngle + h - s),
        s,
        h,
        p,
        -node,
        pp,
    };
}

double equilibriumArgument(const TideConstituent &constituent, const AstronomicalArguments &astro)
{
    const double arguments[7] = {
        astro.tau,
        astro.lunarLongitude,
        astro.solarLongitude,
        astro.lunarPerigee,
        astro.lunarNodePrime,
        astro.solarPerigee,
        90.0,
    };

    double result = 0.0;
    for (size_t index = 0; index < 7; ++index)
    {
        result += static_cast<double>(constituent.doodson[index]) * arguments[index];
    }
    return result;
}
} // namespace

TideEngine::TideEngine(const TideHarmonicStation &station)
    : station_(station)
{
}

double TideEngine::predictHeight(int64_t timestampUtc) const
{
    if (!valid())
    {
        return std::numeric_limits<double>::quiet_NaN();
    }

    const AstronomicalArguments astro = astronomicalArguments(timestampUtc);
    double heightMeters = 0.0;

    for (size_t index = 0; index < station_.constituentCount; ++index)
    {
        const TideConstituent &constituent = station_.constituents[index];
        const double angleDegrees =
            equilibriumArgument(constituent, astro) - constituent.phaseDegreesUtc;
        heightMeters += constituent.amplitudeMeters * std::cos(angleDegrees * degreesToRadians);
    }

    return heightMeters;
}

bool TideEngine::valid() const
{
    return station_.constituents != nullptr && station_.constituentCount > 0;
}

const TideHarmonicStation &TideEngine::station() const
{
    return station_;
}
