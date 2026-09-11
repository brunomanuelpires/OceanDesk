#include "locations/BeachCatalog.h"

#include <cassert>
#include <cmath>
#include <cstring>
#include <iostream>

using namespace BeachCatalog;

int main()
{
    assert(beachCount() == 14);
    assert(tideModelCount() == 1);
    assert(std::strcmp(defaultBeach().id, "agua-de-madeiros") == 0);
    assert(std::strcmp(screenName(defaultBeach()), "Água de Madeiros") == 0);

    for (size_t index = 0; index < beachCount(); ++index)
    {
        const BeachLocation &beach = beachAt(index);
        assert(beach.id != nullptr && beach.id[0] != '\0');
        assert(beach.displayName != nullptr && beach.displayName[0] != '\0');
        assert(std::strcmp(beach.country, "Portugal") == 0);
        assert(beach.area == GeographicArea::Mainland);
        assert(std::isfinite(beach.latitude));
        assert(std::isfinite(beach.longitude));
        const TideModel *model = findTideModel(beach.tideModelId);
        assert(model != nullptr);
        assert(model->firmwareSupported);
        assert(std::strcmp(beach.timezone, model->timezone) == 0);
        assert(findBeach(beach.id) == &beach);
    }

    assert(findBeach("not-in-catalog") == nullptr);
    assert(findBeachByLegacyName("Água de Madeiros") == &defaultBeach());
    assert(findBeachByLegacyName("Praia da Água de Madeiros") == &defaultBeach());

    const StoredBeach current = resolveStoredBeach("nazare", "ignored", "bad-model");
    assert(current.resolution == StoredBeachResolution::CatalogId);
    assert(std::strcmp(current.beach->id, "nazare") == 0);

    const StoredBeach migrated = resolveStoredBeach("", "Água de Madeiros", "nazare");
    assert(migrated.resolution == StoredBeachResolution::LegacyName);
    assert(migrated.beach == &defaultBeach());

    const StoredBeach custom = resolveStoredBeach("", "A minha praia", "nazare");
    assert(custom.resolution == StoredBeachResolution::LegacyCustom);
    assert(custom.beach == nullptr);

    const StoredBeach recovered = resolveStoredBeach("missing", "Nazaré", "nazare");
    assert(recovered.resolution == StoredBeachResolution::LegacyName);
    assert(std::strcmp(recovered.beach->id, "nazare") == 0);

    const StoredBeach fallback = resolveStoredBeach("missing", "Desconhecida", "unsupported");
    assert(fallback.resolution == StoredBeachResolution::DefaultFallback);
    assert(fallback.beach == &defaultBeach());

    const StoredBeach corruptLegacy = resolveStoredBeach("", "Desconhecida", "unsupported");
    assert(corruptLegacy.resolution == StoredBeachResolution::DefaultFallback);
    assert(corruptLegacy.beach == &defaultBeach());

    assert(std::strcmp(geographicAreaName(GeographicArea::Azores), "Açores") == 0);
    assert(std::strcmp(geographicAreaName(GeographicArea::Madeira), "Madeira") == 0);
    std::cout << "BeachCatalog lookup, mapping, migration and fallback tests passed.\n";
    return 0;
}
