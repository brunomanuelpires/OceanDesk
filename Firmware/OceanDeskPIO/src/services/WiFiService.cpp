#include "WiFiService.h"

#include <WiFi.h>

namespace
{
enum class WiFiMode
{
    Off,
    Station,
    SetupAccessPoint
};

constexpr const char *setupSsid = "OceanDesk_Setup";
constexpr const char *setupIpAddress = "192.168.4.1";
constexpr unsigned long reconnectInterval = 30UL * 1000UL;
unsigned long lastReconnectAttempt = 0;
wl_status_t lastStatus = WL_NO_SHIELD;
WiFiMode currentMode = WiFiMode::Off;
}

void WiFiService::beginStation(const char *ssid, const char *password)
{
    Serial.println("WiFi: starting connection...");

    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(true);
    WiFi.begin(ssid, password);
    lastReconnectAttempt = millis();
    lastStatus = WiFi.status();
    currentMode = WiFiMode::Station;
}

bool WiFiService::beginSetupAccessPoint()
{
    const IPAddress localIp(192, 168, 4, 1);
    const IPAddress gateway(192, 168, 4, 1);
    const IPAddress subnet(255, 255, 255, 0);

    Serial.print("WiFi: starting setup network ");
    Serial.println(setupSsid);

    WiFi.mode(WIFI_AP);
    if (!WiFi.softAPConfig(localIp, gateway, subnet) || !WiFi.softAP(setupSsid))
    {
        Serial.println("WiFi: failed to start setup network");
        currentMode = WiFiMode::Off;
        return false;
    }

    currentMode = WiFiMode::SetupAccessPoint;
    Serial.print("WiFi setup address: http://");
    Serial.println(WiFi.softAPIP());
    return true;
}

void WiFiService::update()
{
    if (currentMode != WiFiMode::Station)
    {
        return;
    }

    const unsigned long now = millis();
    const wl_status_t currentStatus = WiFi.status();

    if (currentStatus != lastStatus)
    {
        lastStatus = currentStatus;

        if (currentStatus == WL_CONNECTED)
        {
            Serial.println("WiFi: connected");
            Serial.print("WiFi IP: ");
            Serial.println(WiFi.localIP());
        }
        else
        {
            Serial.print("WiFi: status changed to ");
            Serial.println(static_cast<int>(currentStatus));
        }
    }

    if (currentStatus != WL_CONNECTED &&
        now - lastReconnectAttempt >= reconnectInterval)
    {
        lastReconnectAttempt = now;
        Serial.println("WiFi: requesting reconnection...");
        WiFi.reconnect();
    }
}

bool WiFiService::isConnected()
{
    return currentMode == WiFiMode::Station && WiFi.status() == WL_CONNECTED;
}

bool WiFiService::isSetupMode()
{
    return currentMode == WiFiMode::SetupAccessPoint;
}

const char *WiFiService::setupNetworkName()
{
    return setupSsid;
}

const char *WiFiService::setupAddress()
{
    return setupIpAddress;
}
