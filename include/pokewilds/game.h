#ifndef GUARD_POKEWILDS_GAME_H
#define GUARD_POKEWILDS_GAME_H
#include <stdint.h>
#include "pokewilds/world.h"

enum PwDirection { PW_DIR_NORTH, PW_DIR_SOUTH, PW_DIR_WEST, PW_DIR_EAST };
struct PwPlayer { int32_t worldX, worldY; uint16_t steps; };
struct PwEncounter { uint16_t species; uint8_t level; };
struct PwGame { struct PwPlayer player; struct PwChunk active[PW_ACTIVE_DIAMETER][PW_ACTIVE_DIAMETER]; int16_t centerChunkX, centerChunkY; };

void PwGame_New(struct PwGame *game, uint32_t seed);
void PwGame_Refresh(struct PwGame *game);
int PwGame_Move(struct PwGame *game, enum PwDirection direction);
int PwGame_GatherFacing(struct PwGame *game, enum PwDirection direction);
int PwGame_CheckEncounter(const struct PwGame *game, struct PwEncounter *encounter);
uint8_t PwGame_TileAt(const struct PwGame *game, int32_t worldX, int32_t worldY);
#endif
