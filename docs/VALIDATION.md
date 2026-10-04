# 0.3 validation

Tested on the compiled GBA ROM with mGBA 0.10.2, ARM GCC 13.2.1 and pokeemerald-expansion 1.17.0 at `e8bd1cd7b03fc032ea37e3ecd38b379b5d01a1e7`.

ROM SHA-256: `890122c0a2aadbbf8ba810159c919030fa8216d93ef3936a6d109a657b839bb3`.

## Results

The three emulator scripts passed **48 checks**. They use ordinary controller input and read memory to observe state. They do not inject Pokémon, change inventory, alter RNG, bypass collisions or fabricate save data. A separate host generator predicts routes for navigation; it cannot modify the running emulator. The camera test reads the engine's scroll-register buffer because GBA hardware scroll registers are write-only.

| Area | Verified on the ROM |
| --- | --- |
| Recruitment | Invite Oddish, Geodude, Caterpie and Pidgey; party counts update |
| Field work | CUT refused without a helper; three trees yield wood; SMASH yields stone |
| Following/skills | Skills list displays current helpers; changing lead retains nighttime palette |
| Habitats | Place Caterpie/Pidgey, wait, collect thread/feathers; save/restart/pick up preserves each Pokémon's identity |
| Farming | DIG a plot, plant a seed, wait two minutes, harvest three berries and two seeds |
| Building | Floor, roof and bed placement; bed ingredients deducted; bed interaction retains full party HP |
| World save | Player location, materials, crops, buildings, party and active habitats survive native flash save/restart |
| Exploration | Travel beyond x = -16 and x = +16, reverse course and return to camp |
| Input | An action pressed for one frame during a step is buffered; 12 one-frame menu open/close cycles; continuous held movement and reversal |
| Battles/capture | Start battle from interaction, throw a ball, catch Caterpie, advance capture EXP/dex/name prompts and return to the same position |
| Storage | Deposit captured Pokémon, save/restart, withdraw the same identity with valid HP |
| Native menus | Summary returns to wilderness; Save and Quit returns to title; Continue restores the captured party |
| Extended session | At least nine simulated world minutes, night terrain tint, readable UI and correctly tinted replacement follower |

Portable tests also pass with warnings treated as errors: repeat generation of 2,025 chunks across 25 seeds; all biomes; negative coordinates; partial chunk generation and rapid reversals; collision and Surf passability; saved edit restoration; 192-edit capacity/reclamation; corrupt/duplicate/out-of-range data rejection; crop countdown across a save/reload; resident timers staying ready through 65,536 further seconds.

## Movement measurements

Of **186 individually observed tile moves**:

- 85 walking moves took 9 hardware frames each, including chunk boundaries.
- 101 running moves took 5–9 hardware frames; 91 took 5.
- No sampled scroll update exceeded 2 pixels while walking or 4 while running. The 0.2 tile-sized camera jump was not observed.
- An idle sample advanced 300 world-loop ticks in 300 hardware frames.

These are emulator samples, not a promise of uninterrupted 60 FPS. Occasional longer running frames remain, especially when other world work coincides with movement. The tests also exercise continuous held movement rather than only separated taps.

## Budgets

| Region | Linker usage | Hardware capacity |
| --- | ---: | ---: |
| EWRAM | 242,888 bytes | 262,144 bytes |
| IWRAM | 28,380 bytes | 32,768 bytes |
| ROM before padding | 26,959,404 bytes | 33,554,432 bytes |
| Wilderness save structure | 1,388 bytes | Inside native SaveBlock3 |
| Active terrain | 9 × 264-byte chunks | Fixed allocation |
| World tile maps | 12,288 bytes | Fixed EWRAM allocation |

The distributed ROM is padded to 32 MiB and uses game code PWGE. Native party, bag, boxes and world data are saved through the 128 KiB flash system. The world block has a separate integrity hash. Save/generator version 3 requires a fresh world.

## Remaining coverage gaps

Only mGBA was tested. Android emulators, RetroArch, real GBA hardware, FPGA and flashcarts have not been checked. Every move/evolution/encounter, a full-party capture sent automatically to a box, interruption during flash writing, every construction piece and ROM-level Surf have not been exhaustively tested. Host Surf passability is covered. No BPS/base-ROM verification was performed.

This release is an unfinished GBA adaptation. The README lists missing desktop systems and the current world, encounter and construction limits. Passing these checks does not mean the complete PokéWilds port is finished or free of bugs.
