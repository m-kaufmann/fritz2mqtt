"""Keep the embedded gateway page identical to its editable source."""
from pathlib import Path
root = Path(__file__).resolve().parents[1] / 'components/fritzbox_aha'
page = (root / 'gateway.html').read_text()
assert ')F2M"' not in page
(root / 'gateway_page.h').write_text('#pragma once\nnamespace esphome::fritzbox_aha {\nstatic const char GATEWAY_PAGE[] = R"F2M(\n' + page + '\n)F2M";\n}\n')
