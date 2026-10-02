#ifndef GUARD_POKEWILDS_WORLD_H
#define GUARD_POKEWILDS_WORLD_H

#include <stdint.h>

#define PW_CHUNK_SIZE 16
#define PW_ACTIVE_RADIUS 1
#define PW_ACTIVE_DIAMETER (PW_ACTIVE_RADIUS * 2 + 1)
#define PW_MAX_WORLD_EDITS 192

enum PwBiome { PW_BIOME_PLAINS, PW_BIOME_FOREST, PW_BIOME_BEACH, PW_BIOME_OCEAN, PW_BIOME_MOUNTAIN };
enum PwTile { PW_TILE_GRASS, PW_TILE_TALL_GRASS, PW_TILE_TREE, PW_TILE_SAND, PW_TILE_WATER, PW_TILE_ROCK };

struct PwWorldEdit { int16_t chunkX, chunkY; uint8_t localX, localY, tile; };
struct PwSave { uint32_t magic, version, seed; uint16_t editCount; struct PwWorldEdit edits[PW_MAX_WORLD_EDITS]; };
struct PwChunk { int16_t x, y; uint8_t biome; uint8_t tiles[PW_CHUNK_SIZE][PW_CHUNK_SIZE]; };

void PwWorld_Init(uint32_t seed);
uint32_t PwWorld_GetSeed(void);
uint32_t PwWorld_Hash(uint32_t seed, int32_t x, int32_t y, uint32_t salt);
uint8_t PwWorld_BiomeAt(int16_t chunkX, int16_t chunkY);
void PwWorld_GenerateChunk(struct PwChunk *chunk, int16_t chunkX, int16_t chunkY);
int PwWorld_SetTile(int16_t chunkX, int16_t chunkY, uint8_t x, uint8_t y, uint8_t tile);
const struct PwSave *PwWorld_GetSave(void);
int PwWorld_Load(const struct PwSave *save);

#endif
