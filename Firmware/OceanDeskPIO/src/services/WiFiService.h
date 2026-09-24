#pragma once

#include "ConfigService.h"

class WiFiService
{
public:
    static void beginStation(const DeviceConfig &config);
    static bool beginSetupAccessPoint();
    static void update();
    static bool isConnected();
    static bool isOffline();
    static bool isSetupMode();
    static const char *setupNetworkName();
    static const char *setupAddress();
};
