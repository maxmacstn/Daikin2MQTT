Import("env")
import shutil, os, re

def copy_firmware(source, target, env):
    # Read version from config.h
    config_path = os.path.join(env["PROJECT_DIR"], "src", "config.h")
    version = "unknown"
    with open(config_path, "r") as f:
        for line in f:
            m = re.search(r'dk2mqtt_version\s*=\s*"([^"]+)"', line)
            if m:
                version = m.group(1).replace(".", "_").replace("-", "_")
                break

    src = str(target[0])
    dest_dir = os.path.join(env["PROJECT_DIR"], "bin releases")
    os.makedirs(dest_dir, exist_ok=True)
    dest = os.path.join(dest_dir, f"Daikin2MQTT_{version}_OTA.bin")
    shutil.copy2(src, dest)
    print(f"Copied firmware to: {dest}")

env.AddPostAction("$BUILD_DIR/firmware.bin", copy_firmware)
