#include "BeachCatalog.h"

#include <string.h>

namespace BeachCatalog
{
namespace
{
#include "PortugalBeachCatalogData.inc"

bool hasText(const char *value)
{
    return value != nullptr && value[0] != '\0';
}

bool sameText(const char *left, const char *right)
{
    return left != nullptr && right != nullptr && strcmp(left, right) == 0;
}
}

size_t beachCount()
{
    return sizeof(beaches) / sizeof(beaches[0]);
}

const BeachLocation &beachAt(size_t index)
{
    return beaches[index < beachCount() ? index : 0];
}

const BeachLocation *findBeach(const char *id)
{
    if (!hasText(id)) return nullptr;
    for (const BeachLocation &beach : beaches)
    {
        if (sameText(beach.id, id)) return &beach;
    }
    return nullptr;
}

const BeachLocation *findBeachByLegacyName(const char *name)
{
    if (!hasText(name)) return nullptr;
    for (const BeachLocation &beach : beaches)
    {
        if (sameText(beach.displayName, name) || sameText(beach.shortName, name)) return &beach;
    }
    return nullptr;
}

const BeachLocation &defaultBeach()
{
    const BeachLocation *beach = findBeach("agua-de-madeiros");
    return beach != nullptr ? *beach : beaches[0];
}

const char *screenName(const BeachLocation &beach)
{
    return hasText(beach.shortName) ? beach.shortName : beach.displayName;
}

size_t tideModelCount()
{
    return sizeof(tideModels) / sizeof(tideModels[0]);
}

const TideModel &tideModelAt(size_t index)
{
    return tideModels[index < tideModelCount() ? index : 0];
}

const TideModel *findTideModel(const char *id)
{
    if (!hasText(id)) return nullptr;
    for (const TideModel &model : tideModels)
    {
        if (sameText(model.id, id)) return &model;
    }
    return nullptr;
}

StoredBeach resolveStoredBeach(const char *beachId,
                               const char *legacyBeachName,
                               const char *legacyTideModelId)
{
    if (hasText(beachId))
    {
        if (const BeachLocation *beach = findBeach(beachId))
        {
            return {beach, StoredBeachResolution::CatalogId};
        }

        if (const BeachLocation *beach = findBeachByLegacyName(legacyBeachName))
        {
            const bool modelMatches = !hasText(legacyTideModelId) ||
                                      sameText(beach->tideModelId, legacyTideModelId);
            if (modelMatches) return {beach, StoredBeachResolution::LegacyName};
        }

        return {&defaultBeach(), StoredBeachResolution::DefaultFallback};
    }

    if (const BeachLocation *beach = findBeachByLegacyName(legacyBeachName))
    {
        const bool modelMatches = !hasText(legacyTideModelId) ||
                                  sameText(beach->tideModelId, legacyTideModelId);
        if (modelMatches) return {beach, StoredBeachResolution::LegacyName};
    }

    const TideModel *legacyModel = findTideModel(legacyTideModelId);
    if (hasText(legacyBeachName) && legacyModel != nullptr && legacyModel->firmwareSupported)
    {
        return {nullptr, StoredBeachResolution::LegacyCustom};
    }

    return {&defaultBeach(), StoredBeachResolution::DefaultFallback};
}

const char *geographicAreaName(GeographicArea area)
{
    switch (area)
    {
    case GeographicArea::Mainland: return "Portugal continental";
    case GeographicArea::Azores: return "Açores";
    case GeographicArea::Madeira: return "Madeira";
    }
    return "Portugal";
}
}
