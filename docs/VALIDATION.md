# 0.3.1 validation

Tested on the compiled GBA ROM with mGBA 0.10.2, ARM GCC 13.2.1 and pokeemerald-expansion 1.17.0 at `e8bd1cd7b03fc032ea37e3ecd38b379b5d01a1e7`.

ROM SHA-256: `c89f580e077a9d91b55fe0040bfebf4b1849565138395b088913067c26e4dc8c`.

## Results

The four emulator scripts passed **105 checks**: 56 control/compatibility checks, 28 gameplay checks, 11 battle/storage checks and 10 extended-session checks. They use ordinary controller input and read memory to observe state. They do not inject Pokémon, change inventory, alter RNG, bypass collisions or fabricate save data. A separate host generator predicts routes for navigation; it cannot modify the running emulator. The camera test reads the engine's scroll-register buffer because GBA hardware scroll registers are write-only.

| Area | Verified on the ROM |
| --- | --- |
| 0.3 compatibility | Load a native 0.3 flash-save fixture; preserve the party, use the new menus, save and reload exact Pokémon identities |
| Menu navigation | All ordinary submenus restore their parent and selection; direct map closes to world; skills A/R work; message dismissal blocks hidden actions; held directions do not scroll a newly opened menu |
| Seed and placement | Seed digits wrap independently; short taps turn without walking; habitat preview cancels without party changes and confirms exactly one placement |
| Wild stability | More than 48 steps without visible actor teleports; unrecruited helpers remain at camp through exploration, distant save/reload and return |
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
| Storage | Direction plus A uses the new slot; deposit/withdraw preserves identity; depositing the lead updates the follower graphics |
| Native menus | Summary returns to the selected party member; three Save and Quit/Continue cycles preserve the captured party and keep the title responsive |
| Extended session | At least nine simulated world minutes, night terrain tint, readable UI and correctly tinted replacement follower |

`check_controls.py` uses `tests/fixtures/v03-camp.sav.gz`, a synthetic save produced with normal play in the released 0.3 ROM. It contains no user progress. The remaining scripts create disposable saves.

Portable tests also pass with warnings treated as errors: repeat generation of 2,025 chunks across 25 seeds; all biomes; negative coordinates; partial chunk generation and rapid reversals; collision and Surf passability; saved edit restoration; 192-edit capacity/reclamation; corrupt/duplicate/out-of-range data rejection; crop countdown across a save/reload; resident timers staying ready through 65,536 further seconds.

## Movement measurements

Of **194 individually observed tile moves**:

- 93 walking moves took 9–16 hardware frames from direction press to completion; 38 took 9.
- 101 running moves took 5–12 hardware frames; 61 took 5.
- These totals include the new five-frame turn-in-place delay when changing direction from a standstill, plus occasional rendering or actor waits. They are not a direct speed comparison with the 0.3 measurements.
- No sampled camera update exceeded 2 pixels while walking or 4 while running. No tile-sized camera jumps were observed.
- Continuous held movement and reversal both passed. An idle sample advanced 298 world-loop ticks in 300 hardware frames.

These are emulator samples, not a promise of uninterrupted 60 FPS. Occasional longer frames remain. The controls test also observes visible actors every frame during its walking sequences.

## Budgets

| Region | Linker usage | Hardware capacity |
| --- | ---: | ---: |
| EWRAM | 242,912 bytes | 262,144 bytes |
| IWRAM | 28,380 bytes | 32,768 bytes |
| ROM before padding | 26,962,336 bytes | 33,554,432 bytes |
| Wilderness save structure | 1,388 bytes | Inside native SaveBlock3 |
| Active terrain | 9 × 264-byte chunks | Fixed allocation |
| World tile maps | 12,288 bytes | Fixed EWRAM allocation |

The distributed ROM is padded to 32 MiB and uses game code PWGE. Native party, bag, boxes and world data are saved through the 128 KiB flash system. The world block has a separate integrity hash. Save/generator version 3 is unchanged from 0.3; its native flash saves remain compatible. Saves from 0.2 and emulator save states from other builds are not supported.

## Remaining coverage gaps

Only mGBA was tested. Android emulators, RetroArch, real GBA hardware, FPGA and flashcarts have not been checked. Every move/evolution/encounter, a full-party capture sent automatically to a box, interruption during flash writing, every construction piece and ROM-level Surf have not been exhaustively tested. Host Surf passability is covered. No BPS/base-ROM verification was performed.

This release is an unfinished GBA adaptation. The README lists missing desktop systems and the current world, encounter and construction limits. Passing these checks does not mean the complete PokéWilds port is finished or free of bugs.
