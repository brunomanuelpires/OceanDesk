Import("env")

import os
import re


serial_number = os.environ.get("OCEANDESK_PROVISION_SERIAL", "")
if serial_number:
    if os.environ.get("OCEANDESK_PROVISION_CONFIRM") != "SINGLE_UNIT":
        raise ValueError(
            "Set OCEANDESK_PROVISION_CONFIRM=SINGLE_UNIT to build a one-unit provisioning image"
        )
    if not re.fullmatch(r"OD-[A-Z0-9-]{1,12}", serial_number):
        raise ValueError("OCEANDESK_PROVISION_SERIAL must be an uppercase serial such as OD-000001")
    # Keep the value quoted after SCons expands CPPDEFINES, so C++ receives a
    # string literal rather than tokens such as OD-000001.
    env.Append(CPPDEFINES=[("OCEANDESK_PROVISION_SERIAL", '\\\"%s\\\"' % serial_number)])
