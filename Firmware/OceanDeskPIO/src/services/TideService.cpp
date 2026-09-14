#include "TideService.h"

#include <time.h>
#include "ConfigService.h"
#include "tides/TideEngine.h"
#include "tides/TidePrediction.h"
#include "tides/data/TideModelRegistry.h"

static unsigned long lastUpdate = 0;
static const unsigned long updateInterval = 15UL * 60UL * 1000UL; // 15 minutos

namespace
{
constexpr time_t minimumSynchronizedTime = 1577836800; // 2020-01-01 UTC
TideSnapshot snapshot;

TideEvent toTideEvent(const TideExtremum &extremum, const TideRuntimeModel &model)
{
    TideEvent event;
    if (!extremum.valid)
    {
        return event;
    }

    event.type = extremum.type == TideExtremumType::Low
                     ? TideEventType::Low
                     : TideEventType::High;
    event.timestamp = static_cast<time_t>(extremum.timestampUtc);
    event.height = static_cast<float>(TideModelRegistry::heightAboveHydrographicZero(
        model, extremum.heightMeters));
    event.valid = true;
    return event;
}

void refreshSnapshot(time_t timestampUtc)
{
    snapshot = TideSnapshot{};
    if (timestampUtc < minimumSynchronizedTime)
    {
        return;
    }

    const TideRuntimeModel *configuredModel =
        TideModelRegistry::find(ConfigService::get().tideStation.c_str());
    const TideRuntimeModel &model = configuredModel != nullptr
                                        ? *configuredModel
                                        : TideModelRegistry::defaultModel();
    const TideEngine engine(*model.station);
    const TidePrediction prediction = calculateTidePrediction(engine, timestampUtc);
    if (!prediction.valid)
    {
        return;
    }

    snapshot.station.id = engine.station().id;
    snapshot.station.name = model.displayName;
    snapshot.station.latitude = model.latitude;
    snapshot.station.longitude = model.longitude;
    snapshot.station.source = engine.station().source;
    snapshot.station.license = engine.station().license;
    snapshot.station.valid = true;

    snapshot.state = prediction.rising ? TideState::Rising : TideState::Falling;
    snapshot.currentHeight = static_cast<float>(TideModelRegistry::heightAboveHydrographicZero(
        model, prediction.currentHeightMeters));
    snapshot.previousEvent = toTideEvent(prediction.previous, model);
    snapshot.nextLow = toTideEvent(prediction.nextLow, model);
    snapshot.nextHigh = toTideEvent(prediction.nextHigh, model);
    snapshot.valid = true;
}
}

void TideService::begin()
{
    lastUpdate = 0;
    refreshSnapshot(time(nullptr));
}

void TideService::update()
{
    unsigned long now = millis();

    const time_t timestampUtc = time(nullptr);
    if (timestampUtc >= minimumSynchronizedTime &&
        (!snapshot.valid || lastUpdate == 0 || now - lastUpdate >= updateInterval))
    {
        lastUpdate = now;
        refreshSnapshot(timestampUtc);
    }
}

TideSnapshot TideService::getSnapshot()
{
    return snapshot;
}

String TideService::formatTime(time_t timestamp)
{
    if (timestamp <= 0)
    {
        return "--:--";
    }

    struct tm timeInfo;

    if (localtime_r(&timestamp, &timeInfo) == nullptr)
    {
        return "--:--";
    }

    char buffer[6];
    snprintf(
        buffer,
        sizeof(buffer),
        "%02d:%02d",
        timeInfo.tm_hour,
        timeInfo.tm_min
    );

    return String(buffer);
}
