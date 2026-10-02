#include "pokewilds/game.h"
#include <assert.h>
#include <stdio.h>
int main(void){struct PwGame g;uint8_t before;PwGame_New(&g,12345);assert(g.centerChunkX==0&&g.centerChunkY==0);before=PwGame_TileAt(&g,0,0);assert(before<=PW_TILE_ROCK);g.player.worldX=15;g.player.worldY=0;PwGame_Refresh(&g);(void)PwGame_Move(&g,PW_DIR_EAST);assert(g.centerChunkX>=0);puts("game-loop tests passed");return 0;}
