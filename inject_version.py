Import("env")
import os
import shutil

def inject_version(source, target, env):
    version = os.environ.get("RELEASE_VERSION", "DEV").strip() or "DEV"
    src = os.path.join("extras", "web_interface_data", "index.html")
    dst = os.path.join("extras", "web_interface_data", "index.html")

    with open(src, "r", encoding="utf-8") as f:
        content = f.read()

    new_content = content.replace("?v=PLACEHOLDER", f"?v={version}")

    if new_content != content:
        with open(dst, "w", encoding="utf-8") as f:
            f.write(new_content)
        print(f"inject_version: replaced ?v=PLACEHOLDER with ?v={version} in index.html")
    else:
        print(f"inject_version: nothing to replace (already versioned or no PLACEHOLDER found)")

env.AddPreAction("$BUILD_DIR/littlefs.bin", inject_version)
