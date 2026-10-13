Import("env")

import os
import re


serial_number = os.environ.get("OCEANDESK_PROVISION_SERIAL", "")
if serial_number:
    if not re.fullmatch(r"OD-[A-Z0-9-]{1,12}", serial_number):
        raise ValueError("OCEANDESK_PROVISION_SERIAL must be an uppercase serial such as OD-000001")
    env.Append(CPPDEFINES=[("OCEANDESK_PROVISION_SERIAL", '\"%s\"' % serial_number)])
