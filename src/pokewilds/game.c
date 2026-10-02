#include "pokewilds/game.h"
#include <string.h>

static int16_t FloorDiv16(int32_t n){ if(n>=0) return (int16_t)(n/16); return (int16_t)(-(((-n)+15)/16)); }
static uint8_t Local16(int32_t n){ int32_t r=n%16; if(r<0) r+=16; return (uint8_t)r; }

void PwGame_Refresh(struct PwGame *g){
 int16_t cx=FloorDiv16(g->player.worldX),cy=FloorDiv16(g->player.worldY); int y,x;
 g->centerChunkX=cx; g->centerChunkY=cy;
 for(y=-PW_ACTIVE_RADIUS;y<=PW_ACTIVE_RADIUS;y++) for(x=-PW_ACTIVE_RADIUS;x<=PW_ACTIVE_RADIUS;x++)
  PwWorld_GenerateChunk(&g->active[y+PW_ACTIVE_RADIUS][x+PW_ACTIVE_RADIUS],cx+x,cy+y);
}
void PwGame_New(struct PwGame *g,uint32_t seed){memset(g,0,sizeof(*g));PwWorld_Init(seed);PwGame_Refresh(g);}
uint8_t PwGame_TileAt(const struct PwGame *g,int32_t wx,int32_t wy){
 int16_t cx=FloorDiv16(wx),cy=FloorDiv16(wy); int ix=cx-g->centerChunkX+PW_ACTIVE_RADIUS,iy=cy-g->centerChunkY+PW_ACTIVE_RADIUS;
 if(ix>=0&&ix<PW_ACTIVE_DIAMETER&&iy>=0&&iy<PW_ACTIVE_DIAMETER) return g->active[iy][ix].tiles[Local16(wy)][Local16(wx)];
 {struct PwChunk c;PwWorld_GenerateChunk(&c,cx,cy);return c.tiles[Local16(wy)][Local16(wx)];}
}
static int Passable(uint8_t t){return t==PW_TILE_GRASS||t==PW_TILE_TALL_GRASS||t==PW_TILE_SAND;}
int PwGame_Move(struct PwGame *g,enum PwDirection d){int32_t nx=g->player.worldX,ny=g->player.worldY;int16_t oldX=g->centerChunkX,oldY=g->centerChunkY;
 if(d==PW_DIR_NORTH)ny--;else if(d==PW_DIR_SOUTH)ny++;else if(d==PW_DIR_WEST)nx--;else nx++;
 if(!Passable(PwGame_TileAt(g,nx,ny)))return 0;g->player.worldX=nx;g->player.worldY=ny;g->player.steps++;
 if(FloorDiv16(nx)!=oldX||FloorDiv16(ny)!=oldY)PwGame_Refresh(g);return 1;}
int PwGame_GatherFacing(struct PwGame *g,enum PwDirection d){int32_t x=g->player.worldX,y=g->player.worldY;int16_t cx,cy;uint8_t tile;
 if(d==PW_DIR_NORTH)y--;else if(d==PW_DIR_SOUTH)y++;else if(d==PW_DIR_WEST)x--;else x++;tile=PwGame_TileAt(g,x,y);
 if(tile!=PW_TILE_TREE&&tile!=PW_TILE_ROCK)return 0;cx=FloorDiv16(x);cy=FloorDiv16(y);if(!PwWorld_SetTile(cx,cy,Local16(x),Local16(y),PW_TILE_GRASS))return 0;PwGame_Refresh(g);return 1;}
int PwGame_CheckEncounter(const struct PwGame *g,struct PwEncounter *e){uint8_t t=PwGame_TileAt(g,g->player.worldX,g->player.worldY),b=PwWorld_BiomeAt(FloorDiv16(g->player.worldX),FloorDiv16(g->player.worldY));uint32_t r;
 if(t!=PW_TILE_TALL_GRASS||!e)return 0;r=PwWorld_Hash(PwWorld_GetSeed(),g->player.worldX,g->player.worldY,g->player.steps+0xB411u);if((r%100u)>=24u)return 0;
 if(b==PW_BIOME_FOREST){static const uint16_t s[]={10,13,16,43};e->species=s[(r>>8)%4];}else{static const uint16_t s[]={19,21,29,32};e->species=s[(r>>8)%4];}e->level=(uint8_t)(2+(r>>16)%7);return 1;}
