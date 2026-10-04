# PokéWilds GBA 0.3

A rebuilt GBA wilderness prototype. Explore a generated world, recruit Pokémon for field work, gather supplies, build a camp, grow berries and place Pokémon in habitats that produce materials. Native battles, Pokémon data, boxes and flash saves use pokeemerald-expansion 1.17.0. The game boots into its own wilderness; there is no playable Hoenn campaign.

**This remains an unfinished port.** The full desktop PokéWilds feature set is not implemented.

## Play

Extract and open **Pokewilds-GBA-0.3.gba** in your GBA emulator. Choose **New world**, then press **START**. No compilation or Linux setup is needed to play the supplied ROM.

**Start a fresh 0.3 save.** World generation and the save layout changed. Keep your old 0.2 ROM and save separately; do not rename its `.sav` or load its save states into 0.3.

You begin with Machop, 15 Poké Balls, five Potions and a few materials. Four friendly Pokémon wait near camp:

| Pokémon | Where from the starting tile | What it helps with |
| --- | --- | --- |
| Oddish | 3 tiles left, 2 down | CUT trees and plants for wood/fiber |
| Geodude | 4 right, 3 down | SMASH rocks for stone; DIG planting plots |
| Caterpie | 3 right, 3 up | Produces silky thread when placed in a happy habitat |
| Pidgey | 4 left, 3 up | Produces soft feathers when placed in a happy habitat |

Walk next to a friendly Pokémon, face it, press **A**, and choose **Invite**. Your first Pokémon follows you. Healthy party members provide their field skills automatically; **L** shows which helpers you have.

To make a bed, recruit Caterpie and Pidgey, face an empty patch of grass, open **START → Pokémon**, highlight one, and press **START** to place it. Place the other nearby. After 90 seconds of active world play, face each resident and choose **Collect materials**. A bed needs **6 wood, 2 thread and 2 feathers**. Collecting does not remove the Pokémon; **Pick up** brings the same Pokémon back into your party.

With Geodude in your party, face bare ground and press **A** to DIG. Press **A** again to plant a seed. After two minutes of world play, harvest the plant for three berries and two seeds. Menus and battles pause crop and habitat timers.

Use **START → Save** and wait for “World saved” before closing. The save type is **Flash 128K**. Keep the emulator's `.sav` beside the ROM. The campfire south of spawn and placed beds restore HP, PP and status.

## Controls

| Screen | Controls |
| --- | --- |
| World | D-pad move; hold B to run; A interact/gather; R build; L field skills; START menu; SELECT map |
| Seed | Left/right choose digit; up/down change it; R randomize; START generate; B back |
| Build | D-pad cursor; L/R choose piece; A place; SELECT dismantle; B leave |
| Menu | Up/down choose; A open; B back |
| Pokémon | A summary; SELECT make highlighted Pokémon the lead; START place it in a habitat |
| Friendly Pokémon | A Invite/Battle/Leave; B leave |
| Habitat | A Collect/Pick up/Leave; B leave |
| Bag | A use a Potion on the lead; throw Poké Balls from the native battle Bag |
| Storage | D-pad slot; L/R box; START choose party member; SELECT deposit; A withdraw |

## Changes from 0.2

- Fixed the camera's tile jump and replaced full moving-view redraws with entering rows/columns.
- Generate terrain a few rows per frame; uncached collision queries evaluate just the requested tile.
- Capture brief button presses during rendering and buffer actions pressed while walking.
- Corrected Pokémon sprite atlas indices and direction/frame order. The 0.2 importer incorrectly assumed National Dex order.
- Composite grass, trees, flowers and furniture correctly, with a separate canopy layer, shoreline edges and animated water/fire.
- Added friendly recruitment, a following partner, roaming wild Pokémon and six persistent habitats.
- Added Grass CUT, Rock SMASH, Fighting BUILD and Ground DIG. Selected evolved Water Pokémon provide SURF; bridges also cross water.
- Added seeds, berry growth/harvesting, silky thread, soft feathers and Pokémon-produced building supplies.
- Added roofs over floors and a 12-minute day/night cycle. Night tint survives changing the following Pokémon.
- Extended save validation for crops/residents; material timers no longer wrap after long sessions.

## Recipes and construction

Poké Ball: **5 wood + 2 stone**. Potion: **3 fiber + 1 berry**.

Build pieces: floor, wall, door, campfire, bed, bridge, fence and roof. Fighting Pokémon must be healthy to start building. Roofs go on floors; walls, doors and beds can also replace floors. Bridges go on water. Grass helpers dismantle buildings for half their material cost. The starting campfire cannot be dismantled.

Happy habitats: Water Pokémon need adjacent water; Rock/Ground Pokémon need dirt, soil, sand or mountain terrain; other Pokémon use grassland/forest. Happy residents produce two units every 90 seconds: Bug → thread, Flying → feathers, Grass → berries, Rock/Ground → stone, others → fiber. You can place six residents and must retain at least one party member. Their native Pokémon data is stored in reserved slots at the end of Box 14; those slots cannot be withdrawn through Storage while occupied by residents.

## Current limits

- **192 changed tiles**, including harvested terrain, crops and buildings. A refused placement consumes no supplies. Returning a tile to its generated state reclaims the entry.
- Coordinates are bounded to approximately ±8,192 tiles. The explored map records the central 32×32 chunks. This is not an infinite world.
- Five biomes and 22 encounter species; five wild actors around the player. The sprite pack covers Gen I and six later evolutions from the encounter families. This does not make the full Pokédex obtainable.
- No breeding/eggs, weather, caves/dungeons, bosses, progression ranks, complete crafting catalogue or wilderness Pokédex/evolution guide yet. Homes use individual tiles rather than the desktop game's complete building system.
- Battles, summaries, some menus and audio retain the engine's presentation. Encounter/field-skill rules are a limited adaptation.
- One saved world. New World replaces the previous world only when you save it. Format/generator version 3 rejects older saves.
- Tested with mGBA 0.10.2. Android/RetroArch, real hardware and flashcarts remain untested. Running still has occasional longer frames; see [validation](docs/VALIDATION.md).

## Build from source

For development only:

```sh
sudo apt-get install build-essential git gcc-arm-none-eabi binutils-arm-none-eabi libnewlib-arm-none-eabi libnewlib-dev libpng-dev pkg-config
bash scripts/build-gba.sh ../engine
```

The script verifies engine commit `e8bd1cd7b03fc032ea37e3ecd38b379b5d01a1e7`, applies this repository's overlay and builds `build/Pokewilds-GBA-0.3.gba`. `TOOLCHAIN` overrides `/usr`; `JOBS` defaults to 2. The tested toolchain was ARM GCC 13.2.1.

Packed assets are included. To regenerate them, run `python3 tools/import_assets.py /path/to/pokewilds.jar` with Pillow installed. See [asset attribution](assets/ASSET-SOURCES.md). The supplied desktop archive is not included.

`make test` checks the portable world core. To exercise the compiled ROM, install `libmgba-dev` and Pillow, then:

```sh
mkdir -p validation
cc -shared -fPIC tools/emulator_bridge.c -o validation/libemulator.so -lmgba
python3 tools/check_rebuild.py
python3 tools/check_combat.py
python3 tools/check_session.py
```

These tests drive ordinary controller input and observe memory without modifying it. Disposable saves and screenshots are written under `validation03/`. No base-ROM BPS patch is supplied; this download is the compiled ROM.
