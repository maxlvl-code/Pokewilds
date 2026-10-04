#include "pokewilds/game.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
int main(void) {
    struct PwGame g;
    struct PwSave save;
    int i;
    PwGame_New(&g, 0x00483729);
    assert(PwGame_Passable(PwGame_TileAt(&g, 0, 0)));
    for (i = -50; i <= 50; i++)
        assert(PwGame_Edit(&g, i, 0, PW_TILE_FLOOR));
    for (i = 0; i < 40; i++)
    {
        assert(PwGame_Move(&g, PW_DIR_WEST));
        PwGame_Stream(&g, 2);
        assert(PwGame_TileAt(&g, g.player.worldX, 0) == PW_TILE_FLOOR);
    }
    assert(g.player.worldX == -40 && g.centerChunkX == -3);
    for (i = 0; i < 80; i++)
    {
        assert(PwGame_Move(&g, PW_DIR_EAST));
        PwGame_Stream(&g, 2);
        assert(PwGame_TileAt(&g, g.player.worldX, 0) == PW_TILE_FLOOR);
    }
    assert(g.player.worldX == 40 && g.centerChunkX == 2);
    /* Abandon partially generated chunks repeatedly, then verify every cell.
     * This catches stale ring slots and edits overwritten by delayed rows. */
    for (i = 0; i < 4; i++) {
        int j;
        for (j = 0; j < 80; j++) {
            assert(PwGame_Move(&g, i % 2 ? PW_DIR_EAST : PW_DIR_WEST));
            PwGame_Stream(&g, 1);
        }
    }
    PwGame_Stream(&g, 144);
    for (int y = -16; y < 32; y++)
        for (int x = 16; x < 64; x++)
            assert(PwGame_TileAt(&g, x, y) == PwWorld_TileAt(x, y));
    assert(PwGame_Edit(&g, 41, 0, PW_TILE_WATER));
    assert(!PwGame_Move(&g, PW_DIR_EAST));
    assert(PwGame_MoveWithSurf(&g, PW_DIR_EAST, 1));
    assert(!PwGame_Move(&g, (enum PwDirection)255));
    PwGame_New(&g, 11);
    assert(PwGame_Move(&g, PW_DIR_WEST));
    assert(PwGame_Move(&g, PW_DIR_WEST));
    assert(PwGame_Move(&g, PW_DIR_NORTH));
    assert(!PwGame_Move(&g, PW_DIR_WEST));
    assert(PwGame_GatherFacing(&g, PW_DIR_WEST) == PW_TILE_TREE + 1);
    assert(PwWorld_GetSave()->wood == 3);
    assert(PwGame_Move(&g, PW_DIR_WEST));
    assert(PwGame_TileAt(&g, -3, -1) == PW_TILE_GRASS);
    PwWorld_Seal(PwWorld_MutableSave());
    save = *PwWorld_GetSave();
    PwGame_New(&g, 99);
    assert(PwWorld_Load(&save));
    PwGame_Restore(&g);
    assert(g.player.worldX == -3 && g.player.worldY == -1 && PwWorld_GetSave()->wood == 3);
    assert(PwGame_TileAt(&g, -3, -1) == PW_TILE_GRASS);
    puts("Game checks passed: movement across positive/negative chunk seams, collision, gathering and "
         "persisted player/resources.");
    return 0;
}
