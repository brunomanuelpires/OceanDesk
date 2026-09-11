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
    String beachName;
    String tideStation;
    String timezone;
    uint32_t textColor;
    uint32_t backgroundColor;
    DisplayStyle displayStyle = DisplayStyle::Classic;
};

class ConfigService
{
public:
    static void begin();
    static bool isConfigured();
    static const DeviceConfig &get();
    static bool save(const DeviceConfig &newConfig);
    static bool reset();
    static const char *displayStyleName(DisplayStyle style);
    static bool parseDisplayStyle(const String &value, DisplayStyle &style);
};
