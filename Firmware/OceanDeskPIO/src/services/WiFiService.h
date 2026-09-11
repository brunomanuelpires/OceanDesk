#pragma once

class WiFiService
{
public:
    static void beginStation(const char *ssid, const char *password);
    static bool beginSetupAccessPoint();
    static void update();
    static bool isConnected();
    static bool isSetupMode();
    static const char *setupNetworkName();
    static const char *setupAddress();
};
