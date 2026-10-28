"""PlatformIO custom target: pio run -e freenove_esp32s3 -t merged.

Use the same boot images, offsets and flash settings as the upload target.
No device is accessed. The result is a fresh flash image for ROM-boot simulation.
"""
from pathlib import Path
import subprocess

Import("env")


def merge_flash(source, target, env):
    output = Path(env.subst("$BUILD_DIR")) / "firmware-merged.bin"
    command = [
        env.subst("$PYTHONEXE"), env.subst("$OBJCOPY"),
        "--chip", "esp32s3", "merge_bin", "-o", str(output),
        "--flash_mode", env.subst("${__get_board_flash_mode(__env__)}"),
        "--flash_freq", env.subst("${__get_board_f_image(__env__)}"),
        "--flash_size", env.BoardConfig().get("upload.flash_size"),
    ]
    for offset, image in env.get("FLASH_EXTRA_IMAGES", []):
        command.extend([offset, env.subst(image)])
    command.extend([env.subst("$ESP32_APP_OFFSET"), env.subst("$BUILD_DIR/${PROGNAME}.bin")])
    subprocess.run(command, check=True)


env.AddCustomTarget(
    "merged", "$BUILD_DIR/${PROGNAME}.bin", merge_flash,
    title="Merge flash image", description="Build a ROM-boot image for simulation",
)


def simantic_image(source, target, env):
    import os
    rom = os.environ.get("SIMANTIC_ESP32S3_ROM")
    if not rom or not Path(rom).is_file():
        raise RuntimeError("Set SIMANTIC_ESP32S3_ROM to your 384 KB ESP32-S3 mask-ROM dump; see README")
    merge_flash(source, target, env)
    build = Path(env.subst("$BUILD_DIR"))
    subprocess.run([
        env.subst("$PYTHONEXE"),
        str(Path(env.subst("$PROJECT_DIR")) / "tools" / "make_simantic_image.py"),
        "--rom", rom, "--flash", str(build / "firmware-merged.bin"),
        "--output", str(build / "firmware-simantic.elf"),
    ], check=True)


env.AddCustomTarget(
    "simantic", "$BUILD_DIR/${PROGNAME}.bin", simantic_image,
    title="Package Simantic boot ELF", description="Requires SIMANTIC_ESP32S3_ROM; simulation only",
)
