#pragma once

#include <Arduino.h>

enum class DisplayStyle
{
    Classic,
    Graph,
    Info
};

struct DeviceConfig
{
    String wifiSsid;
    String wifiPassword;
    String wifiSsid2;
    String wifiPassword2;
    String wifiSsid3;
    String wifiPassword3;
    String beachId;
    String beachName;
    String tideStation;
    String timezone;
    // `textColor` is the original persisted text colour. It remains the
    // primary role so existing devices retain their selected appearance.
    uint32_t textColor = 0xFFFFFF;
    uint32_t secondaryTextColor = 0xB6C7D1;
    uint32_t tertiaryTextColor = 0x8196A4;
    uint32_t backgroundColor = 0x0B1620;
    DisplayStyle displayStyle = DisplayStyle::Classic;
    bool alarmEnabled = false;
    uint8_t alarmHour = 7;
    uint8_t alarmMinute = 0;
    uint8_t brightness = 100;
    bool nightModeEnabled = true;
    uint8_t nightBrightness = 35;
    uint16_t nightStartMinute = 22 * 60 + 30;
    uint16_t nightEndMinute = 7 * 60;
};

class ConfigService
{
public:
    static void begin();
    static bool isConfigured();
    static const DeviceConfig &get();
    static bool save(const DeviceConfig &newConfig);
    static bool reset();
    static bool selectBeach(DeviceConfig &target, const String &beachId);
    static const char *displayStyleName(DisplayStyle style);
    static bool parseDisplayStyle(const String &value, DisplayStyle &style);
    static bool wasAlarmTriggered(uint32_t day, uint16_t scheduledMinute);
    static bool markAlarmTriggered(uint32_t day, uint16_t scheduledMinute);
    static bool saveDisplayStyle(DisplayStyle style);
    static void setDisplayStylePreview(DisplayStyle style);
    static bool saveBrightness(uint8_t brightness, bool nightModeEnabled);
};
