#include "ConfigService.h"

#include <Preferences.h>
#include "locations/BeachCatalog.h"

namespace
{
constexpr const char *preferencesNamespace = "oceandesk";
constexpr const char *configuredKey = "configured";
constexpr const char *wifiSsidKey = "wifi_ssid";
constexpr const char *wifiPasswordKey = "wifi_pass";
constexpr const char *beachIdKey = "beach_id";
constexpr const char *beachNameKey = "beach";
constexpr const char *tideStationKey = "station";
constexpr const char *timezoneKey = "timezone";
constexpr const char *textColorKey = "text_color";
constexpr const char *backgroundColorKey = "bg_color";
constexpr const char *displayStyleKey = "display";
constexpr DisplayStyle defaultDisplayStyle = DisplayStyle::Classic;

Preferences preferences;
DeviceConfig config;
bool configured = false;
bool initialized = false;

bool isLegacyTimezone(const String &timezone)
{
    return timezone == "WET0WEST,M3.5.0/1,M10.5.0" ||
           timezone == "AZOT1AZOST0,M3.5.0/0,M10.5.0/1" ||
           timezone == "CET-1CEST,M3.5.0/2,M10.5.0/3" ||
           timezone == "UTC0";
}

void applyCatalogBeach(DeviceConfig &target, const BeachCatalog::BeachLocation &beach)
{
    target.beachId = beach.id;
    target.beachName = BeachCatalog::screenName(beach);
    target.tideStation = beach.tideModelId;
    target.timezone = beach.timezone;
}
}

void ConfigService::begin()
{
    if (initialized)
    {
        return;
    }

    if (!preferences.begin(preferencesNamespace, false))
    {
        Serial.println("Config: failed to open persistent storage");
        return;
    }

    config.wifiSsid = preferences.getString(wifiSsidKey, "");
    config.wifiPassword = preferences.getString(wifiPasswordKey, "");
    const BeachCatalog::BeachLocation &defaultBeach = BeachCatalog::defaultBeach();
    config.beachId = preferences.getString(beachIdKey, "");
    config.beachName = preferences.getString(beachNameKey, BeachCatalog::screenName(defaultBeach));
    config.tideStation = preferences.getString(tideStationKey, defaultBeach.tideModelId);
    config.timezone = preferences.getString(timezoneKey, defaultBeach.timezone);
    config.textColor = preferences.getULong(textColorKey, 0xFFFFFF);
    config.backgroundColor = preferences.getULong(backgroundColorKey, 0x0B1620);
    DisplayStyle storedDisplayStyle;
    if (!ConfigService::parseDisplayStyle(
            preferences.getString(displayStyleKey, ConfigService::displayStyleName(defaultDisplayStyle)),
            storedDisplayStyle))
    {
        storedDisplayStyle = defaultDisplayStyle;
    }
    config.displayStyle = storedDisplayStyle;
    const BeachCatalog::StoredBeach storedBeach = BeachCatalog::resolveStoredBeach(
        config.beachId.c_str(), config.beachName.c_str(), config.tideStation.c_str());
    if (storedBeach.beach != nullptr)
    {
        applyCatalogBeach(config, *storedBeach.beach);
    }
    else
    {
        // An existing custom beach/station pair remains usable until the user
        // explicitly selects a catalog location in the settings page.
        config.beachId = "";
        const BeachCatalog::TideModel *legacyModel =
            BeachCatalog::findTideModel(config.tideStation.c_str());
        if (!isLegacyTimezone(config.timezone) && legacyModel != nullptr)
        {
            config.timezone = legacyModel->timezone;
        }
    }
    configured = preferences.getBool(configuredKey, false) &&
                 !config.wifiSsid.isEmpty();
    initialized = true;

    Serial.println(configured ? "Config: valid configuration found"
                              : "Config: device is not configured");
}

bool ConfigService::isConfigured()
{
    return initialized && configured && !config.wifiSsid.isEmpty();
}

const DeviceConfig &ConfigService::get()
{
    return config;
}

bool ConfigService::save(const DeviceConfig &newConfig)
{
    const BeachCatalog::BeachLocation *catalogBeach =
        BeachCatalog::findBeach(newConfig.beachId.c_str());
    const BeachCatalog::TideModel *tideModel =
        BeachCatalog::findTideModel(newConfig.tideStation.c_str());
    const bool validCatalogBeach = catalogBeach != nullptr && tideModel != nullptr &&
                                    tideModel->firmwareSupported &&
                                    newConfig.beachName == BeachCatalog::screenName(*catalogBeach) &&
                                    newConfig.tideStation == catalogBeach->tideModelId &&
                                    newConfig.timezone == catalogBeach->timezone;
    const bool validLegacyBeach = newConfig.beachId.isEmpty() &&
                                   tideModel != nullptr && tideModel->firmwareSupported &&
                                   isLegacyTimezone(newConfig.timezone);
    const char *displayStyle = displayStyleName(newConfig.displayStyle);
    if (!initialized || newConfig.wifiSsid.isEmpty() || newConfig.beachName.isEmpty() ||
        (!validCatalogBeach && !validLegacyBeach) || displayStyle == nullptr)
    {
        return false;
    }

    // Mark the record valid only after both values have been written.
    preferences.putBool(configuredKey, false);
    const bool ssidSaved = preferences.putString(wifiSsidKey, newConfig.wifiSsid) == newConfig.wifiSsid.length();
    const bool passwordSaved =
        preferences.putString(wifiPasswordKey, newConfig.wifiPassword) == newConfig.wifiPassword.length();
    const bool settingsSaved =
        preferences.putString(beachIdKey, newConfig.beachId) == newConfig.beachId.length() &&
        preferences.putString(beachNameKey, newConfig.beachName) == newConfig.beachName.length() &&
        preferences.putString(tideStationKey, newConfig.tideStation) == newConfig.tideStation.length() &&
        preferences.putString(timezoneKey, newConfig.timezone) == newConfig.timezone.length() &&
        preferences.putULong(textColorKey, newConfig.textColor) == sizeof(uint32_t) &&
        preferences.putULong(backgroundColorKey, newConfig.backgroundColor) == sizeof(uint32_t) &&
        preferences.putString(displayStyleKey, displayStyle) == strlen(displayStyle);

    if (!ssidSaved || !passwordSaved || !settingsSaved || !preferences.putBool(configuredKey, true))
    {
        configured = false;
        return false;
    }

    config = newConfig;
    configured = true;
    return true;
}

bool ConfigService::reset()
{
    if (!initialized || !preferences.clear())
    {
        return false;
    }

    config = DeviceConfig{};
    configured = false;
    return true;
}

bool ConfigService::selectBeach(DeviceConfig &target, const String &beachId)
{
    const BeachCatalog::BeachLocation *beach = BeachCatalog::findBeach(beachId.c_str());
    if (beach == nullptr)
    {
        return false;
    }

    const BeachCatalog::TideModel *model = BeachCatalog::findTideModel(beach->tideModelId);
    if (model == nullptr || !model->firmwareSupported)
    {
        return false;
    }

    applyCatalogBeach(target, *beach);
    return true;
}

const char *ConfigService::displayStyleName(DisplayStyle style)
{
    switch (style)
    {
    case DisplayStyle::Classic: return "classic";
    case DisplayStyle::Minimal: return "minimal";
    case DisplayStyle::Graph: return "graph";
    case DisplayStyle::Info: return "info";
    }

    return nullptr;
}

bool ConfigService::parseDisplayStyle(const String &value, DisplayStyle &style)
{
    if (value == "classic") style = DisplayStyle::Classic;
    else if (value == "minimal") style = DisplayStyle::Minimal;
    else if (value == "graph") style = DisplayStyle::Graph;
    else if (value == "info") style = DisplayStyle::Info;
    else return false;
    return true;
}
