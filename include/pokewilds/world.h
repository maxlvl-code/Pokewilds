#ifndef GUARD_POKEWILDS_WORLD_H
#define GUARD_POKEWILDS_WORLD_H
#include <stdint.h>
#define PW_CHUNK_SIZE 16
#define PW_ACTIVE_RADIUS 1
#define PW_ACTIVE_DIAMETER 3
#define PW_MAX_WORLD_EDITS 192
#define PW_WORLD_LIMIT 8192
#define PW_SAVE_MAGIC 0x50574742u
#define PW_SAVE_VERSION 3u
#define PW_GENERATOR_VERSION 3u
#define PW_RESIDENTS 6
enum PwBiome {
    PW_BIOME_PLAINS,
    PW_BIOME_FOREST,
    PW_BIOME_BEACH,
    PW_BIOME_OCEAN,
    PW_BIOME_MOUNTAIN,
    PW_BIOME_COUNT
};
enum PwTile {
    PW_TILE_GRASS,
    PW_TILE_TALL_GRASS,
    PW_TILE_TREE,
    PW_TILE_SAND,
    PW_TILE_WATER,
    PW_TILE_ROCK,
    PW_TILE_FLOWER,
    PW_TILE_DIRT,
    PW_TILE_FLOOR,
    PW_TILE_WALL,
    PW_TILE_DOOR,
    PW_TILE_FIRE,
    PW_TILE_BED,
    PW_TILE_BRIDGE,
    PW_TILE_FENCE,
    PW_TILE_SOIL,
    PW_TILE_SPROUT,
    PW_TILE_BERRY,
    PW_TILE_ROOF,
    PW_TILE_COUNT
};
/* Six-byte edits, explicit widths, no pointers in flash. */
struct __attribute__((packed)) PwWorldEdit {
    int16_t x, y;
    uint8_t tile, reserved;
};
struct PwResident {
    int16_t x, y;
    uint16_t cooldown;
    uint8_t active, reserved;
};
struct PwSave {
    uint32_t magic, version, seed, generatorVersion;
    int16_t playerX, playerY, campX, campY;
    uint32_t steps;
    uint16_t editCount, wood, stone, fiber;
    uint8_t facing, music, visited[128];
    struct PwWorldEdit edits[PW_MAX_WORLD_EDITS];
    uint32_t seconds;
    uint16_t seeds, berries, thread, feathers;
    struct PwResident residents[PW_RESIDENTS];
    uint8_t follower, starterFriends, reserved[2];
    uint32_t integrity;
};
struct PwChunk {
    int16_t x, y;
    uint8_t biome, valid;
    uint8_t tiles[16][16];
};
int32_t PwFloorDiv(int32_t n, int32_t d);
uint32_t PwWorld_Hash(uint32_t seed, int32_t x, int32_t y, uint32_t salt);
void PwWorld_BindSave(struct PwSave *save);
void PwWorld_Init(uint32_t seed);
uint32_t PwWorld_GetSeed(void);
uint8_t PwWorld_BiomeAt(int16_t chunkX, int16_t chunkY);
uint8_t PwWorld_BiomeAtTile(int32_t x, int32_t y);
uint8_t PwWorld_BaseTile(int32_t x, int32_t y);
void PwWorld_GenerateChunk(struct PwChunk *chunk, int16_t chunkX, int16_t chunkY);
void PwWorld_GenerateRow(struct PwChunk *chunk, uint8_t row);
void PwWorld_ApplyEdits(struct PwChunk *chunk);
uint8_t PwWorld_TileAt(int32_t x, int32_t y);
int PwWorld_Tick(void);
int PwWorld_SetTile(int16_t chunkX, int16_t chunkY, uint8_t x, uint8_t y, uint8_t tile);
int PwWorld_SetAt(int32_t x, int32_t y, uint8_t tile);
const struct PwSave *PwWorld_GetSave(void);
struct PwSave *PwWorld_MutableSave(void);
void PwWorld_Seal(struct PwSave *save);
int PwWorld_CheckIntegrity(const struct PwSave *save);
int PwWorld_Validate(const struct PwSave *save);
int PwWorld_Load(const struct PwSave *save);
void PwWorld_Discover(int32_t x, int32_t y);
int PwWorld_IsDiscovered(int16_t cx, int16_t cy);
#endif
