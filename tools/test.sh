#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
python3 tests/public_package_test.py
out=$(mktemp -d)
trap 'rm -rf "$out"' EXIT
for name in protocol runtime_config mqtt http_request_body firmware_policy; do
  clang++ -std=c++17 -fsanitize=address,undefined -Wno-deprecated-declarations -I tests "tests/${name}_test.cpp" -lexpat -o "$out/$name"
  "$out/$name"
done
python3 - "$out/page.js" <<'PY'
from pathlib import Path
import sys
page=Path('components/fritzbox_aha/gateway.html').read_text()
Path(sys.argv[1]).write_text(page.split('<script>')[1].split('</script>')[0])
PY
"${NODE_BINARY:-node}" --check "$out/page.js"
