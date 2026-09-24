#include "ConfigService.h"

#include <Preferences.h>
#include "locations/BeachCatalog.h"

namespace
{
constexpr const char *preferencesNamespace = "oceandesk";
constexpr const char *configuredKey = "configured";
constexpr const char *wifiSsidKey = "wifi_ssid";
constexpr const char *wifiPasswordKey = "wifi_pass";
constexpr const char *wifiSsid2Key = "wifi_ssid2";
constexpr const char *wifiPassword2Key = "wifi_pass2";
constexpr const char *wifiSsid3Key = "wifi_ssid3";
constexpr const char *wifiPassword3Key = "wifi_pass3";
constexpr const char *beachIdKey = "beach_id";
constexpr const char *beachNameKey = "beach";
constexpr const char *tideStationKey = "station";
constexpr const char *timezoneKey = "timezone";
constexpr const char *textColorKey = "text_color";
constexpr const char *backgroundColorKey = "bg_color";
constexpr const char *displayStyleKey = "display";
constexpr const char *alarmEnabledKey = "alarm_enabled";
constexpr const char *alarmHourKey = "alarm_hour";
constexpr const char *alarmMinuteKey = "alarm_minute";
constexpr const char *lastAlarmDayKey = "alarm_last_day";
constexpr const char *lastAlarmMinuteKey = "alarm_last_min";
constexpr const char *brightnessKey = "brightness";
constexpr const char *nightModeKey = "night_mode";
// NVS keys are limited to 15 characters on ESP32.  The former key
// "night_brightness" had 16 and made every full configuration save fail.
constexpr const char *nightBrightnessKey = "night_bright";
constexpr const char *nightStartKey = "night_start";
constexpr const char *nightEndKey = "night_end";
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
    config.wifiSsid2 = preferences.getString(wifiSsid2Key, "");
    config.wifiPassword2 = preferences.getString(wifiPassword2Key, "");
    config.wifiSsid3 = preferences.getString(wifiSsid3Key, "");
    config.wifiPassword3 = preferences.getString(wifiPassword3Key, "");
    const BeachCatalog::BeachLocation &defaultBeach = BeachCatalog::defaultBeach();
    config.beachId = preferences.getString(beachIdKey, "");
    config.beachName = preferences.getString(beachNameKey, BeachCatalog::screenName(defaultBeach));
    config.tideStation = preferences.getString(tideStationKey, defaultBeach.tideModelId);
    config.timezone = preferences.getString(timezoneKey, defaultBeach.timezone);
    config.textColor = preferences.getULong(textColorKey, 0xFFFFFF);
    config.backgroundColor = preferences.getULong(backgroundColorKey, 0x0B1620);
    config.alarmEnabled = preferences.getBool(alarmEnabledKey, false);
    config.alarmHour = preferences.getUChar(alarmHourKey, 7);
    config.alarmMinute = preferences.getUChar(alarmMinuteKey, 0);
    config.brightness = preferences.getUChar(brightnessKey, 100);
    config.nightModeEnabled = preferences.getBool(nightModeKey, true);
    config.nightBrightness = preferences.getUChar(nightBrightnessKey, 35);
    config.nightStartMinute = preferences.getUShort(nightStartKey, 22 * 60 + 30);
    config.nightEndMinute = preferences.getUShort(nightEndKey, 7 * 60);
    if (config.brightness < 25 || config.brightness > 100) config.brightness = 100;
    if (config.nightBrightness < 5 || config.nightBrightness > 100) config.nightBrightness = 35;
    if (config.nightStartMinute >= 24 * 60 || config.nightEndMinute >= 24 * 60)
    { config.nightStartMinute = 22 * 60 + 30; config.nightEndMinute = 7 * 60; }
    if (config.alarmHour > 23 || config.alarmMinute > 59)
    {
        config.alarmEnabled = false;
        config.alarmHour = 7;
        config.alarmMinute = 0;
    }
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
    const bool validAlarm = newConfig.alarmHour <= 23 && newConfig.alarmMinute <= 59;
    const bool validBrightness = newConfig.brightness >= 25 && newConfig.brightness <= 100 &&
                                 newConfig.nightBrightness >= 5 && newConfig.nightBrightness <= 100 &&
                                 newConfig.nightStartMinute < 24 * 60 && newConfig.nightEndMinute < 24 * 60;
    const bool validAdditionalNetworks =
        (newConfig.wifiSsid2.isEmpty() || newConfig.wifiSsid2 != newConfig.wifiSsid) &&
        (newConfig.wifiSsid3.isEmpty() ||
         (newConfig.wifiSsid3 != newConfig.wifiSsid && newConfig.wifiSsid3 != newConfig.wifiSsid2));
    if (!initialized || newConfig.wifiSsid.isEmpty() || newConfig.beachName.isEmpty() ||
        (!validCatalogBeach && !validLegacyBeach) || displayStyle == nullptr || !validAlarm ||
        !validBrightness || !validAdditionalNetworks)
    {
        Serial.printf("Config: validation failed (wifi=%d beach=%d catalog=%d legacy=%d display=%d alarm=%d brightness=%d)\n",
                      !newConfig.wifiSsid.isEmpty(), !newConfig.beachName.isEmpty(), validCatalogBeach,
                      validLegacyBeach, displayStyle != nullptr, validAlarm, validBrightness);
        return false;
    }

    // Keep the last known-good marker until every new value is written. A bad
    // browser submission must never force an otherwise working device back to setup.
    const bool wasConfigured = configured;
    const bool ssidSaved = preferences.putString(wifiSsidKey, newConfig.wifiSsid) == newConfig.wifiSsid.length();
    const bool passwordSaved =
        preferences.putString(wifiPasswordKey, newConfig.wifiPassword) == newConfig.wifiPassword.length();
    const bool additionalNetworksSaved =
        preferences.putString(wifiSsid2Key, newConfig.wifiSsid2) == newConfig.wifiSsid2.length() &&
        preferences.putString(wifiPassword2Key, newConfig.wifiPassword2) == newConfig.wifiPassword2.length() &&
        preferences.putString(wifiSsid3Key, newConfig.wifiSsid3) == newConfig.wifiSsid3.length() &&
        preferences.putString(wifiPassword3Key, newConfig.wifiPassword3) == newConfig.wifiPassword3.length();
    const bool settingsSaved =
        preferences.putString(beachIdKey, newConfig.beachId) == newConfig.beachId.length() &&
        preferences.putString(beachNameKey, newConfig.beachName) == newConfig.beachName.length() &&
        preferences.putString(tideStationKey, newConfig.tideStation) == newConfig.tideStation.length() &&
        preferences.putString(timezoneKey, newConfig.timezone) == newConfig.timezone.length() &&
        preferences.putULong(textColorKey, newConfig.textColor) == sizeof(uint32_t) &&
        preferences.putULong(backgroundColorKey, newConfig.backgroundColor) == sizeof(uint32_t) &&
        preferences.putString(displayStyleKey, displayStyle) == strlen(displayStyle) &&
        preferences.putBool(alarmEnabledKey, newConfig.alarmEnabled) == sizeof(bool) &&
        preferences.putUChar(alarmHourKey, newConfig.alarmHour) == sizeof(uint8_t) &&
        preferences.putUChar(alarmMinuteKey, newConfig.alarmMinute) == sizeof(uint8_t) &&
        preferences.putUChar(brightnessKey, newConfig.brightness) == sizeof(uint8_t) &&
        preferences.putBool(nightModeKey, newConfig.nightModeEnabled) == sizeof(bool) &&
        preferences.putUChar(nightBrightnessKey, newConfig.nightBrightness) == sizeof(uint8_t) &&
        preferences.putUShort(nightStartKey, newConfig.nightStartMinute) == sizeof(uint16_t) &&
        preferences.putUShort(nightEndKey, newConfig.nightEndMinute) == sizeof(uint16_t);

    if (!ssidSaved || !passwordSaved || !additionalNetworksSaved || !settingsSaved ||
        !preferences.putBool(configuredKey, true))
    {
        Serial.printf("Config: storage failed (ssid=%d password=%d additional=%d settings=%d)\n",
                      ssidSaved, passwordSaved, additionalNetworksSaved, settingsSaved);
        configured = wasConfigured;
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

bool ConfigService::wasAlarmTriggered(uint32_t day, uint16_t scheduledMinute)
{
    return initialized && preferences.getULong(lastAlarmDayKey, 0) == day &&
           preferences.getUShort(lastAlarmMinuteKey, UINT16_MAX) == scheduledMinute;
}

bool ConfigService::markAlarmTriggered(uint32_t day, uint16_t scheduledMinute)
{
    return initialized &&
           preferences.putUShort(lastAlarmMinuteKey, scheduledMinute) == sizeof(uint16_t) &&
           preferences.putULong(lastAlarmDayKey, day) == sizeof(uint32_t);
}

bool ConfigService::saveDisplayStyle(DisplayStyle style)
{
    const char *name = displayStyleName(style);
    if (!initialized || name == nullptr || preferences.putString(displayStyleKey, name) != strlen(name))
    {
        return false;
    }
    config.displayStyle = style;
    return true;
}

void ConfigService::setDisplayStylePreview(DisplayStyle style)
{
    config.displayStyle = style;
}

bool ConfigService::saveBrightness(uint8_t brightness, bool nightModeEnabled)
{
    if (!initialized || brightness < 25 || brightness > 100 ||
        preferences.putUChar(brightnessKey, brightness) != sizeof(uint8_t) ||
        preferences.putBool(nightModeKey, nightModeEnabled) != sizeof(bool))
    {
        return false;
    }
    config.brightness = brightness;
    config.nightModeEnabled = nightModeEnabled;
    return true;
}
