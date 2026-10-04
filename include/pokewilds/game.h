#ifndef GUARD_POKEWILDS_GAME_H
#define GUARD_POKEWILDS_GAME_H
#include "pokewilds/world.h"
enum PwDirection { PW_DIR_NORTH, PW_DIR_SOUTH, PW_DIR_WEST, PW_DIR_EAST };
struct PwPlayer {
    int32_t worldX, worldY;
    uint32_t steps;
};
struct PwEncounter {
    uint16_t species;
    uint8_t level;
};
struct PwGame {
    struct PwPlayer player;
    struct PwChunk active[3][3];
    int16_t centerChunkX, centerChunkY;
    int16_t streamX, streamY;
    uint8_t streamRow, streaming;
};
void PwGame_New(struct PwGame *game, uint32_t seed);
void PwGame_Restore(struct PwGame *game);
void PwGame_Refresh(struct PwGame *game);
void PwGame_Stream(struct PwGame *game, unsigned rows);
int PwGame_Move(struct PwGame *game, enum PwDirection direction);
int PwGame_MoveWithSurf(struct PwGame *game, enum PwDirection direction, int surf);
int PwGame_GatherFacing(struct PwGame *game, enum PwDirection direction);
int PwGame_CheckEncounter(const struct PwGame *game, struct PwEncounter *encounter);
uint8_t PwGame_TileAt(const struct PwGame *game, int32_t x, int32_t y);
int PwGame_Passable(uint8_t tile);
int PwGame_Edit(struct PwGame *game, int32_t x, int32_t y, uint8_t tile);
#endif
