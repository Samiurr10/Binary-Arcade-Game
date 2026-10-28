#!/usr/bin/env python3
"""Embed the offline UI. Run after editing web/index.html; --check is for CI."""
from pathlib import Path
import sys
root = Path(__file__).resolve().parents[1]
html = (root / 'web/index.html').read_text()
assert ')ARCADE"' not in html
content = '#pragma once\n#include <Arduino.h>\nstatic const char web_ui[] PROGMEM = R"ARCADE(' + html + ')ARCADE";\n'
target = root / 'include/web_ui.hpp'
if '--check' in sys.argv:
    if not target.exists() or target.read_text() != content:
        sys.exit('web_ui.hpp is stale; run firmware/esp32/tools/embed_web.py')
else:
    target.write_text(content)
