#include "pokewilds/game.h"
#include <string.h>
static int Mod(int n, int d) {
    int r = n % d;
    return r < 0 ? r + d : r;
}
void PwGame_Refresh(struct PwGame *g) {
    int x, y;
    g->centerChunkX = PwFloorDiv(g->player.worldX, 16);
    g->centerChunkY = PwFloorDiv(g->player.worldY, 16);
    for (y = g->centerChunkY - 1; y <= g->centerChunkY + 1; y++)
        for (x = g->centerChunkX - 1; x <= g->centerChunkX + 1; x++) {
            struct PwChunk *c = &g->active[Mod(y, 3)][Mod(x, 3)];
            if (!c->valid || c->x != x || c->y != y)
                PwWorld_GenerateChunk(c, x, y);
        }
}
void PwGame_Restore(struct PwGame *g) {
    const struct PwSave *s = PwWorld_GetSave();
    memset(g, 0, sizeof(*g));
    g->player.worldX = s->playerX;
    g->player.worldY = s->playerY;
    g->player.steps = s->steps;
    PwGame_Refresh(g);
}
void PwGame_New(struct PwGame *g, uint32_t seed) {
    PwWorld_Init(seed);
    PwGame_Restore(g);
}
/* Fill at most the requested number of rows. A boundary no longer generates
 * three complete chunks inside the input handler. Partially filled slots are
 * never exposed as valid terrain. */
void PwGame_Stream(struct PwGame *g, unsigned rows) {
    while (rows--) {
        struct PwChunk *c;
        if (g->streaming && (g->streamX < g->centerChunkX - 1 || g->streamX > g->centerChunkX + 1 ||
            g->streamY < g->centerChunkY - 1 || g->streamY > g->centerChunkY + 1)) g->streaming = 0;
        if (!g->streaming) {
            int x, y, best = 99;
            for (y = g->centerChunkY - 1; y <= g->centerChunkY + 1; y++)
                for (x = g->centerChunkX - 1; x <= g->centerChunkX + 1; x++) {
                    int dx = x - g->centerChunkX, dy = y - g->centerChunkY;
                    int distance = (dx < 0 ? -dx : dx) + (dy < 0 ? -dy : dy);
                    c = &g->active[Mod(y, 3)][Mod(x, 3)];
                    if ((!c->valid || c->x != x || c->y != y) && distance < best) {
                        best = distance; g->streamX = x; g->streamY = y;
                    }
                }
            if (best == 99) return;
            c = &g->active[Mod(g->streamY, 3)][Mod(g->streamX, 3)];
            memset(c, 0, sizeof(*c));
            c->x = g->streamX; c->y = g->streamY;
            c->biome = PwWorld_BiomeAt(c->x, c->y);
            g->streamRow = 0; g->streaming = 1;
        }
        c = &g->active[Mod(g->streamY, 3)][Mod(g->streamX, 3)];
        PwWorld_GenerateRow(c, g->streamRow++);
        if (g->streamRow == 16) {
            PwWorld_ApplyEdits(c); c->valid = 1; g->streaming = 0;
        }
    }
}
uint8_t PwGame_TileAt(const struct PwGame *g, int32_t x, int32_t y) {
    int cx = PwFloorDiv(x, 16), cy = PwFloorDiv(y, 16);
    const struct PwChunk *c = &g->active[Mod(cy, 3)][Mod(cx, 3)];
    if (c->valid && c->x == cx && c->y == cy)
        return c->tiles[(unsigned)y & 15][(unsigned)x & 15];
    return PwWorld_TileAt(x, y);
}
int PwGame_Passable(uint8_t t) {
    return t == PW_TILE_GRASS || t == PW_TILE_TALL_GRASS || t == PW_TILE_SAND || t == PW_TILE_DIRT ||
           t == PW_TILE_FLOWER || t == PW_TILE_FLOOR || t == PW_TILE_DOOR || t == PW_TILE_BRIDGE ||
           t == PW_TILE_SOIL || t == PW_TILE_SPROUT || t == PW_TILE_ROOF;
}
int PwGame_Move(struct PwGame *g, enum PwDirection d) { return PwGame_MoveWithSurf(g, d, 0); }
int PwGame_MoveWithSurf(struct PwGame *g, enum PwDirection d, int surf) {
    static const int dx[] = {0, 0, -1, 1}, dy[] = {-1, 1, 0, 0};
    struct PwSave *s = PwWorld_MutableSave();
    int32_t x, y;
    if (d > PW_DIR_EAST)
        return 0;
    s->facing = d;
    x = g->player.worldX + dx[d];
    y = g->player.worldY + dy[d];
    if (x <= -PW_WORLD_LIMIT || x >= PW_WORLD_LIMIT - 1 || y <= -PW_WORLD_LIMIT || y >= PW_WORLD_LIMIT - 1 ||
        (!PwGame_Passable(PwGame_TileAt(g, x, y)) && !(surf && PwGame_TileAt(g, x, y) == PW_TILE_WATER)))
        return 0;
    g->player.worldX = x;
    g->player.worldY = y;
    g->player.steps++;
    s->playerX = x;
    s->playerY = y;
    s->steps = g->player.steps;
    PwWorld_Discover(x, y);
    g->centerChunkX = PwFloorDiv(x, 16);
    g->centerChunkY = PwFloorDiv(y, 16);
    return 1;
}
int PwGame_Edit(struct PwGame *g, int32_t x, int32_t y, uint8_t t) {
    int cx = PwFloorDiv(x, 16), cy = PwFloorDiv(y, 16);
    struct PwChunk *c = &g->active[Mod(cy, 3)][Mod(cx, 3)];
    if (!PwWorld_SetAt(x, y, t))
        return 0;
    if (c->valid && c->x == cx && c->y == cy)
        c->tiles[Mod(y, 16)][Mod(x, 16)] = t;
    return 1;
}
int PwGame_GatherFacing(struct PwGame *g, enum PwDirection d) {
    static const int dx[] = {0, 0, -1, 1}, dy[] = {-1, 1, 0, 0};
    int32_t x, y;
    uint8_t t;
    struct PwSave *s = PwWorld_MutableSave();
    uint16_t *res;
    unsigned amt;
    if (d > PW_DIR_EAST)
        return 0;
    x = g->player.worldX + dx[d];
    y = g->player.worldY + dy[d];
    t = PwGame_TileAt(g, x, y);
    if (t == PW_TILE_TREE) {
        res = &s->wood;
        amt = 3;
    } else if (t == PW_TILE_ROCK) {
        res = &s->stone;
        amt = 2;
    } else if (t == PW_TILE_TALL_GRASS || t == PW_TILE_FLOWER) {
        res = &s->fiber;
        amt = 2;
    } else
        return 0;
    if (*res > 65535 - amt)
        return -2;
    if (!PwGame_Edit(g, x, y, PW_TILE_GRASS))
        return -1;
    *res += amt;
    return t + 1;
}
int PwGame_CheckEncounter(const struct PwGame *g, struct PwEncounter *e) {
    uint32_t r;
    if (!e || PwGame_TileAt(g, g->player.worldX, g->player.worldY) != PW_TILE_TALL_GRASS)
        return 0;
    r = PwWorld_Hash(PwWorld_GetSeed(), g->player.worldX, g->player.worldY, g->player.steps + 0xB411);
    if (r % 100 >= 7)
        return 0;
    {
        static const uint16_t species[] = {10, 13, 16, 19, 25, 43};
        e->species = species[(r >> 8) % 6];
        e->level = 2 + (r >> 16) % 4;
    }
    return 1;
}
