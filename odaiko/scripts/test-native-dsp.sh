#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
OUT="$(mktemp -d)"
cc -std=c11 -O2 -fno-builtin -c src/main/cpp/OdaikoCore.c -o "$OUT/core.o"
c++ -std=c++17 -O2 -c src/main/cpp/OdaikoProfile.cpp -o "$OUT/profile.o"
c++ -std=c++17 -O2 tests/dsp_smoke.cpp "$OUT/core.o" "$OUT/profile.o" -lm -o "$OUT/test"
"$OUT/test"
