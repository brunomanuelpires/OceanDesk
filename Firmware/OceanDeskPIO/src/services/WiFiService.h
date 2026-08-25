#pragma once

class WiFiService
{
public:
    static void begin();
    static void update();
    static bool isConnected();
};