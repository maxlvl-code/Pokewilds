#include "pokewilds/game.h"
#include <stdio.h>
#include <stdlib.h>

static char Glyph(uint8_t t){static const char g[]=".\"T~^#";return t<6?g[t]:'?';}
static void Draw(struct PwGame *game){int y,x;puts("\nPokéWilds GBA Prototype - portable gameplay harness");printf("Seed %u | Position %ld,%ld | Steps %u\n",PwWorld_GetSeed(),(long)game->player.worldX,(long)game->player.worldY,game->player.steps);for(y=-8;y<=8;y++){for(x=-16;x<=16;x++){if(!x&&!y)putchar('@');else putchar(Glyph(PwGame_TileAt(game,game->player.worldX+x,game->player.worldY+y)));}putchar('\n');}puts("WASD move | IJKL gather N/W/S/E | Q quit");}
int main(int argc,char **argv){struct PwGame game;unsigned seed=argc>1?(unsigned)strtoul(argv[1],0,10):12345;int c;PwGame_New(&game,seed);for(;;){Draw(&game);c=getchar();while(c=='\n'||c=='\r')c=getchar();if(c=='q'||c=='Q')break;if(c=='w')PwGame_Move(&game,PW_DIR_NORTH);else if(c=='s')PwGame_Move(&game,PW_DIR_SOUTH);else if(c=='a')PwGame_Move(&game,PW_DIR_WEST);else if(c=='d')PwGame_Move(&game,PW_DIR_EAST);else if(c=='i')PwGame_GatherFacing(&game,PW_DIR_NORTH);else if(c=='k')PwGame_GatherFacing(&game,PW_DIR_SOUTH);else if(c=='j')PwGame_GatherFacing(&game,PW_DIR_WEST);else if(c=='l')PwGame_GatherFacing(&game,PW_DIR_EAST);{struct PwEncounter e;if(PwGame_CheckEncounter(&game,&e))printf("\nWild species #%u appeared at Lv.%u!\n",e.species,e.level);}}return 0;}
