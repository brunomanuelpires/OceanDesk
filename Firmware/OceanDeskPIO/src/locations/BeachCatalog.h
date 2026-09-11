#pragma once

#include <stddef.h>

namespace BeachCatalog
{
enum class GeographicArea
{
    Mainland,
    Azores,
    Madeira
};

struct TideModel
{
    const char *id;
    const char *stationId;
    const char *displayName;
    GeographicArea area;
    double latitude;
    double longitude;
    const char *timezone;
    bool firmwareSupported;
};

struct BeachLocation
{
    const char *id;
    const char *displayName;
    const char *shortName;
    const char *region;
    const char *country;
    GeographicArea area;
    double latitude;
    double longitude;
    const char *tideModelId;
    const char *timezone;
};

enum class StoredBeachResolution
{
    CatalogId,
    LegacyName,
    LegacyCustom,
    DefaultFallback
};

struct StoredBeach
{
    const BeachLocation *beach;
    StoredBeachResolution resolution;
};

size_t beachCount();
const BeachLocation &beachAt(size_t index);
const BeachLocation *findBeach(const char *id);
const BeachLocation *findBeachByLegacyName(const char *name);
const BeachLocation &defaultBeach();
const char *screenName(const BeachLocation &beach);

size_t tideModelCount();
const TideModel &tideModelAt(size_t index);
const TideModel *findTideModel(const char *id);

StoredBeach resolveStoredBeach(const char *beachId,
                               const char *legacyBeachName,
                               const char *legacyTideModelId);
const char *geographicAreaName(GeographicArea area);
}
