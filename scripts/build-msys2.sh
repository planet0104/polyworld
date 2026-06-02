#!/usr/bin/env bash
# Build Polyworld on MSYS2 UCRT64. Run only inside "MSYS2 UCRT64" terminal.
set -euo pipefail

if [[ -z "${MSYSTEM:-}" ]] || [[ "${MSYSTEM}" != "UCRT64" && "${MSYSTEM}" != "MINGW64" ]]; then
	echo "Error: open the MSYS2 UCRT64 (or MINGW64) shell, not PowerShell or MSYS." >&2
	exit 1
fi

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

if [[ -f configure ]]; then
	sed -i 's/\r$//' configure 2>/dev/null || true
fi

./configure --os msys2 "$@"
make app -j"$(nproc)"

echo ""
echo "Build OK. Run from: $ROOT"
echo "  export PATH=\"\$PWD/lib:\$PATH\""
echo "  ./Polyworld.exe worldfiles/tests/low-spec-pc/minitest.wf"
