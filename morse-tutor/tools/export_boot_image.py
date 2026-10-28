Import("env")

from pathlib import Path
import shutil


def export_boot_image(source, target, env):
    framework = Path(env.PioPlatform().get_package_dir("framework-arduinoespressif32"))
    shutil.copyfile(
        framework / "tools" / "partitions" / "boot_app0.bin",
        Path(env.subst("$BUILD_DIR")) / "boot_app0.bin",
    )


env.AddPostAction("$BUILD_DIR/firmware.bin", export_boot_image)
