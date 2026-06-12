import os

version = os.environ.get("RELEASE_VERSION", "DEV").strip() or "DEV"

print(f"-DFIRMWARE_VERSION='\"{version}\"'")
