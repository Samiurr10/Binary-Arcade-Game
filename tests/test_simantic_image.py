import importlib.util
from pathlib import Path
import struct
import unittest

script = Path(__file__).resolve().parents[1] / 'firmware/esp32/tools/make_simantic_image.py'
spec = importlib.util.spec_from_file_location('simantic_image', script)
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)


class ImageTests(unittest.TestCase):
    def setUp(self):
        self.rom = bytes(range(256)) * (0x60000 // 256)
        self.flash = bytearray(0x11000)
        for offset in (0, 0x10000):
            self.flash[offset] = 0xE9
            struct.pack_into('<H', self.flash, offset + 12, 9)

    def test_rom_boot_segments_and_entry(self):
        image = module.make_image(self.rom, self.flash)
        self.assertEqual(image[:7], b'\x7fELF\x01\x01\x01')
        self.assertEqual(struct.unpack_from('<H', image, 18)[0], 94)  # Xtensa
        self.assertEqual(struct.unpack_from('<I', image, 24)[0], 0x40000400)
        self.assertEqual(struct.unpack_from('<H', image, 44)[0], 3)
        expected = [(0x40000000, self.rom), (0x3FF00000, self.rom[0x40000:]),
                    (0x30000000, self.flash)]
        for i, (address, data) in enumerate(expected):
            kind, offset, virtual, physical, size, memory, flags, alignment = struct.unpack_from('<8I', image, 52 + 32 * i)
            self.assertEqual((kind, virtual, physical, size, memory), (1, address, address, len(data), len(data)))
            self.assertEqual(offset % alignment, 0)
            self.assertEqual(image[offset:offset + size], data)

    def test_reject_incomplete_or_wrong_chip_inputs(self):
        for rom, flash in [(self.rom[:-1], self.flash), (self.rom, self.flash[0x10000:]),
                           (self.rom, bytes(len(self.flash)))]:
            with self.assertRaises(ValueError):
                module.make_image(rom, flash)
        self.flash[12] = 0  # original ESP32, not S3
        with self.assertRaises(ValueError):
            module.make_image(self.rom, self.flash)


if __name__ == '__main__':
    unittest.main()
