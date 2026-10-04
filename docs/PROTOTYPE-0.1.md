# Prototype 0.1 — Seeded Wilderness Core

Historical notes. The 0.3 wilderness runtime supersedes this integration plan; see the root README and VALIDATION.md.

This branch starts with the system that determines whether a PokéWilds-style GBA demake is viable.

## Implemented
- 32-bit world seed
- deterministic 16×16 chunk generation
- plains, forest, beach, ocean and mountain biome selection
- deterministic terrain/resource placement
- sparse persistent tile modifications
- save validation/versioning
- host-side repeatability + save/load tests

## Next integration step
Vendor/pin pokeemerald-expansion, adapt the chunk output into an Emerald metatile staging map, then connect player chunk transitions and encounter tables. The world core intentionally has no heap allocation and uses fixed-width data structures suitable for GBA integration.

## Persistence budget
Prototype 0.1 reserves 192 sparse edits. This is deliberately conservative until the expansion save layout is integrated and measured. Later revisions should compact edits per chunk and reclaim unused Emerald save systems.

## Acceptance test
Run `make test` on a normal C toolchain. It verifies deterministic regeneration and persistence across save reload.
