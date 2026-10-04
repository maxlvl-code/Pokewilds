#include "pokewilds/world.h"
#include <string.h>
#ifndef __arm__
static struct PwSave sLocalSave;
static struct PwSave *sSave = &sLocalSave;
#else
static struct PwSave *sSave;
#endif
int32_t PwFloorDiv(int32_t n, int32_t d) {
    if (d == 16)
        return n >= 0 ? n >> 4 : -(int32_t)(((0u - (uint32_t)n) + 15u) >> 4);
    int32_t q = n / d;
    return q - ((n % d) < 0);
}

static uint32_t Mix(uint32_t x) {
    x ^= x >> 16;
    x *= 0x7feb352du;
    x ^= x >> 15;
    x *= 0x846ca68bu;
    return x ^ (x >> 16);
}
uint32_t PwWorld_Hash(uint32_t s, int32_t x, int32_t y, uint32_t salt) {
    return Mix(s ^ Mix((uint32_t)x) ^ (Mix((uint32_t)y) << 1) ^ salt);
}
void PwWorld_BindSave(struct PwSave *save) { sSave = save; }
void PwWorld_Init(uint32_t seed) {
    memset(sSave, 0, sizeof(*sSave));
    sSave->magic = PW_SAVE_MAGIC;
    sSave->version = PW_SAVE_VERSION;
    sSave->generatorVersion = PW_GENERATOR_VERSION;
    sSave->seed = seed;
    sSave->campY = 3;
    sSave->facing = 1;
    sSave->music = 1;
    sSave->seeds = 3;
    sSave->berries = 3;
    PwWorld_Discover(0, 0);
}
uint32_t PwWorld_GetSeed(void) { return sSave->seed; }
/* Integer coherent noise in absolute coordinates. Floor division is essential west/north of zero. */
static int Noise(int32_t x, int32_t y, int scale, uint32_t salt) {
    int32_t cx, cy, fx, fy, n;
    if (scale == 8) {
        cx = x >= 0 ? x >> 3 : -((7 - x) >> 3);
        cy = y >= 0 ? y >> 3 : -((7 - y) >> 3);
    } else if (scale == 32) {
        cx = x >= 0 ? x >> 5 : -((31 - x) >> 5);
        cy = y >= 0 ? y >> 5 : -((31 - y) >> 5);
    } else {
        cx = PwFloorDiv(x, 48);
        cy = PwFloorDiv(y, 48);
    }
    fx = x - cx * scale;
    fy = y - cy * scale;
    int a = PwWorld_Hash(sSave->seed, cx, cy, salt) & 255;
    int b = PwWorld_Hash(sSave->seed, cx + 1, cy, salt) & 255;
    int c = PwWorld_Hash(sSave->seed, cx, cy + 1, salt) & 255;
    int d = PwWorld_Hash(sSave->seed, cx + 1, cy + 1, salt) & 255;
    n = (a * (scale - fx) + b * fx) * (scale - fy) + (c * (scale - fx) + d * fx) * fy;
    return scale == 8 ? n >> 6 : scale == 32 ? n >> 10 : n / 2304;
}
uint8_t PwWorld_BiomeAtTile(int32_t x, int32_t y) {
    if (x > -15 && x < 15 && y > -15 && y < 15)
        return (x < -5 || y < -6) ? PW_BIOME_FOREST : PW_BIOME_PLAINS;
    int e = Noise(x, y, 32, 0xE1), m = Noise(x, y, 48, 0xA7);
    if (e < 86)
        return PW_BIOME_OCEAN;
    if (e < 105)
        return PW_BIOME_BEACH;
    if (e > 183)
        return PW_BIOME_MOUNTAIN;
    if (m > 134)
        return PW_BIOME_FOREST;
    return PW_BIOME_PLAINS;
}
uint8_t PwWorld_BiomeAt(int16_t x, int16_t y) {
    return PwWorld_BiomeAtTile((int32_t)x * 16 + 8, (int32_t)y * 16 + 8);
}
uint8_t PwWorld_BaseTile(int32_t x, int32_t y) {
    uint32_t r = PwWorld_Hash(sSave->seed, x, y, 0xC31);
    int patch;
    uint8_t b;
    if (x <= -PW_WORLD_LIMIT || x >= PW_WORLD_LIMIT - 1 || y <= -PW_WORLD_LIMIT || y >= PW_WORLD_LIMIT - 1)
        return PW_TILE_ROCK;
    if (x == 0 && y == 3)
        return PW_TILE_FIRE;
    if ((x == -3 && y == -1) || (x == 3 && y == -1) || (x == -4 && y == 1))
        return PW_TILE_TREE;
    if ((x == 2 && y == 2) || (x == 4 && y == 2))
        return PW_TILE_ROCK;
    if ((x == -3 && y == 2) || (x == 4 && y == 3) || (x == 3 && y == -3) || (x == -4 && y == -3))
        return PW_TILE_GRASS;
    if (x >= -2 && x <= 2 && y >= -2 && y <= 3)
        return PW_TILE_GRASS;
    if (x >= 5 && x <= 9 && y >= -2 && y <= 2)
        return PW_TILE_TALL_GRASS;
    b = PwWorld_BiomeAtTile(x, y);
    if (b == PW_BIOME_OCEAN)
        return PW_TILE_WATER;
    if (b == PW_BIOME_BEACH)
        return r % 41 == 0 ? PW_TILE_ROCK : PW_TILE_SAND;
    if (b == PW_BIOME_MOUNTAIN)
        return r % 100 < 27 ? PW_TILE_ROCK : PW_TILE_DIRT;
    patch = Noise(x, y, 8, 0xF02);
    if (b == PW_BIOME_FOREST && patch > 101 && r % 100 < 36)
        return PW_TILE_TREE;
    if (b == PW_BIOME_PLAINS && r % 100 < 4)
        return PW_TILE_TREE;
    if (r % 100 == 83)
        return PW_TILE_ROCK;
    if (patch > 135 && r % 100 < 79)
        return PW_TILE_TALL_GRASS;
    if (r % 100 < 7)
        return PW_TILE_FLOWER;
    return PW_TILE_GRASS;
}
void PwWorld_GenerateRow(struct PwChunk *c, uint8_t y) {
    int x;
    for (x = 0; x < 16; x++)
        c->tiles[y][x] = PwWorld_BaseTile(c->x * 16 + x, c->y * 16 + y);
}
void PwWorld_ApplyEdits(struct PwChunk *c) {
    unsigned i;
    for (i = 0; i < sSave->editCount; i++) {
        int lx = sSave->edits[i].x - c->x * 16, ly = sSave->edits[i].y - c->y * 16;
        if (lx >= 0 && lx < 16 && ly >= 0 && ly < 16)
            c->tiles[ly][lx] = sSave->edits[i].tile;
    }
}
void PwWorld_GenerateChunk(struct PwChunk *c, int16_t cx, int16_t cy) {
    int y;
    memset(c, 0, sizeof(*c));
    c->x = cx; c->y = cy; c->biome = PwWorld_BiomeAt(cx, cy);
    for (y = 0; y < 16; y++) PwWorld_GenerateRow(c, y);
    PwWorld_ApplyEdits(c);
    c->valid = 1;
}
uint8_t PwWorld_TileAt(int32_t x, int32_t y) {
    unsigned i;
    for (i = 0; i < sSave->editCount; i++)
        if (sSave->edits[i].x == x && sSave->edits[i].y == y) return sSave->edits[i].tile;
    return PwWorld_BaseTile(x, y);
}
int PwWorld_Tick(void) {
    unsigned i;
    int changed = 0;
    sSave->seconds++;
    for (i = 0; i < PW_RESIDENTS; i++)
        if (sSave->residents[i].active && sSave->residents[i].cooldown) sSave->residents[i].cooldown--;
    for (i = 0; i < sSave->editCount; i++) {
        struct PwWorldEdit *e = &sSave->edits[i];
        if (e->tile != PW_TILE_SPROUT) continue;
        if (e->reserved) e->reserved--;
        if (!e->reserved) {e->tile = PW_TILE_BERRY; changed = 1;}
    }
    return changed;
}
int PwWorld_SetAt(int32_t x, int32_t y, uint8_t tile) {
    int i;
    uint8_t base;
    if (x <= -PW_WORLD_LIMIT || x >= PW_WORLD_LIMIT - 1 || y <= -PW_WORLD_LIMIT || y >= PW_WORLD_LIMIT - 1 ||
        tile >= PW_TILE_COUNT)
        return 0;
    base = PwWorld_BaseTile(x, y);
    for (i = 0; i < sSave->editCount; i++)
        if (sSave->edits[i].x == x && sSave->edits[i].y == y) {
            if (tile == base) {
                sSave->edits[i] = sSave->edits[--sSave->editCount];
                memset(&sSave->edits[sSave->editCount], 0, sizeof(sSave->edits[0]));
            } else {
                sSave->edits[i].tile = tile;
                sSave->edits[i].reserved = tile == PW_TILE_SPROUT ? 120 : 0;
            }
            return 1;
        }
    if (tile == base)
        return 1;
    if (sSave->editCount >= PW_MAX_WORLD_EDITS)
        return 0;
    sSave->edits[sSave->editCount++] = (struct PwWorldEdit){(int16_t)x, (int16_t)y, tile, tile == PW_TILE_SPROUT ? 120 : 0};
    return 1;
}
int PwWorld_SetTile(int16_t cx, int16_t cy, uint8_t x, uint8_t y, uint8_t t) {
    if (x >= 16 || y >= 16)
        return 0;
    return PwWorld_SetAt((int32_t)cx * 16 + x, (int32_t)cy * 16 + y, t);
}
const struct PwSave *PwWorld_GetSave(void) { return sSave; }
struct PwSave *PwWorld_MutableSave(void) { return sSave; }
int PwWorld_Validate(const struct PwSave *s) {
    unsigned i, j;
    if (!s || s->magic != PW_SAVE_MAGIC || s->version != PW_SAVE_VERSION ||
        s->generatorVersion != PW_GENERATOR_VERSION || s->editCount > PW_MAX_WORLD_EDITS || s->facing > 3 ||
        s->music > 1 || s->follower >= 6)
        return 0;
    if (s->playerX <= -PW_WORLD_LIMIT || s->playerX >= PW_WORLD_LIMIT - 1 || s->playerY <= -PW_WORLD_LIMIT ||
        s->playerY >= PW_WORLD_LIMIT - 1)
        return 0;
    for (i = 0; i < s->editCount; i++) {
        const struct PwWorldEdit *e = &s->edits[i];
        if (e->x <= -PW_WORLD_LIMIT || e->x >= PW_WORLD_LIMIT - 1 || e->y <= -PW_WORLD_LIMIT ||
            e->y >= PW_WORLD_LIMIT - 1 || e->tile >= PW_TILE_COUNT ||
            (e->tile == PW_TILE_SPROUT ? e->reserved > 120 : e->reserved != 0))
            return 0;
        for (j = 0; j < i; j++)
            if (e->x == s->edits[j].x && e->y == s->edits[j].y)
                return 0;
    }
    for (i = 0; i < PW_RESIDENTS; i++) {
        const struct PwResident *r = &s->residents[i];
        if (r->active > 1 || r->reserved || r->cooldown > 90) return 0;
        if (r->active && (r->x <= -PW_WORLD_LIMIT || r->x >= PW_WORLD_LIMIT - 1 ||
            r->y <= -PW_WORLD_LIMIT || r->y >= PW_WORLD_LIMIT - 1)) return 0;
        for (j = 0; r->active && j < i; j++)
            if (s->residents[j].active && r->x == s->residents[j].x && r->y == s->residents[j].y) return 0;
    }
    return 1;
}
int PwWorld_Load(const struct PwSave *s) {
    if (!PwWorld_Validate(s))
        return 0;
    *sSave = *s;
    return 1;
}
void PwWorld_Discover(int32_t x, int32_t y) {
    int cx = PwFloorDiv(x, 16) + 16, cy = PwFloorDiv(y, 16) + 16, n = cy * 32 + cx;
    if (cx >= 0 && cx < 32 && cy >= 0 && cy < 32)
        sSave->visited[n / 8] |= 1u << (n % 8);
}
int PwWorld_IsDiscovered(int16_t cx, int16_t cy) {
    int n;
    cx += 16;
    cy += 16;
    if (cx < 0 || cx >= 32 || cy < 0 || cy >= 32)
        return 0;
    n = cy * 32 + cx;
    return !!(sSave->visited[n / 8] & (1u << (n % 8)));
}

/* SaveBlock3 occupies flash-sector slack outside the engine's sector checksum.
 * Protect it independently and reject incomplete or corrupted world data. */
static uint32_t Integrity(const struct PwSave *s) {
    const uint8_t *p = (const uint8_t *)s;
    uint32_t h = 2166136261u;
    unsigned i;
    for (i = 0; i < sizeof(*s) - sizeof(s->integrity); i++)
        h = (h ^ p[i]) * 16777619u;
    return h;
}
void PwWorld_Seal(struct PwSave *s) { s->integrity = Integrity(s); }
int PwWorld_CheckIntegrity(const struct PwSave *s) { return s && s->integrity == Integrity(s); }
