#!/usr/bin/env python3
"""Wrap an ESP32-S3 mask-ROM dump and merged flash for Simantic 0.8.6.

This is a simulator input, NOT a firmware image for flashing to a board.
The caller supplies its own ROM dump; no vendor ROM is downloaded or distributed.
"""
import argparse
from pathlib import Path
import struct

ROM_SIZE = 0x60000


def make_image(rom: bytes, flash: bytes) -> bytes:
    if len(rom) != ROM_SIZE:
        raise ValueError("ESP32-S3 ROM dump must contain exactly 0x60000 bytes")
    if not 0x10018 <= len(flash) <= 8 * 1024 * 1024:
        raise ValueError("Expected a merged flash image with bootloader and app (maximum 8 MB)")
    for offset in (0, 0x10000):
        if flash[offset] != 0xE9 or struct.unpack_from('<H', flash, offset + 12)[0] != 9:
            raise ValueError(f"Missing ESP32-S3 image header at {offset:#x}; use the merged image")
    # DROM is the data view of the final 128 KB of the S3's 384 KB mask ROM.
    # Flash is backing storage: the ROM/bootloader configures the application's
    # instruction/data mappings and CPU state, rather than us jumping into it.
    segments = [(0x40000000, rom, 5),
                (0x3FF00000, rom[0x40000:0x60000], 4),
                (0x30000000, flash, 6)]
    ident = b'\x7fELF\x01\x01\x01' + bytes(9)
    header = struct.pack('<16sHHIIIIIHHHHHH', ident, 2, 94, 1, 0x40000400,
                         52, 0, 0, 52, 32, len(segments), 0, 0, 0)
    image = bytearray(header + bytes(32 * len(segments)))
    for index, (address, data, flags) in enumerate(segments):
        offset = (len(image) + 4095) & ~4095
        image.extend(bytes(offset - len(image)))
        image.extend(data)
        struct.pack_into('<IIIIIIII', image, 52 + 32 * index, 1, offset,
                         address, address, len(data), len(data), flags, 4096)
    return bytes(image)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--rom', required=True, type=Path)
    parser.add_argument('--flash', required=True, type=Path)
    parser.add_argument('--output', required=True, type=Path)
    args = parser.parse_args()
    if args.output.resolve() in (args.rom.resolve(), args.flash.resolve()):
        parser.error('Output must not overwrite either input')
    try:
        image = make_image(args.rom.read_bytes(), args.flash.read_bytes())
    except (OSError, ValueError) as exc:
        parser.error(str(exc))
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_bytes(image)
    print(f'Simantic ROM-boot ELF: {args.output} ({len(image)} bytes; simulation only)')


if __name__ == '__main__':
    main()
