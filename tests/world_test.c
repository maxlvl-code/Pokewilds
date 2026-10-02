#include "pokewilds/world.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

int main(void)
{
    struct PwChunk a,b,c;
    struct PwSave snapshot;
    PwWorld_Init(0x12345678u);
    PwWorld_GenerateChunk(&a,7,-3);
    PwWorld_GenerateChunk(&b,7,-3);
    assert(memcmp(&a,&b,sizeof(a))==0);
    assert(PwWorld_SetTile(7,-3,4,9,PW_TILE_SAND));
    PwWorld_GenerateChunk(&c,7,-3);
    assert(c.tiles[9][4]==PW_TILE_SAND);
    snapshot=*PwWorld_GetSave();
    PwWorld_Init(99);
    assert(PwWorld_Load(&snapshot));
    PwWorld_GenerateChunk(&c,7,-3);
    assert(c.tiles[9][4]==PW_TILE_SAND);
    puts("PokéWilds world-core tests passed.");
    return 0;
}
