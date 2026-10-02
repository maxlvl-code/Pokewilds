#!/usr/bin/env bash
set -euo pipefail
ENGINE="${1:-engine}"
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
mkdir -p "$ENGINE/include/pokewilds" "$ENGINE/src/pokewilds"
cp "$ROOT/include/pokewilds/world.h" "$ENGINE/include/pokewilds/world.h"
cp "$ROOT/include/pokewilds/game.h" "$ENGINE/include/pokewilds/game.h"
cp "$ROOT/src/pokewilds/world.c" "$ENGINE/src/pokewilds/world.c"
cp "$ROOT/src/pokewilds/game.c" "$ENGINE/src/pokewilds/game.c"
echo "PokéWilds portable core overlaid into $ENGINE"
