#!/usr/bin/env bash
set -euo pipefail
ENGINE="${1:-engine}"
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
mkdir -p "$ENGINE/include/pokewilds" "$ENGINE/src/pokewilds"
cp "$ROOT/include/pokewilds/world.h" "$ENGINE/include/pokewilds/world.h"
cp "$ROOT/include/pokewilds/game.h" "$ENGINE/include/pokewilds/game.h"
cp "$ROOT/src/pokewilds/world.c" "$ENGINE/src/pokewilds/world.c"
cp "$ROOT/src/pokewilds/game.c" "$ENGINE/src/pokewilds/game.c"

python3 - "$ENGINE/src/new_game.c" <<'PY'
from pathlib import Path
import sys
p=Path(sys.argv[1])
s=p.read_text()
needle='#include "follower_npc.h"'
if '#include "pokewilds/game.h"' not in s:
    s=s.replace(needle, needle+'\n#include "pokewilds/game.h"')
marker='EWRAM_DATA bool8 gEnableContestDebugging = FALSE;'
if 'sPokeWildsGameState' not in s:
    s=s.replace(marker, marker+'\nEWRAM_DATA static PwGameState sPokeWildsGameState = {0};')
old='''static void WarpToTruck(void)
{
    if (IS_FRLG)
        SetWarpDestination(MAP_GROUP(MAP_PALLET_TOWN_PLAYERS_HOUSE_2F), MAP_NUM(MAP_PALLET_TOWN_PLAYERS_HOUSE_2F), WARP_ID_NONE, 6, 6);
    else
        SetWarpDestination(MAP_GROUP(MAP_INSIDE_OF_TRUCK), MAP_NUM(MAP_INSIDE_OF_TRUCK), WARP_ID_NONE, -1, -1);
    WarpIntoMap();
}'''
new='''static void WarpToTruck(void)
{
    // PokéWilds GBA prototype boot: initialize deterministic wilderness state
    // and start outdoors so the ROM immediately reaches a playable field.
    PwGame_New(&sPokeWildsGameState, 0x504F4B45u);
    SetWarpDestination(MAP_GROUP(MAP_ROUTE101), MAP_NUM(MAP_ROUTE101), WARP_ID_NONE, 16, 8);
    WarpIntoMap();
}'''
if old not in s:
    raise SystemExit('Expected WarpToTruck block not found; upstream changed')
s=s.replace(old,new)
p.write_text(s)
PY
echo "PokéWilds GBA runtime overlay applied to $ENGINE"
