#include "TideEngine.h"

#include <algorithm>
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
    double lunarNode;
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
        node,
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

TideNodalCorrection fromSinCos(double fSinU, double fCosU)
{
    return {
        std::sqrt(fSinU * fSinU + fCosU * fCosU),
        std::atan2(fSinU, fCosU) / degreesToRadians,
    };
}

TideNodalCorrection fundamentalCorrection(TideNodalFundamental fundamental,
                                          const AstronomicalArguments &astro)
{
    const double node = astro.lunarNode * degreesToRadians;
    const double perigee = astro.lunarPerigee * degreesToRadians;

    switch (fundamental)
    {
    case TideNodalFundamental::Mm:
        return {
            1.0 - 0.1311 * std::cos(node) + 0.0538 * std::cos(2.0 * perigee) +
                0.0205 * std::cos(2.0 * perigee - node),
            0.0,
        };
    case TideNodalFundamental::Mf:
        return {
            1.084 + 0.415 * std::cos(node) + 0.039 * std::cos(2.0 * node),
            -23.7 * std::sin(node) + 2.7 * std::sin(2.0 * node) - 0.4 * std::sin(3.0 * node),
        };
    case TideNodalFundamental::O1:
        return {
            1.0176 + 0.1871 * std::cos(node) - 0.0147 * std::cos(2.0 * node),
            10.8 * std::sin(node) - 1.34 * std::sin(2.0 * node) + 0.19 * std::sin(3.0 * node),
        };
    case TideNodalFundamental::K1:
        return {
            1.006 + 0.115 * std::cos(node) - 0.0088 * std::cos(2.0 * node) +
                0.0006 * std::cos(3.0 * node),
            -8.86 * std::sin(node) + 0.68 * std::sin(2.0 * node) - 0.07 * std::sin(3.0 * node),
        };
    case TideNodalFundamental::J1:
        return {
            1.1029 + 0.1676 * std::cos(node) - 0.017 * std::cos(2.0 * node) +
                0.0016 * std::cos(3.0 * node),
            -12.94 * std::sin(node) + 1.34 * std::sin(2.0 * node) - 0.19 * std::sin(3.0 * node),
        };
    case TideNodalFundamental::M1:
        return fromSinCos(
            std::sin(perigee) + 0.2 * std::sin(perigee - node),
            2.0 * (std::cos(perigee) + 0.2 * std::cos(perigee - node)));
    case TideNodalFundamental::M2:
        return {
            1.0007 - 0.0373 * std::cos(node) + 0.0002 * std::cos(2.0 * node),
            -2.14 * std::sin(node),
        };
    case TideNodalFundamental::K2:
        return {
            1.0246 + 0.2863 * std::cos(node) + 0.0083 * std::cos(2.0 * node) -
                0.0015 * std::cos(3.0 * node),
            -17.74 * std::sin(node) + 0.68 * std::sin(2.0 * node) - 0.04 * std::sin(3.0 * node),
        };
    case TideNodalFundamental::M3:
    {
        const TideNodalCorrection m2 =
            fundamentalCorrection(TideNodalFundamental::M2, astro);
        return {
            std::pow(std::sqrt(m2.amplitudeFactor), 3.0),
            -3.21 * std::sin(node),
        };
    }
    case TideNodalFundamental::L2:
        return fromSinCos(
            -0.2505 * std::sin(2.0 * perigee) -
                0.1102 * std::sin(2.0 * perigee - node) -
                0.0156 * std::sin(2.0 * perigee - 2.0 * node) - 0.037 * std::sin(node),
            1.0 - 0.2505 * std::cos(2.0 * perigee) -
                0.1102 * std::cos(2.0 * perigee - node) -
                0.0156 * std::cos(2.0 * perigee - 2.0 * node) - 0.037 * std::cos(node));
    case TideNodalFundamental::None:
        return {1.0, 0.0};
    }

    return {1.0, 0.0};
}

TideNodalCorrection composedCorrection(const TideConstituent &constituent,
                                        const AstronomicalArguments &astro)
{
    TideNodalCorrection result = {1.0, 0.0};

    const size_t termCount = std::min(static_cast<size_t>(constituent.nodalTermCount),
                                      tideNodalTermCapacity);
    for (size_t index = 0; index < termCount; ++index)
    {
        const TideNodalTerm &term = constituent.nodalTerms[index];
        const TideNodalCorrection correction = fundamentalCorrection(term.fundamental, astro);
        result.amplitudeFactor *= std::pow(correction.amplitudeFactor, std::abs(term.factor));
        result.phaseDegrees += static_cast<double>(term.factor) * correction.phaseDegrees;
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
    return predictHeightInternal(timestampUtc, true);
}

double TideEngine::predictHeightWithoutNodalCorrections(int64_t timestampUtc) const
{
    return predictHeightInternal(timestampUtc, false);
}

TideNodalCorrection TideEngine::nodalCorrection(size_t constituentIndex, int64_t timestampUtc) const
{
    if (!valid() || constituentIndex >= station_.constituentCount)
    {
        return {1.0, 0.0};
    }

    const TideConstituent &constituent = station_.constituents[constituentIndex];
    const AstronomicalArguments astro = astronomicalArguments(timestampUtc);
    return composedCorrection(constituent, astro);
}

double TideEngine::predictHeightInternal(int64_t timestampUtc, bool applyNodalCorrections) const
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
        const TideNodalCorrection correction =
            applyNodalCorrections ? composedCorrection(constituent, astro) : TideNodalCorrection{1.0, 0.0};
        const double angleDegrees =
            equilibriumArgument(constituent, astro) + correction.phaseDegrees - constituent.phaseDegreesUtc;
        heightMeters += constituent.amplitudeMeters * correction.amplitudeFactor *
                        std::cos(angleDegrees * degreesToRadians);
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
