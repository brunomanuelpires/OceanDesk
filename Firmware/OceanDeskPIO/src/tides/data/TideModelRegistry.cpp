#include "TideModelRegistry.h"

#include <cstring>
#include "NazareTideData.h"
#include "NazareTideDatum.h"
#include "PenicheTideData.h"
#include "PenicheTideDatum.h"

namespace TideModelRegistry
{
namespace
{
constexpr TideRuntimeModel models[] = {
    {
        "nazare",
        &TideDevelopmentData::nazare,
        "Nazaré",
        39.585999,
        -9.074,
        TideDatum::nazareMeanLevelAboveHydrographicZeroMeters,
    },
    {
        "peniche",
        &TideDevelopmentData::peniche,
        "Peniche",
        39.354,
        -9.367,
        TideDatum::penicheMeanLevelAboveHydrographicZeroMeters,
    },
};
}

const TideRuntimeModel &defaultModel()
{
    return models[0];
}

const TideRuntimeModel *find(const char *id)
{
    if (id == nullptr || id[0] == '\0')
    {
        return nullptr;
    }

    for (const TideRuntimeModel &model : models)
    {
        if (std::strcmp(model.id, id) == 0)
        {
            return &model;
        }
    }
    return nullptr;
}
}
