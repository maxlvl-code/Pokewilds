#include "pokewilds/world.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
int main(void) {
    struct PwChunk a, b;
    struct PwSave copy;
    unsigned i;
    int x, y;
    unsigned biomes = 0;
    assert(sizeof(struct PwWorldEdit) == 6);
    assert(sizeof(struct PwSave) <= 1500);
    assert(PwFloorDiv(-1, 16) == -1 && PwFloorDiv(-16, 16) == -1 && PwFloorDiv(-17, 16) == -2);
    for (i = 0; i < 25; i++) {
        PwWorld_Init(i * 374761393u);
        for (x = -4; x <= 4; x++)
            for (y = -4; y <= 4; y++) {
                PwWorld_GenerateChunk(&a, x, y);
                PwWorld_GenerateChunk(&b, x, y);
                assert(!memcmp(&a, &b, sizeof(a)));
                biomes |= 1u << a.biome;
                for (int ly = 0; ly < 16; ly++)
                    for (int lx = 0; lx < 16; lx++)
                        assert(a.tiles[ly][lx] == PwWorld_BaseTile(x * 16 + lx, y * 16 + ly));
            }
    }
    assert(biomes == (1u << PW_BIOME_COUNT) - 1);
    PwWorld_Init(0);
    assert(PwWorld_GetSeed() == 0);
    assert(PwWorld_SetAt(-17, -1, PW_TILE_FLOOR));
    PwWorld_GenerateChunk(&a, -2, -1);
    assert(a.tiles[15][15] == PW_TILE_FLOOR);
    PwWorld_Seal(PwWorld_MutableSave());
    copy = *PwWorld_GetSave();
    assert(PwWorld_CheckIntegrity(&copy));
    PwWorld_Init(42);
    assert(PwWorld_Load(&copy));
    PwWorld_GenerateChunk(&a, -2, -1);
    assert(a.tiles[15][15] == PW_TILE_FLOOR);
    copy.edits[0].tile = 255;
    assert(!PwWorld_Load(&copy));
    assert(!PwWorld_CheckIntegrity(&copy));
    assert(!PwWorld_SetAt(1, 1, 255));
    assert(!PwWorld_SetTile(0, 0, 16, 0, PW_TILE_GRASS));
    PwWorld_Init(12345);
    for (i = 0; i < PW_MAX_WORLD_EDITS; i++)
        assert(PwWorld_SetAt(100 + i, 100, PW_TILE_FLOOR));
    assert(!PwWorld_SetAt(500, 100, PW_TILE_FLOOR));
    assert(PwWorld_GetSave()->editCount == 192);
    assert(PwWorld_SetAt(100, 100, PW_TILE_WALL));
    assert(PwWorld_GetSave()->editCount == 192);
    assert(PwWorld_SetAt(100, 100, PwWorld_BaseTile(100, 100)));
    assert(PwWorld_GetSave()->editCount == 191);
    assert(PwWorld_SetAt(500, 100, PW_TILE_FLOOR));
    assert(PwWorld_GetSave()->editCount == 192);
    PwWorld_Seal(PwWorld_MutableSave());
    copy = *PwWorld_GetSave();
    copy.edits[1] = copy.edits[0];
    assert(!PwWorld_Validate(&copy));
    assert(!PwWorld_SetAt(PW_WORLD_LIMIT, 0, PW_TILE_GRASS));
    PwWorld_Init(42);
    assert(PwWorld_SetAt(-17, -2, PW_TILE_SPROUT));
    PwWorld_MutableSave()->residents[0] = (struct PwResident){1, 2, 90, 1, 0};
    for (i = 0; i < 60; i++) assert(!PwWorld_Tick());
    PwWorld_Seal(PwWorld_MutableSave());
    copy = *PwWorld_GetSave();
    assert(copy.residents[0].cooldown == 30);
    PwWorld_Init(0);
    assert(PwWorld_Load(&copy));
    for (i = 0; i < 59; i++) assert(!PwWorld_Tick());
    assert(PwWorld_TileAt(-17, -2) == PW_TILE_SPROUT);
    assert(PwWorld_Tick());
    assert(PwWorld_TileAt(-17, -2) == PW_TILE_BERRY);
    assert(PwWorld_GetSave()->residents[0].cooldown == 0);
    /* Mature materials stay ready, including after the old 16-bit time wrap. */
    for (i = 0; i < 65536; i++) PwWorld_Tick();
    assert(PwWorld_GetSave()->residents[0].cooldown == 0);
    copy = *PwWorld_GetSave();
    copy.residents[1] = copy.residents[0];
    assert(!PwWorld_Validate(&copy));
    copy.residents[1].active = 0;
    copy.residents[0].cooldown = 91;
    assert(!PwWorld_Validate(&copy));
    printf("World checks passed: 2025 chunks, all biomes, negative coordinates, edit capacity/reclamation, "
           "corruption checks. Save=%zu bytes.\n",
           sizeof(struct PwSave));
    return 0;
}
