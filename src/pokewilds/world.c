#include "pokewilds/world.h"
#include <string.h>

#define PW_SAVE_MAGIC 0x50574742u /* PWGB */
#define PW_SAVE_VERSION 1u

static struct PwSave sSave;

static uint32_t Mix(uint32_t x)
{
    x ^= x >> 16; x *= 0x7feb352du; x ^= x >> 15; x *= 0x846ca68bu; x ^= x >> 16;
    return x;
}

uint32_t PwWorld_Hash(uint32_t seed, int32_t x, int32_t y, uint32_t salt)
{
    return Mix(seed ^ Mix((uint32_t)x) ^ (Mix((uint32_t)y) << 1) ^ salt);
}

void PwWorld_Init(uint32_t seed)
{
    memset(&sSave, 0, sizeof(sSave));
    sSave.magic = PW_SAVE_MAGIC; sSave.version = PW_SAVE_VERSION; sSave.seed = seed ? seed : 1u;
}

uint32_t PwWorld_GetSeed(void) { return sSave.seed; }

uint8_t PwWorld_BiomeAt(int16_t cx, int16_t cy)
{
    uint32_t elevation = PwWorld_Hash(sSave.seed, cx / 3, cy / 3, 0xE1u) & 255u;
    uint32_t moisture  = PwWorld_Hash(sSave.seed, cx / 4, cy / 4, 0xA7u) & 255u;
    if (elevation < 42) return PW_BIOME_OCEAN;
    if (elevation < 62) return PW_BIOME_BEACH;
    if (elevation > 215) return PW_BIOME_MOUNTAIN;
    if (moisture > 145) return PW_BIOME_FOREST;
    return PW_BIOME_PLAINS;
}

static uint8_t BaseTile(uint8_t biome, uint32_t r)
{
    switch (biome) {
    case PW_BIOME_OCEAN: return PW_TILE_WATER;
    case PW_BIOME_BEACH: return (r % 19u == 0) ? PW_TILE_ROCK : PW_TILE_SAND;
    case PW_BIOME_MOUNTAIN: return (r % 5u) ? PW_TILE_ROCK : PW_TILE_GRASS;
    case PW_BIOME_FOREST: return (r % 100u < 34u) ? PW_TILE_TREE : ((r % 100u < 56u) ? PW_TILE_TALL_GRASS : PW_TILE_GRASS);
    default: return (r % 100u < 18u) ? PW_TILE_TALL_GRASS : ((r % 100u == 99u) ? PW_TILE_TREE : PW_TILE_GRASS);
    }
}

void PwWorld_GenerateChunk(struct PwChunk *c, int16_t cx, int16_t cy)
{
    uint8_t x,y; uint16_t i;
    c->x=cx; c->y=cy; c->biome=PwWorld_BiomeAt(cx,cy);
    for(y=0;y<PW_CHUNK_SIZE;y++) for(x=0;x<PW_CHUNK_SIZE;x++)
        c->tiles[y][x]=BaseTile(c->biome,PwWorld_Hash(sSave.seed,cx*PW_CHUNK_SIZE+x,cy*PW_CHUNK_SIZE+y,0xC31u));
    for(i=0;i<sSave.editCount;i++) if(sSave.edits[i].chunkX==cx && sSave.edits[i].chunkY==cy)
        c->tiles[sSave.edits[i].localY][sSave.edits[i].localX]=sSave.edits[i].tile;
}

int PwWorld_SetTile(int16_t cx,int16_t cy,uint8_t x,uint8_t y,uint8_t tile)
{
    uint16_t i;
    if(x>=PW_CHUNK_SIZE||y>=PW_CHUNK_SIZE) return 0;
    for(i=0;i<sSave.editCount;i++) if(sSave.edits[i].chunkX==cx&&sSave.edits[i].chunkY==cy&&sSave.edits[i].localX==x&&sSave.edits[i].localY==y){sSave.edits[i].tile=tile;return 1;}
    if(sSave.editCount>=PW_MAX_WORLD_EDITS) return 0;
    sSave.edits[sSave.editCount++]=(struct PwWorldEdit){cx,cy,x,y,tile}; return 1;
}

const struct PwSave *PwWorld_GetSave(void){return &sSave;}
int PwWorld_Load(const struct PwSave *save){if(!save||save->magic!=PW_SAVE_MAGIC||save->version!=PW_SAVE_VERSION||save->editCount>PW_MAX_WORLD_EDITS)return 0;sSave=*save;return 1;}
