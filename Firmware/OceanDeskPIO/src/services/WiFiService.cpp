#include "WiFiService.h"

#include <WiFi.h>

static const char *SSID = "Casa";
static const char *PASSWORD = "casa2023";

void WiFiService::begin()
{
    Serial.println("WiFi: starting connection...");

    WiFi.mode(WIFI_STA);
    WiFi.begin(SSID, PASSWORD);
}

void WiFiService::update()
{
    static wl_status_t lastStatus = WL_NO_SHIELD;
    static unsigned long lastAttempt = 0;

    wl_status_t currentStatus = WiFi.status();

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

    if (currentStatus != WL_CONNECTED)
    {
        if (millis() - lastAttempt > 10000)
        {
            lastAttempt = millis();

            Serial.println("WiFi: reconnecting...");

            WiFi.disconnect();
            WiFi.begin(SSID, PASSWORD);
        }
    }
}

bool WiFiService::isConnected()
{
    return WiFi.status() == WL_CONNECTED;
}