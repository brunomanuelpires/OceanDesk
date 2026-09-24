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
constexpr unsigned long networkAttemptDuration = 12UL * 1000UL;
constexpr unsigned long offlineSetupDelay = 45UL * 1000UL;
struct KnownNetwork { String ssid; String password; };
KnownNetwork knownNetworks[3];
uint8_t knownNetworkCount = 0;
uint8_t activeNetwork = 0;
unsigned long networkAttemptStartedAt = 0;
unsigned long disconnectedSince = 0;
wl_status_t lastStatus = WL_NO_SHIELD;
WiFiMode currentMode = WiFiMode::Off;

void startNetwork(uint8_t index)
{
    activeNetwork = index;
    networkAttemptStartedAt = millis();
    Serial.printf("WiFi: trying saved network %u of %u\n", index + 1, knownNetworkCount);
    WiFi.disconnect(false, false);
    WiFi.begin(knownNetworks[index].ssid.c_str(), knownNetworks[index].password.c_str());
}

bool startFallbackAccessPoint()
{
    const IPAddress localIp(192, 168, 4, 1);
    const IPAddress gateway(192, 168, 4, 1);
    const IPAddress subnet(255, 255, 255, 0);

    if (currentMode == WiFiMode::SetupAccessPoint) return true;
    Serial.println("WiFi: no saved network available; opening setup network");
    WiFi.mode(WIFI_AP_STA);
    if (!WiFi.softAPConfig(localIp, gateway, subnet) || !WiFi.softAP(setupSsid))
    {
        Serial.println("WiFi: failed to start fallback setup network");
        return false;
    }
    currentMode = WiFiMode::SetupAccessPoint;
    return true;
}
}

void WiFiService::beginStation(const DeviceConfig &config)
{
    Serial.println("WiFi: starting connection...");

    knownNetworkCount = 0;
    const auto addNetwork = [](const String &ssid, const String &password) {
        if (!ssid.isEmpty() && knownNetworkCount < 3)
        {
            knownNetworks[knownNetworkCount++] = {ssid, password};
        }
    };
    addNetwork(config.wifiSsid, config.wifiPassword);
    addNetwork(config.wifiSsid2, config.wifiPassword2);
    addNetwork(config.wifiSsid3, config.wifiPassword3);

    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(false);
    lastStatus = WiFi.status();
    currentMode = WiFiMode::Station;
    disconnectedSince = millis();
    if (knownNetworkCount > 0) startNetwork(0);
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
    if (currentMode != WiFiMode::Station && currentMode != WiFiMode::SetupAccessPoint)
    {
        return;
    }

    if (currentMode == WiFiMode::SetupAccessPoint && knownNetworkCount == 0) return;

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
            disconnectedSince = 0;
            if (currentMode == WiFiMode::SetupAccessPoint)
            {
                WiFi.softAPdisconnect(true);
                WiFi.mode(WIFI_STA);
                currentMode = WiFiMode::Station;
                Serial.println("WiFi: setup network closed");
            }
        }
        else
        {
            Serial.print("WiFi: status changed to ");
            Serial.println(static_cast<int>(currentStatus));
        }
    }

    if (currentStatus != WL_CONNECTED)
    {
        if (disconnectedSince == 0) disconnectedSince = now;
        if (knownNetworkCount > 0 && now - networkAttemptStartedAt >= networkAttemptDuration)
        {
            startNetwork((activeNetwork + 1) % knownNetworkCount);
        }
        if (now - disconnectedSince >= offlineSetupDelay)
        {
            startFallbackAccessPoint();
        }
    }
}

bool WiFiService::isConnected()
{
    return currentMode == WiFiMode::Station && WiFi.status() == WL_CONNECTED;
}

bool WiFiService::isOffline()
{
    return !isConnected();
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
