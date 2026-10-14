# ESP32-S3 production security procedure

This procedure prepares Secure Boot v2 and Flash Encryption for a **dedicated
sacrificial ESP32-S3 test unit only**. It must never be run against OceanDesk
development unit #001. It is intentionally not part of `pio run -t upload`.

## Preconditions

- Freeze and archive the exact production firmware binary, partition table and
  bootloader, including SHA-256 hashes.
- Use an isolated production workstation with a current Espressif `esptool` /
  ESP-IDF toolchain and an audited key-management process.
- Generate the Secure Boot v2 signing key and flash-encryption key in approved
  protected storage; do not commit them, put them in `DOWNLOADS/`, or paste
  them into tickets/logs.
- Confirm recovery/flashing tooling works on an unprotected sacrificial unit,
  and that the chosen partition layout supports encrypted OTA updates.
- Label the test unit with its MAC/device ID and `SACRIFICIAL SECURITY TEST`.

## Safe dry run

Run `python3 tools/security/esp32s3_security_dry_run.py --unit SACRIFICIAL-…`
first. It only prints the required sequence and checks that no connected port
or eFuse command has been supplied. It does **not** call `espefuse.py`, write
flash, generate keys, or alter eFuses.

## Controlled test-unit sequence

1. Capture and archive `espefuse.py summary` and a verified read-back of the
   currently flashed test image.
2. Build/sign the bootloader and application with Secure Boot v2 enabled using
   the offline signing key. Flash the signed test image and verify it boots
   before burning any protection.
3. In a witnessed, logged session, enable Flash Encryption using the approved
   release process, then verify encrypted boot, OTA update, factory reset and
   serial/device-ID retention.
4. Only after all checks pass, burn the Secure Boot v2 and download/debug
   protection eFuses prescribed by the approved Espressif release procedure.
   Read back the eFuse summary and retain it in the unit record.

These steps include irreversible eFuse operations. A failure after encryption
or key protection may make recovery impossible. Require a separate change
approval naming the exact device ID, firmware hashes, operator and witness.

## Explicitly not executed by this repository

- No eFuse has been burned.
- No Flash Encryption has been enabled.
- No Secure Boot v2 key has been generated, stored or enrolled.
- No security command may target OceanDesk #001.

The required manufacturer references are Espressif's ESP-IDF documentation for
the ESP32-S3 Secure Boot v2 and Flash Encryption release procedures. Recheck
that documentation against the pinned Espressif toolchain before authorising a
production run.
