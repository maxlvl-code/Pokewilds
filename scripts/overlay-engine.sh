#!/usr/bin/env bash
set -euo pipefail
ENGINE="${1:?Pass the pinned engine directory}"
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
mkdir -p "$ENGINE/include/pokewilds" "$ENGINE/src/pokewilds" "$ENGINE/graphics/pokewilds"
cp -u "$ROOT"/include/pokewilds/*.h "$ENGINE/include/pokewilds/"
cp -u "$ROOT"/src/pokewilds/*.c "$ENGINE/src/pokewilds/"
cp -u "$ROOT"/assets/*.h "$ENGINE/include/pokewilds/"
cp -u "$ROOT"/assets/*.4bpp "$ROOT"/assets/*.8bpp "$ROOT"/assets/*.gbapal "$ENGINE/graphics/pokewilds/"
python3 - "$ENGINE" <<'PY'
from pathlib import Path
import sys
root=Path(sys.argv[1])
def patch(name,old,new):
 p=root/name;s=p.read_text()
 if new in s:return
 if s.count(old)!=1:raise SystemExit(f'{name}: expected one matching upstream block, got {s.count(old)}')
 p.write_text(s.replace(old,new))
patch('include/global.h','struct SaveBlock3\n{','#include "pokewilds/world.h"\n\nstruct SaveBlock3\n{\n    struct PwSave wilds;')
patch('ld_script_modern.ld','gInitialMainCB2 = CB2_InitCopyrightScreenAfterBootup;','gInitialMainCB2 = CB2_PwBoot;')
patch('src/battle_main.c','#include "global.h"','#include "global.h"\n#include "pokewilds/runtime.h"')
patch('src/battle_main.c','gBattleEnvironment = BattleSetup_GetEnvironmentId();','gBattleEnvironment = Pw_GetBattleEnvironment();')
# Native summaries use this metadata label; no campaign map is displayed.
p=root/'src/data/region_map/region_map_sections.json'
s=p.read_text();updated=s.replace('"name": "ROUTE 101"','"name": "WILDERNESS"')
if updated != s:p.write_text(updated)
PY
