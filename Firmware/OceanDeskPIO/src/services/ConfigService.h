#pragma once

#include <Arduino.h>

enum class DisplayStyle
{
    Classic,
    Minimal,
    Graph,
    Info
};

struct DeviceConfig
{
    String wifiSsid;
    String wifiPassword;
    String beachId;
    String beachName;
    String tideStation;
    String timezone;
    uint32_t textColor;
    uint32_t backgroundColor;
    DisplayStyle displayStyle = DisplayStyle::Classic;
    bool alarmEnabled = false;
    uint8_t alarmHour = 7;
    uint8_t alarmMinute = 0;
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
};
