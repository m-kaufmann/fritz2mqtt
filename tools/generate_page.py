"""Embed the gateway page and touch icon in the firmware."""
from pathlib import Path
root = Path(__file__).resolve().parents[1] / 'components/fritzbox_aha'
page = (root / 'gateway.html').read_text()
assert ')F2M"' not in page
(root / 'gateway_page.h').write_text('#pragma once\nnamespace esphome::fritzbox_aha {\nstatic const char GATEWAY_PAGE[] = R"F2M(\n' + page + '\n)F2M";\n}\n')

icons = []
for filename, symbol in [('apple-touch-icon.png', 'GATEWAY_TOUCH_ICON'), ('favicon.png', 'GATEWAY_FAVICON')]:
    icon = (root / filename).read_bytes()
    lines = ['  ' + ', '.join(f'0x{byte:02x}' for byte in icon[i:i+16]) + ','
             for i in range(0, len(icon), 16)]
    icons.append(f'static const uint8_t {symbol}[] = {{\n' + '\n'.join(lines) + '\n};\n')

(root / 'gateway_icon.h').write_text(
    '#pragma once\n#include <cstdint>\nnamespace esphome::fritzbox_aha {\n'
    + ''.join(icons) + '}\n'
)
