#include "WiFiService.h"

#include <WiFi.h>

static const char *SSID = "Casa";
static const char *PASSWORD = "casa2023";

namespace
{
constexpr unsigned long reconnectInterval = 30UL * 1000UL;
unsigned long lastReconnectAttempt = 0;
wl_status_t lastStatus = WL_NO_SHIELD;
}

void WiFiService::begin()
{
    Serial.println("WiFi: starting connection...");

    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(true);
    WiFi.begin(SSID, PASSWORD);
    lastReconnectAttempt = millis();
    lastStatus = WiFi.status();
}

void WiFiService::update()
{
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
    return WiFi.status() == WL_CONNECTED;
}
