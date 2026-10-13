#pragma once

#include <Arduino.h>

struct DeviceIdentity
{
    String serialNumber;
    String deviceId;
    String firmwareVersion;
    String hardwareModel;
    bool serialProvisioned = false;
};

class DeviceIdentityService
{
public:
    static void begin();
    static const DeviceIdentity &get();
    static bool provisionSerialNumber(const String &serialNumber);
    static const char *restartReason();
};
