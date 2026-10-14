# OceanDesk final production checklist

Use one checklist per physical unit.  Do not reuse a serial number and do not
record customer Wi-Fi credentials in the production register.

| Step | Pass | Record / acceptance criteria |
| --- | --- | --- |
| Intake | ☐ | PCB/display hardware revision and assembly date recorded. |
| Visual inspection | ☐ | Enclosure, USB connector, display cable and antenna seated; no damage. |
| Display | ☐ | Full-screen red, green, blue, white and black test: no persistent lines, dead pixels or RGB artefacts. |
| Touch | ☐ | All four screen corners and main navigation controls respond correctly. |
| Wi-Fi | ☐ | Join a controlled test SSID; web UI works; disconnect/restart and confirm offline path remains usable. Forget the test network afterwards. |
| NFC | ☐ | Read a known test tag and verify the expected UI/action. |
| RTC | ☐ | Set/synchronise time, power-cycle for at least two minutes, then confirm time/date remain correct. |
| Buzzer | ☐ | Audible test tone without distortion or continuous output. |
| Tide data | ☐ | Select Nazaré, Peniche and Cascais; correct model/station shown and a tide prediction is present. |
| Identity | ☐ | Record immutable `device_id`, assign unused `OD-######` serial, verify both at `/system`; record firmware and hardware version. |
| Alarm and night mode | ☐ | Create and trigger a short test alarm; enable/disable night mode and verify display behaviour. Remove test alarm. |
| Final reset | ☐ | Run the customer-settings factory reset; after restart confirm setup mode, no saved Wi-Fi/alarm/beach preference and the assigned serial/device ID remain intact. |
| Release | ☐ | Install normal release image (never a provisioning image), reboot, rerun `/system` identity check, sign/date this record. |

**Stop conditions:** duplicate serial, `TEST-…` serial on a release candidate,
identity mismatch, display artefacts, failed RTC retention, or any test that
does not survive its required restart. Quarantine the unit; do not package it.
