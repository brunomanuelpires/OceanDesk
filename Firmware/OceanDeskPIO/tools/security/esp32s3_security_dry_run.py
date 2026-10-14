#!/usr/bin/env python3
"""Print-only guardrail for the ESP32-S3 Secure Boot v2 test procedure.

This script cannot talk to a serial port, invoke Espressif tooling, create
keys, flash firmware or burn eFuses. It exists to make the review gate
repeatable before a separately approved sacrificial-unit operation.
"""

import argparse
import sys


def main() -> int:
    parser = argparse.ArgumentParser(description="Print the non-destructive ESP32-S3 security rehearsal")
    parser.add_argument("--unit", required=True, help="physical sacrificial test-unit label")
    parser.add_argument("--port", help="rejected: dry run never accepts a device port")
    parser.add_argument("--execute", action="store_true", help="rejected: this tool cannot execute security actions")
    args = parser.parse_args()

    if args.port or args.execute:
        parser.error("dry-run mode accepts neither --port nor --execute; no hardware action is available")
    if "SACRIFICIAL" not in args.unit.upper():
        parser.error("--unit must explicitly contain SACRIFICIAL; development and production units are rejected")

    print(f"Security dry run for: {args.unit}")
    print("1. Record device ID/MAC and current eFuse summary (manually, on the test unit).")
    print("2. Verify signed-image boot before any irreversible operation.")
    print("3. Review encrypted boot, OTA, factory reset and identity-retention test plan.")
    print("4. Obtain a separate approval that names this unit and firmware hashes.")
    print("No keys were generated. No serial port was opened. No flash/eFuse state changed.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
