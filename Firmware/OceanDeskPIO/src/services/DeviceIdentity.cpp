#include "DeviceIdentity.h"

#include <Preferences.h>
#include <esp_mac.h>
#include <esp_system.h>

namespace
{
constexpr const char *preferencesNamespace = "od_identity";
constexpr const char *serialNumberKey = "serial";
#ifndef OCEANDESK_FIRMWARE_VERSION
#define OCEANDESK_FIRMWARE_VERSION "0.2.0-dev"
#endif
#ifndef OCEANDESK_HARDWARE_MODEL
#define OCEANDESK_HARDWARE_MODEL "OceanDesk 5B / ESP32-S3 N16R8"
#endif
constexpr const char *firmwareVersion = OCEANDESK_FIRMWARE_VERSION;
constexpr const char *hardwareModel = OCEANDESK_HARDWARE_MODEL;

Preferences preferences;
DeviceIdentity identity;
bool initialized = false;

bool isValidSerialNumber(const String &value)
{
    if (value.length() < 4 || value.length() > 15) return false;
    for (size_t index = 0; index < value.length(); ++index)
    {
        const char character = value[index];
        if (!((character >= 'A' && character <= 'Z') ||
              (character >= '0' && character <= '9') || character == '-'))
        {
            return false;
        }
    }
    return value.startsWith("OD-");
}

String defaultTestSerial(const String &deviceId)
{
    return String("TEST-") + deviceId.substring(deviceId.length() - 6);
}
}

void DeviceIdentityService::begin()
{
    if (initialized) return;

    uint8_t mac[6] = {};
    if (esp_read_mac(mac, ESP_MAC_WIFI_STA) != ESP_OK)
    {
        Serial.println("Identity: failed to read factory MAC");
        identity.deviceId = "OD-UNKNOWN";
    }
    else
    {
        char deviceId[20];
        snprintf(deviceId, sizeof(deviceId), "OD-%02X%02X%02X%02X%02X%02X",
                 mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
        identity.deviceId = deviceId;
    }

    identity.firmwareVersion = firmwareVersion;
    identity.hardwareModel = hardwareModel;
    if (!preferences.begin(preferencesNamespace, false))
    {
        Serial.println("Identity: failed to open persistent storage");
        identity.serialNumber = defaultTestSerial(identity.deviceId);
        initialized = true;
        return;
    }

    const String storedSerial = preferences.getString(serialNumberKey, "");
    identity.serialProvisioned = isValidSerialNumber(storedSerial);
    identity.serialNumber = identity.serialProvisioned ? storedSerial : defaultTestSerial(identity.deviceId);
    initialized = true;
}

const DeviceIdentity &DeviceIdentityService::get()
{
    return identity;
}

bool DeviceIdentityService::provisionSerialNumber(const String &serialNumber)
{
    if (!initialized || !isValidSerialNumber(serialNumber)) return false;
    if (preferences.putString(serialNumberKey, serialNumber) != serialNumber.length()) return false;
    identity.serialNumber = serialNumber;
    identity.serialProvisioned = true;
    return true;
}

const char *DeviceIdentityService::restartReason()
{
    switch (esp_reset_reason())
    {
    case ESP_RST_POWERON: return "Ligado";
    case ESP_RST_EXT: return "Reset externo";
    case ESP_RST_SW: return "Reinício por software";
    case ESP_RST_PANIC: return "Falha de software";
    case ESP_RST_INT_WDT: return "Watchdog de interrupção";
    case ESP_RST_TASK_WDT: return "Watchdog de tarefa";
    case ESP_RST_WDT: return "Watchdog";
    case ESP_RST_DEEPSLEEP: return "Deep sleep";
    case ESP_RST_BROWNOUT: return "Queda de tensão";
    case ESP_RST_SDIO: return "SDIO";
    default: return "Desconhecido";
    }
}
