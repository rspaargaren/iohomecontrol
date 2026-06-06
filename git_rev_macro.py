import subprocess

try:
    version = subprocess.check_output(
        ["git", "describe", "--tags", "--always", "--dirty"],
        stderr=subprocess.DEVNULL
    ).decode().strip()
except Exception:
    version = "unknown"

print(f"-DFIRMWARE_VERSION='\"{version}\"'")
