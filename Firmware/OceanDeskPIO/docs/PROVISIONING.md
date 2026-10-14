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

At final production test, reserve the next number in the production register,
then build and flash a one-unit provisioning image. For example, for the first
device:

```sh
OCEANDESK_PROVISION_SERIAL=OD-000001 \
OCEANDESK_PROVISION_CONFIRM=SINGLE_UNIT \
pio run -t upload
```

The build hook only accepts uppercase `OD-...` serials and requires an explicit
single-unit acknowledgement. The firmware refuses to replace an existing,
different serial. After this one-time flash, build and install the normal
release firmware with no provisioning variables. The assigned serial remains
in NVS across ordinary reflashes and factory resets. Record the serial,
displayed device ID, firmware version, hardware revision and final-test date
together in the production register before packaging.

Do not use the provisioning variable for a shared release build: it writes the
same serial to every device it is flashed onto.

Use [the final production checklist](PRODUCTION_CHECKLIST.md) for the complete
test and final-reset sequence. See [the security procedure](SECURE_BOOT_FLASH_ENCRYPTION.md)
for the intentionally separate Secure Boot v2 / Flash Encryption test process.
