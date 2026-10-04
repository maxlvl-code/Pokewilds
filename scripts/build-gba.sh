#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
ENGINE="${1:-$ROOT/../engine}"
export PATH="${TOOLCHAIN:-/usr}/bin:$PATH"
PIN=e8bd1cd7b03fc032ea37e3ecd38b379b5d01a1e7
if [[ ! -d "$ENGINE/.git" ]]; then
    git clone --depth 1 --branch expansion/1.17.0 https://github.com/rh-hideout/pokeemerald-expansion.git "$ENGINE"
fi
if [[ "$(git -C "$ENGINE" rev-parse HEAD)" != "$PIN" ]]; then
    echo "Expected pokeemerald-expansion at $PIN; use a separate engine checkout." >&2
    exit 1
fi
make -C "$ROOT" test
bash "$ROOT/scripts/overlay-engine.sh" "$ENGINE"
make -C "$ENGINE" modern TOOLCHAIN="${TOOLCHAIN:-/usr}" TITLE=POKEWILDSGBA GAME_CODE=PWGE -j"${JOBS:-2}"
mkdir -p "$ROOT/build"
cp "$ENGINE/pokeemerald.gba" "$ROOT/build/Pokewilds-GBA-0.3.gba"
sha256sum "$ROOT/build/Pokewilds-GBA-0.3.gba"
