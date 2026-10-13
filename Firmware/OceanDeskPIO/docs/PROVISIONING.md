# OceanDesk production provisioning

Every OceanDesk has a permanent internal `device_id` derived from the ESP32-S3
factory STA MAC. It is displayed as `OD-` followed by the twelve hexadecimal
MAC digits, for example `OD-58E6C571BC5C`. It is stable across normal firmware
updates and does not depend on NVS.

The customer-facing serial is stored separately in the `od_identity` NVS
namespace. A device without a provisioned serial shows `TEST-` followed by the
last six digits of its device ID; this is deliberately not a production serial.
Factory reset clears Wi-Fi and user settings, but never clears `od_identity`.

## Assigning a serial

At final production test, assign the next number from the production register
and flash once with it. For example, for the first two devices:

```sh
OCEANDESK_PROVISION_SERIAL=OD-000001 pio run -t upload
OCEANDESK_PROVISION_SERIAL=OD-000002 pio run -t upload
```

The build hook only accepts uppercase `OD-...` serials. After that one-time
flash, build and install the normal release firmware with no environment
variable. The assigned serial remains in NVS across ordinary reflashes and
factory resets. Record the serial, displayed device ID, hardware revision and
final-test date together in the production register before packaging.

Do not use the provisioning variable for a shared release build: it writes the
same serial to every device it is flashed onto.
