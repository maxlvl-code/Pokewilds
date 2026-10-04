#include "global.h"
#include "pokewilds/runtime.h"
#include "battle.h"
#include "battle_main.h"
#include "bg.h"
#include "constants/flags.h"
#include "constants/items.h"
#include "constants/map_groups.h"
#include "constants/moves.h"
#include "constants/pokemon.h"
#include "constants/rgb.h"
#include "constants/songs.h"
#include "constants/species.h"
#include "event_data.h"
#include "gpu_regs.h"
#include "item.h"
#include "load_save.h"
#include "main.h"
#include "malloc.h"
#include "menu.h"
#include "new_game.h"
#include "overworld.h"
#include "palette.h"
#include "play_time.h"
#include "pokedex.h"
#include "pokemon.h"
#include "pokemon_storage_system.h"
#include "pokemon_summary_screen.h"
#include "pokewilds/game.h"
#include "pokewilds/species_ids.h"
#include "pokewilds/terrain_ids.h"
#include "random.h"
#include "save.h"
#include "scanline_effect.h"
#include "script_pokemon_util.h"
#include "sound.h"
#include "sprite.h"
#include "string_util.h"
#include "task.h"
#include "text.h"
#include "window.h"
#include "util.h"
#include <stdarg.h>

#define PARTY gParties[B_TRAINER_PLAYER]
#define PARTY_COUNT gPartiesCount[B_TRAINER_PLAYER]
#define WORLD (gSaveBlock3Ptr->wilds)
#define ACTORS 5
#define PLAYER_TAG 3000
#define CURSOR_TAG 3040
#define RESIDENT_BOX 13
#define RESIDENT_SLOT 24
#define PARTNER_SLOT (ACTORS + PW_RESIDENTS + 1)
static const u8 sTerrain[] = INCBIN_U8("graphics/pokewilds/terrain.8bpp");
static const u16 sTerrainPal[] = INCBIN_U16("graphics/pokewilds/terrain.gbapal");
static const u8 sActorsGfx[] = INCBIN_U8("graphics/pokewilds/actors.4bpp");
static const u16 sActorsPal[] = INCBIN_U16("graphics/pokewilds/actors.gbapal");
static const struct BgTemplate sBgs[] = {
    {.bg = 0, .charBaseIndex = 2, .mapBaseIndex = 31, .screenSize = 0, .paletteMode = 0, .priority = 0},
    {.bg = 1, .charBaseIndex = 0, .mapBaseIndex = 26, .screenSize = 1, .paletteMode = 1, .priority = 1},
    {.bg = 2, .charBaseIndex = 0, .mapBaseIndex = 28, .screenSize = 1, .paletteMode = 1, .priority = 2},
};
static const struct WindowTemplate sWindows[] = {
    {.bg = 0, .tilemapLeft = 0, .tilemapTop = 0, .width = 30, .height = 20, .paletteNum = 15, .baseBlock = 1},
    DUMMY_WIN_TEMPLATE};
static const struct OamData sOam = {.shape = SPRITE_SHAPE(16x16), .size = SPRITE_SIZE(16x16), .priority = 2};
static const struct SpriteTemplate sSprite = {.tileTag = PLAYER_TAG,
                                              .paletteTag = PLAYER_TAG,
                                              .oam = &sOam,
                                              .anims = gDummySpriteAnimTable,
                                              .images = NULL,
                                              .affineAnims = gDummySpriteAffineAnimTable,
                                              .callback = SpriteCallbackDummy};
static const u16 sUiPalette[16] = {RGB(0, 0, 0),    RGB(27, 29, 24), RGB(3, 7, 5), RGB(22, 25, 20),
                                   RGB(24, 26, 14), RGB(19, 25, 15), RGB(23, 27, 19), RGB(9, 17, 24),
                                   RGB(24, 21, 13), RGB(17, 17, 19), RGB(11, 22, 11), RGB(6, 15, 7),
                                   RGB(31, 27, 9)};
static const u8 sTextColors[] = {0, 2, 3};
static const char *const sBiomes[] = {"GRASSLAND", "FOREST", "BEACH", "OCEAN", "MOUNTAIN"};
static const char *const sBuildNames[] = {"FLOOR", "WALL", "DOOR", "CAMPFIRE", "BED", "BRIDGE", "FENCE", "ROOF"};
static const u8 sBuildTiles[] = {PW_TILE_FLOOR, PW_TILE_WALL,   PW_TILE_DOOR, PW_TILE_FIRE,
                                 PW_TILE_BED,   PW_TILE_BRIDGE, PW_TILE_FENCE, PW_TILE_ROOF};
static const u8 sCosts[][3] = {{2, 0, 0}, {4, 0, 0}, {3, 0, 0}, {3, 3, 0}, {6, 0, 0}, {3, 0, 0}, {2, 0, 0}, {3, 0, 0}};
static const int sDx[] = {0, 0, -1, 1}, sDy[] = {-1, 1, 0, 0};
enum {
    UI_TITLE,
    UI_SEED,
    UI_WORLD,
    UI_MENU,
    UI_CRAFT,
    UI_PARTY,
    UI_MAP,
    UI_HELP,
    UI_BUILD,
    UI_BAG,
    UI_OPTIONS,
    UI_STORAGE,
    UI_SAVE, UI_WILD, UI_RESIDENT, UI_FIELD
};
struct WildActor {
    s16 x, y;
    u16 species;
    u8 level, active, sprite, asset, direction, friendly;
    s16 offsetX, offsetY;
    u8 moving, friendId, delay, reserved;
};
EWRAM_DATA static struct PwGame sGame = {0};
EWRAM_DATA static struct WildActor sWild[ACTORS] = {0};
EWRAM_DATA static u8 sResidentSprites[PW_RESIDENTS] = {0};
EWRAM_DATA static u16 sResidentSpecies[PW_RESIDENTS] = {0};
EWRAM_DATA static u8 sPartnerSprite = 0, sTalkActor = 0, sTalkResident = 0;
EWRAM_DATA static s16 sFollowX = 0, sFollowY = 0, sFollowOffsetX = 0, sFollowOffsetY = 0;
EWRAM_DATA static u8 sFollowDir = 0;
static int FindWorker(int skill);
static int ResidentAt(int x, int y);
static void ReloadActorGfx(void);
static void ReloadPartner(void);
static void SetUi(u8 ui);
static void DropPartner(void);
static void ResidentAction(void);
static void WildAction(void);
static void Wander(void);
static void TickWorld(void);
static void ApplyLighting(void);
static bool8 CanUse(u16 species, int skill);
static bool8 HabitatHappy(unsigned slot);
static bool8 HasType(u16 species, u8 type);
enum { FIELD_CUT, FIELD_SMASH, FIELD_BUILD, FIELD_DIG, FIELD_SURF };
static const char *const sSkillNames[] = {"CUT", "SMASH", "BUILD", "DIG", "SURF"};
EWRAM_DATA static u16 sMaps[3][2048] = {0};
EWRAM_DATA static u8 sCursorGfx[128] = {0};
EWRAM_DATA static char sNotice[72] = {0};
EWRAM_DATA static u32 sFrames = 0, sSeed = 0;
EWRAM_DATA static s16 sScrollX = 0, sScrollY = 0, sCursorX = 0, sCursorY = 0;
EWRAM_DATA static u16 sNoticeFrames = 0, sEncounterCooldown = 0;
EWRAM_DATA static u8 sUi = 0, sSelection = 0, sHasSave = 0, sPlayerSprite = 0, sCursorSprite = 0, sBuild = 0,
                     sDigit = 0, sMoveFrames = 0, sMoveSpeed = 0, sBox = 0, sBoxSlot = 0, sPartySlot = 0;
EWRAM_DATA static bool8 sUiDirty = FALSE, sWorldDirty = FALSE, sBattleReturning = FALSE,
                        sSaveAndTitle = FALSE;
EWRAM_DATA static s16 sRenderX = 0, sRenderY = 0;
EWRAM_DATA static bool8 sRenderValid = FALSE;
EWRAM_DATA static volatile u16 sPressed = 0, sRepeated = 0, sHeld = 0;
EWRAM_DATA static u16 sLastHeld = 0, sPendingKeys = 0;
EWRAM_DATA static u8 sKeyDelay = 0, sSecondFrames = 0;
static void ApplyCamera(void);
static void DrawCell(int x, int y);
static void MainLoop(void);
static void InitScreen(void);
static void DrawUI(void);
static void DrawWorld(void);
static void UpdateSprites(void);
static void SpawnActors(void);
static void BeginBattle(u16 species, u8 level);

/* Bounded, integer-only UI formatting. Avoid newlib stdio and its heap/syscall dependencies. */
static void PwFormat(char *dst, unsigned size, const char *fmt, ...) {
    va_list ap;
    unsigned used = 0;
    va_start(ap, fmt);
    while (*fmt) {
        char tmp[40];
        const char *part = tmp;
        unsigned len = 0, width = 0;
        char pad = ' ';
        if (*fmt != '%') {
            tmp[len++] = *fmt++;
        } else {
            fmt++;
            if (*fmt == '0') {
                pad = '0';
                fmt++;
            }
            while (*fmt >= '0' && *fmt <= '9') {
                width = width * 10 + *fmt++ - '0';
            }
            if (*fmt == 's') {
                part = va_arg(ap, const char *);
                while (part[len])
                    len++;
            } else {
                unsigned value, base = *fmt == 'X' ? 16 : 10;
                char rev[32];
                unsigned n = 0;
                bool8 negative = FALSE;
                if (*fmt == 'd') {
                    int v = va_arg(ap, int);
                    negative = v < 0;
                    value = negative ? 0u - (unsigned)v : (unsigned)v;
                } else
                    value = va_arg(ap, unsigned);
                do {
                    rev[n++] = "0123456789ABCDEF"[value % base];
                    value /= base;
                } while (value);
                if (negative)
                    tmp[len++] = '-';
                while (width > n + len && len < 20)
                    tmp[len++] = pad;
                while (n)
                    tmp[len++] = rev[--n];
            }
            if (*fmt)
                fmt++;
        }
        for (unsigned i = 0; i < len; i++) {
            if (used + 1 < size)
                dst[used] = part[i];
            used++;
        }
    }
    if (size)
        dst[used < size ? used : size - 1] = 0;
    va_end(ap);
}

static int Abs(int n) { return n < 0 ? -n : n; }
static void Text(int x, int y, const char *str) {
    u8 out[128];
    int i;
    for (i = 0; str[i] && i < 126; i++) {
        unsigned char c = str[i];
        if (c >= 'A' && c <= 'Z')
            out[i] = 0xBB + c - 'A';
        else if (c >= 'a' && c <= 'z')
            out[i] = 0xD5 + c - 'a';
        else if (c >= '0' && c <= '9')
            out[i] = 0xA1 + c - '0';
        else
            switch (c) {
            case ' ':
                out[i] = 0;
                break;
            case '!':
                out[i] = 0xAB;
                break;
            case '?':
                out[i] = 0xAC;
                break;
            case '.':
                out[i] = 0xAD;
                break;
            case '-':
                out[i] = 0xAE;
                break;
            case ':':
                out[i] = 0xF0;
                break;
            case '/':
                out[i] = 0xBA;
                break;
            case '+':
                out[i] = 0x2E;
                break;
            case '>':
                out[i] = 0x86;
                break;
            default:
                out[i] = 0;
            }
    }
    out[i] = EOS;
    AddTextPrinterParameterized4(0, FONT_SMALL, x, y, 0, 0, sTextColors, TEXT_SKIP_DRAW, out);
}
static void EncodedText(int x, int y, const u8 *str) {
    AddTextPrinterParameterized4(0, FONT_SMALL, x, y, 0, 0, sTextColors, TEXT_SKIP_DRAW, str);
}
static void Notice(const char *str) {
    PwFormat(sNotice, sizeof(sNotice), "%s", str);
    sNoticeFrames = 150;
    sUiDirty = TRUE;
}
static void VBlank(void) {
    u16 held = REG_KEYINPUT ^ KEYS_MASK;
    u16 fresh = held & ~sLastHeld;
    sPressed |= fresh;
    sRepeated |= fresh;
    if (held != sLastHeld) sKeyDelay = 20;
    else if (held && sKeyDelay && !--sKeyDelay) {
        sRepeated |= held & (DPAD_UP | DPAD_DOWN | DPAD_LEFT | DPAD_RIGHT);
        sKeyDelay = 7;
    }
    sHeld = sLastHeld = held;
    LoadOam();
    ProcessSpriteCopyRequests();
    TransferPlttBuffer();
}
static u8 AssetId(u16 species) {
    unsigned i;
    for (i = 1; i < ARRAY_COUNT(sActorSpecies); i++)
        if (sActorSpecies[i] == species)
            return i;
    return 1;
}
static void MakeActorSprite(int slot, u8 asset) {
    struct SpriteSheet sheet;
    struct SpritePalette pal;
    struct SpriteTemplate t = sSprite;
    u16 tag = PLAYER_TAG + slot;
    sheet = (struct SpriteSheet){sActorsGfx + asset * 1024, 1024, tag};
    pal = (struct SpritePalette){sActorsPal + asset * 16, tag};
    LoadSpriteSheet(&sheet);
    LoadSpritePalette(&pal);
    t.tileTag = tag;
    t.paletteTag = tag;
    if (slot == 0)
        sPlayerSprite = CreateSprite(&t, 120, 80, 0);
    else if (slot <= ACTORS)
        sWild[slot - 1].sprite = CreateSprite(&t, 0, 0, slot);
    else if (slot < PARTNER_SLOT)
        sResidentSprites[slot - ACTORS - 1] = CreateSprite(&t, 0, 0, slot);
    else
        sPartnerSprite = CreateSprite(&t, 0, 0, slot);
}
static void InitScreen(void) {
    int i;
    struct SpriteSheet sheet;
    struct SpritePalette pal;
    struct SpriteTemplate t = sSprite;
    SetVBlankCallback(NULL);
    SetHBlankCallback(NULL);
    SetGpuReg(REG_OFFSET_DISPCNT, DISPCNT_FORCED_BLANK);
    ScanlineEffect_Stop();
    FreeAllWindowBuffers();
    ResetTasks();
    ResetSpriteData();
    FreeAllSpritePalettes();
    ResetPaletteFade();
    ResetBgsAndClearDma3BusyFlags(0);
    CpuFill32(0, (void *)VRAM, VRAM_SIZE);
    CpuFill32(0, (void *)OAM, OAM_SIZE);
    memset(sMaps, 0, sizeof(sMaps));
    InitBgsFromTemplates(0, sBgs, ARRAY_COUNT(sBgs));
    for (i = 0; i < 3; i++)
        SetBgTilemapBuffer(i, sMaps[i]);
    LoadBgTiles(1, sTerrain, sizeof(sTerrain), 0);
    LoadPalette(sTerrainPal, 0, sizeof(sTerrainPal));
    LoadPalette(sUiPalette, 240, 32);
    InitWindows(sWindows);
    DeactivateAllTextPrinters();
    FillWindowPixelBuffer(0, 0);
    PutWindowTilemap(0);
    MakeActorSprite(0, 0);
    for (i = 0; i < ACTORS; i++)
        MakeActorSprite(i + 1, sWild[i].asset ? sWild[i].asset : 1);
    for (i = 0; i < PW_RESIDENTS; i++) {
        sResidentSpecies[i] = WORLD.residents[i].active ? GetBoxMonData(GetBoxedMonPtr(RESIDENT_BOX, RESIDENT_SLOT + i), MON_DATA_SPECIES) : 0;
        MakeActorSprite(ACTORS + 1 + i, AssetId(sResidentSpecies[i]));
    }
    if (WORLD.follower >= PARTY_COUNT) WORLD.follower = 0;
    MakeActorSprite(PARTNER_SLOT, AssetId(PARTY_COUNT ? GetMonData(&PARTY[WORLD.follower], MON_DATA_SPECIES) : SPECIES_MACHOP));
    memset(sCursorGfx, 0, sizeof(sCursorGfx));
    for (i = 0; i < 256; i++) {
        int x = i % 16, y = i / 16;
        if (x == 0 || x == 15 || y == 0 || y == 15) {
            int n = ((y / 8) * 2 + x / 8) * 64 + (y % 8) * 8 + x % 8;
            sCursorGfx[n / 2] |= 1 << ((n % 2) * 4);
        }
    }
    sheet = (struct SpriteSheet){sCursorGfx, 128, CURSOR_TAG};
    pal = (struct SpritePalette){sUiPalette + 11, CURSOR_TAG};
    /* A complete palette is required even though only index 1 is used. */
    {
        static const u16 cursorPal[16] = {0, RGB(31, 27, 9)};
        pal.data = cursorPal;
        LoadSpritePalette(&pal);
    }
    LoadSpriteSheet(&sheet);
    t.tileTag = CURSOR_TAG;
    t.paletteTag = CURSOR_TAG;
    sCursorSprite = CreateSprite(&t, 120, 96, 0);
    gSprites[sCursorSprite].oam.priority = 0;
    sPressed = sRepeated = sPendingKeys = 0;
    sHeld = sLastHeld = REG_KEYINPUT ^ KEYS_MASK;
    sRenderValid = FALSE;
    ApplyCamera();
    SetGpuReg(REG_OFFSET_BLDCNT, 0);
    SetGpuReg(REG_OFFSET_BLDALPHA, 0);
    SetGpuReg(REG_OFFSET_BLDY, 0);
    SetGpuReg(REG_OFFSET_WININ, 0);
    SetGpuReg(REG_OFFSET_WINOUT, 0);
    ShowBg(0);
    ShowBg(1);
    ShowBg(2);
    SetGpuReg(REG_OFFSET_DISPCNT, DISPCNT_MODE_0 | DISPCNT_OBJ_1D_MAP | DISPCNT_BG0_ON | DISPCNT_BG1_ON |
                                      DISPCNT_BG2_ON | DISPCNT_OBJ_ON);
    DrawWorld();
    DrawUI();
    UpdateSprites();
    BuildOamBuffer();
    ApplyLighting();
    SetVBlankCallback(VBlank);
    SetMainCallback2(MainLoop);
}
/* Absolute tile coordinates wrap through the hardware tilemap. Scrolling never
 * recenters/rebuilds the map under a stationary camera. Only entering edges need
 * refreshing; edits request one bounded visible-region refresh. */
static void PutTile(int bg, int x, int y, u16 value) {
    x &= 63; y &= 31;
    sMaps[bg][(x >> 5) * 1024 + y * 32 + (x & 31)] = value;
}
static void Metatile(int bg, int x, int y, u16 tile, int height) {
    int tx, ty;
    for (ty = 0; ty < height; ty++)
        for (tx = 0; tx < 2; tx++)
            PutTile(bg, x * 2 + tx, y * 2 + ty, tile ? tile + ty * 2 + tx : 0);
}
static void ApplyCamera(void) {
    u32 x = ((u32)(WORLD.playerX * 16 - 112 + sScrollX) & 511) * 256;
    u32 y = ((u32)(WORLD.playerY * 16 - 72 + sScrollY) & 255) * 256;
    ChangeBgX(1, x, BG_COORD_SET); ChangeBgX(2, x, BG_COORD_SET);
    ChangeBgY(1, y, BG_COORD_SET); ChangeBgY(2, y, BG_COORD_SET);
}
static void DrawCell(int x, int y) {
    u8 tile = PwGame_TileAt(&sGame, x, y), below = PwGame_TileAt(&sGame, x, y + 1);
    u16 ground = PW_GFX_PLAIN, top = 0;
    switch (tile) {
    case PW_TILE_GRASS: ground = PwWorld_Hash(WORLD.seed, x, y, 6) % 5 ? PW_GFX_PLAIN : PW_GFX_SPECKLE; break;
    case PW_TILE_TALL_GRASS: ground = PW_GFX_TALL; break;
    case PW_TILE_TREE: ground = PW_GFX_TREE_BASE; break;
    case PW_TILE_SAND: ground = PW_GFX_SAND; break;
    case PW_TILE_WATER: {
        u8 mask = (PwGame_TileAt(&sGame,x,y-1) != PW_TILE_WATER) |
            ((PwGame_TileAt(&sGame,x,y+1) != PW_TILE_WATER) << 1) |
            ((PwGame_TileAt(&sGame,x-1,y) != PW_TILE_WATER) << 2) |
            ((PwGame_TileAt(&sGame,x+1,y) != PW_TILE_WATER) << 3);
        ground = mask ? PW_GFX_SHORE_0 + mask * 4 : PW_GFX_WATER;
        break;
    }
    case PW_TILE_ROCK: ground = PW_GFX_ROCK; break;
    case PW_TILE_FLOWER: ground = PW_GFX_FLOWER; break;
    case PW_TILE_DIRT: ground = PW_GFX_DIRT; break;
    case PW_TILE_FLOOR: ground = PW_GFX_FLOOR; break;
    case PW_TILE_WALL: ground = PW_GFX_WALL; break;
    case PW_TILE_DOOR: ground = PW_GFX_DOOR; break;
    case PW_TILE_FIRE: ground = PW_GFX_FIRE; break;
    case PW_TILE_BED: ground = PW_GFX_BED; break;
    case PW_TILE_BRIDGE: ground = PW_GFX_BRIDGE; break;
    case PW_TILE_FENCE: ground = PW_GFX_FENCE; break;
    case PW_TILE_SOIL: ground = PW_GFX_SOIL; break;
    case PW_TILE_SPROUT: ground = PW_GFX_SPROUT; break;
    case PW_TILE_BERRY: ground = PW_GFX_BERRY; break;
    case PW_TILE_ROOF:
        ground = PW_GFX_FLOOR;
        if (sUi == UI_BUILD || PwGame_TileAt(&sGame,WORLD.playerX,WORLD.playerY) != PW_TILE_ROOF)
            top = PW_GFX_ROOF;
        break;
    }
    if (below == PW_TILE_TREE) top = PW_GFX_TREE_TOP;
    if (below == PW_TILE_BED) top = PW_GFX_BED_TOP;
    if (below == PW_TILE_FIRE) top = PW_GFX_FIRE_TOP;
    if (below == PW_TILE_BERRY) top = PW_GFX_BERRY_TOP;
    Metatile(2, x, y, ground, 2);
    Metatile(1, x, y, top, 2);
}
static void DrawWorld(void) {
    int x, y, dx = WORLD.playerX - sRenderX, dy = WORLD.playerY - sRenderY;
    if (sWorldDirty || !sRenderValid || Abs(dx) > 1 || Abs(dy) > 1) {
        for (y = WORLD.playerY - 6; y <= WORLD.playerY + 7; y++)
            for (x = WORLD.playerX - 9; x <= WORLD.playerX + 9; x++) DrawCell(x, y);
    } else {
        if (dx) {
            x = WORLD.playerX + (dx > 0 ? 9 : -9);
            for (y = WORLD.playerY - 6; y <= WORLD.playerY + 7; y++) DrawCell(x, y);
        }
        if (dy) {
            y = WORLD.playerY + (dy > 0 ? 7 : -6);
            for (x = WORLD.playerX - 9; x <= WORLD.playerX + 9; x++) DrawCell(x, y);
        }
    }
    sRenderX = WORLD.playerX; sRenderY = WORLD.playerY; sRenderValid = TRUE;
    CopyBgTilemapBufferToVram(1); CopyBgTilemapBufferToVram(2);
    sWorldDirty = FALSE;
}
static void Panel(const char *title) {
    FillWindowPixelBuffer(0, PIXEL_FILL(1));
    FillWindowPixelRect(0, 5, 0, 0, 240, 19);
    FillWindowPixelRect(0, 6, 0, 141, 240, 19);
    Text(8, 2, title);
}
static void Row(int n, int y, const char *str) {
    if (sSelection == n)
        FillWindowPixelRect(0, 5, 5, y, 230, 16);
    Text(10, y, str);
}
static void SpeciesName(int x, int y, u16 species) { EncodedText(x, y, GetSpeciesName(species)); }
static void DrawUI(void) {
    char b[120];
    int i;
    FillWindowPixelBuffer(0, 0);
    if (sUi == UI_WORLD || sUi == UI_BUILD) {
        FillWindowPixelRect(0, 1, 0, 0, 240, 16);
        FillWindowPixelRect(0, 1, 0, 145, 240, 15);
        if (sUi == UI_BUILD) {
            PwFormat(b, sizeof(b), "BUILD %s  W%u S%u F%u", sBuildNames[sBuild], sCosts[sBuild][0],
                     sCosts[sBuild][1], sCosts[sBuild][2]);
            if (sBuildTiles[sBuild]==PW_TILE_BED) PwFormat(b,sizeof(b),"BED W6 THREAD2 FEATHERS2");
            Text(4, 1, b);
            Text(4, 145, "A Place L/R Piece SELECT Cut B Exit");
        } else {
            PwFormat(b,sizeof(b),"%s%u %s W%u S%u",WORLD.seconds%720>=540?"NIGHT":"DAY",
                WORLD.seconds/720+1,sBiomes[PwWorld_BiomeAtTile(WORLD.playerX,WORLD.playerY)],WORLD.wood,WORLD.stone);
            Text(4, 1, b);
            Text(4, 145, "A Interact  R Build  L Skills  START Menu");
        }
    } else if (sUi == UI_TITLE) {
        FillWindowPixelRect(0, 1, 15, 24, 210, 108);
        FillWindowPixelRect(0, 5, 15, 24, 210, 25);
        Text(64, 30, "POKEWILDS GBA");
        Text(50, 55, "WILDERNESS REBUILD 0.3");
        Text(35, 79, sHasSave ? (sSelection == 0 ? "> Continue" : "  Continue") : "  No saved world");
        Text(35, 97, sSelection == 1 ? "> New world" : "  New world");
        Text(35, 115, "A Select");
    } else if (sUi == UI_SEED) {
        Panel("NEW WORLD");
        Text(10, 26, "World seed");
        PwFormat(b, sizeof(b), "%08X", (unsigned)sSeed);
        Text(27, 49, b);
        Text(27 + sDigit * 6, 60, "-");
        Text(10, 82, "Partner: MACHOP / Level 7");
        Text(10, 101, "Left/Right: digit    Up/Down: value");
        Text(10, 117, "R: random seed");
        Text(6, 145, "START Generate   B Back");
    } else if (sUi == UI_MENU) {
        static const char *items[] = {"POKEMON", "BAG / SUPPLIES", "CRAFT",   "BUILD", "WORLD MAP",
                                      "STORAGE", "SAVE WORLD",     "OPTIONS", "HELP"};
        Panel("CAMP MENU");
        for (i = 0; i < 9; i++)
            Row(i, 21 + i * 13, items[i]);
        Text(6, 145, "A Open   B Resume");
    } else if (sUi == UI_CRAFT) {
        Panel("CRAFT");
        PwFormat(b, sizeof(b), "Wood %u  Stone %u  Fiber %u", WORLD.wood, WORLD.stone, WORLD.fiber);
        Text(8, 23, b);
        Row(0, 50, "POKE BALL   5 wood / 2 stone");
        Row(1, 74, "POTION      3 fiber / 1 berry");
        Text(8, 101, "CUT plants. SMASH rocks. Grow berries.");
        Text(6, 145, "A Craft   B Back");
    } else if (sUi == UI_PARTY) {
        u16 selected=GetMonData(&PARTY[sSelection],MON_DATA_SPECIES);
        Panel("POKEMON / PARTNERS");
        for (i=0;i<PARTY_COUNT;i++) {
            unsigned hp=GetMonData(&PARTY[i],MON_DATA_HP),max=GetMonData(&PARTY[i],MON_DATA_MAX_HP);
            if(i==sSelection) FillWindowPixelRect(0,5,5,22+i*16,230,16);
            SpeciesName(10,23+i*16,GetMonData(&PARTY[i],MON_DATA_SPECIES));
            PwFormat(b,sizeof(b),"L%u %u/%u",(unsigned)GetMonData(&PARTY[i],MON_DATA_LEVEL),hp,max);
            Text(116,23+i*16,b);
        }
        Text(8,125,CanUse(selected,FIELD_CUT)?"Field: CUT trees and plants":
            CanUse(selected,FIELD_BUILD)?"Field: BUILD your home":CanUse(selected,FIELD_SMASH)?"Field: SMASH rocks / DIG soil":
            HasType(selected,TYPE_BUG)?"Happy habitat: makes SILKY THREAD":HasType(selected,TYPE_FLYING)?"Happy habitat: makes SOFT FEATHERS":"Place in a habitat to collect materials");
        Text(4,145,"A Info SELECT Lead START Place B Back");
    } else if (sUi == UI_MAP) {
        Panel("WORLD MAP / EXPLORED CHUNKS");
        for (i = 0; i < 32 * 32; i++) {
            int cx = i % 32 - 16, cy = i / 32 - 16;
            if (PwWorld_IsDiscovered(cx, cy)) {
                u8 colors[] = {10, 11, 8, 7, 9};
                FillWindowPixelRect(0, colors[PwWorld_BiomeAt(cx, cy)], 8 + (i % 32) * 3, 29 + (i / 32) * 3,
                                    3, 3);
            }
        }
        {
            int x = PwFloorDiv(WORLD.playerX, 16) + 16, y = PwFloorDiv(WORLD.playerY, 16) + 16;
            if (x >= 0 && x < 32 && y >= 0 && y < 32)
                FillWindowPixelRect(0, 12, 8 + x * 3, 29 + y * 3, 3, 3);
        }
        PwFormat(b, sizeof(b), "X %d / Y %d", WORLD.playerX, WORLD.playerY);
        Text(115, 33, b);
        PwFormat(b, sizeof(b), "Seed %08X", (unsigned)WORLD.seed);
        Text(115, 54, b);
        Text(115, 75, sBiomes[PwWorld_BiomeAtTile(WORLD.playerX, WORLD.playerY)]);
        PwFormat(b, sizeof(b), "Edits %u/%u", WORLD.editCount, PW_MAX_WORLD_EDITS);
        Text(115, 96, b);
        Text(5, 145, "Yellow: you   Each cell: 16 tiles   B Back");
    } else if (sUi == UI_BAG) {
        Panel("BAG / SUPPLIES");
        PwFormat(b, sizeof(b), "POKE BALLS  %u", CountTotalItemQuantityInBag(ITEM_POKE_BALL));
        Text(10, 28, b);
        PwFormat(b, sizeof(b), "POTIONS     %u", CountTotalItemQuantityInBag(ITEM_POTION));
        Text(10, 49, b);
        PwFormat(b, sizeof(b), "WOOD %u / STONE %u / FIBER %u", WORLD.wood, WORLD.stone, WORLD.fiber);
        Text(10, 76, b);
        PwFormat(b,sizeof(b),"SEEDS %u / BERRIES %u",WORLD.seeds,WORLD.berries);Text(10,94,b);
        PwFormat(b,sizeof(b),"THREAD %u / FEATHERS %u",WORLD.thread,WORLD.feathers);Text(10,112,b);
        Text(6,145,"A Potion on lead   B Back");
    } else if (sUi == UI_HELP) {
        Panel("HOW TO PLAY");
        Text(8,23,"Befriend Oddish and Geodude near camp.");
        Text(8,39,"Grass: CUT. Rock: SMASH. Ground: DIG.");
        Text(8,55,"Machop BUILDs with R. L shows skills.");
        Text(8,71,"A on prepared soil plants a seed.");
        Text(8,87,"Party > START places a habitat Pokemon.");
        Text(8,103,"Bug/flying residents give bed materials.");
        Text(8,119,"Beds and campfires heal. Save often.");
        Text(6,145,"D-pad move   B Run / Back");
    } else if (sUi == UI_OPTIONS) {
        Panel("OPTIONS");
        PwFormat(b, sizeof(b), "Music: %s", WORLD.music ? "ON" : "OFF");
        Row(0, 30, b);
        PwFormat(b, sizeof(b), "Battle animations: %s", gSaveBlock2Ptr->optionsBattleSceneOff ? "OFF" : "ON");
        Row(1, 55, b);
        Row(2, 80, "Save and return to title");
        Text(8, 114, "Save changes from the camp menu.");
        Text(5, 145, "A Change / Select   B Back");
    } else if (sUi == UI_STORAGE) {
        Panel("POKEMON STORAGE");
        PwFormat(b, sizeof(b), "BOX %u / SLOT %u", sBox + 1, sBoxSlot + 1);
        Text(8, 24, b);
        for (i = 0; i < 30; i++) {
            u16 sp = GetBoxMonData(GetBoxedMonPtr(sBox, i), MON_DATA_SPECIES);
            FillWindowPixelRect(0, sp ? 10 : 6, 9 + (i % 6) * 17, 49 + (i / 6) * 15, 14, 12);
            if (i == sBoxSlot)
                FillWindowPixelRect(0, 12, 9 + (i % 6) * 17, 49 + (i / 6) * 15, 14, 2);
        }
        i = GetBoxMonData(GetBoxedMonPtr(sBox, sBoxSlot), MON_DATA_SPECIES);
        if (i)
            SpeciesName(116, 56, i);
        else
            Text(116, 56, "Empty");
        Text(116, 81, "A Withdraw");
        Text(116, 97, "SELECT Deposit");
        SpeciesName(116, 114, GetMonData(&PARTY[sPartySlot], MON_DATA_SPECIES));
        Text(5, 145, "L/R Box   START Party slot   B Back");
    } else if (sUi == UI_WILD) {
        struct WildActor *a=&sWild[sTalkActor];
        Panel("WILD POKEMON");SpeciesName(10,27,a->species);
        PwFormat(b,sizeof(b),"Level %u",a->level);Text(137,27,b);
        Text(10,48,a->friendly?"It seems friendly. Invite it to join you.":"It is watching you cautiously.");
        if(a->friendly) {Row(0,76,"ASK TO JOIN");Row(1,96,"BATTLE");Row(2,116,"LEAVE");}
        else {Row(0,76,"BATTLE");Row(1,96,"LEAVE");}
        Text(6,145,"A Choose   B Leave");
    } else if (sUi == UI_RESIDENT) {
        u16 species=sResidentSpecies[sTalkResident];
        Panel("HABITAT POKEMON");SpeciesName(10,27,species);
        Text(10,45,HabitatHappy(sTalkResident)?"Happy in this habitat.":"This habitat does not suit it.");
        Text(10,61,HasType(species,TYPE_BUG)?"Produces SILKY THREAD":HasType(species,TYPE_FLYING)?"Produces SOFT FEATHERS":
            HasType(species,TYPE_GRASS)?"Produces BERRIES":HasType(species,TYPE_ROCK)||HasType(species,TYPE_GROUND)?"Produces STONE":"Produces FIBER");
        Row(0,82,"COLLECT MATERIALS");Row(1,101,"PICK UP");Row(2,120,"LEAVE");
        Text(6,145,"A Choose   B Leave");
    } else if (sUi == UI_FIELD) {
        Panel("POKEMON FIELD SKILLS");
        Text(8,25,"Healthy partners help automatically.");
        for(i=0;i<5;i++) {
            int worker=FindWorker(i);
            Text(10,47+i*17,sSkillNames[i]);
            if(worker>=0) SpeciesName(78,47+i*17,GetMonData(&PARTY[worker],MON_DATA_SPECIES));
            else Text(78,47+i*17,"Find a partner");
        }
        Text(6,145,"A Interact / gather   R Build   B Back");
    } else if (sUi == UI_SAVE) {
        Panel("SAVE WORLD");
        Text(10, 36, "Saving world and Pokemon...");
        Text(10, 64, "Do not turn off the emulator.");
    }
    if (sNoticeFrames && sUi != UI_TITLE && sUi != UI_SEED && sUi != UI_SAVE) {
        char line[42]; unsigned split=0,n=0;
        while(sNotice[n] && n<38) {if(sNotice[n]==' ')split=n;n++;}
        if(!sNotice[n])split=n;
        else if(!split)split=n;
        memcpy(line,sNotice,split);line[split]=0;
        FillWindowPixelRect(0,1,2,110,236,33);
        Text(6,111,line);
        if(sNotice[split])Text(6,126,sNotice+split+(sNotice[split]==' '));
    }
    CopyWindowToVram(0, COPYWIN_FULL);
    CopyBgTilemapBufferToVram(0);
    sUiDirty = FALSE;
}
static void SetWorldSprite(u8 id, int x, int y, u16 tag, u8 direction, u8 walking, bool8 visible) {
    struct Sprite *sp = &gSprites[id];
    int frame = direction == 0 ? 2 : direction == 1 ? 0 : 4;
    if (walking && (sFrames & 8)) frame++;
    sp->x = x; sp->y = y;
    sp->invisible = !visible || x < -16 || x > 256 || y < 0 || y > 168;
    sp->oam.tileNum = GetSpriteTileStartByTag(tag) + frame * 4;
    sp->hFlip = direction == 3;
    sp->subpriority = y < 0 ? 255 : y > 160 ? 0 : 160 - y;
}
static void UpdateSprites(void) {
    int i;
    bool8 visible = sUi == UI_WORLD || sUi == UI_BUILD || sUi == UI_TITLE;
    struct Sprite *p = &gSprites[sPlayerSprite];
    u8 dir = WORLD.facing;
    u8 frame = (dir == 1 ? 0 : dir == 0 ? 2 : dir == 2 ? 4 : 6) +
        (sMoveFrames && (sFrames & 8) ? 1 : 0);
    p->oam.tileNum = GetSpriteTileStartByTag(PLAYER_TAG) + frame * 4;
    p->invisible = !visible; p->subpriority = 80;
    for (i = 0; i < ACTORS; i++) {
        struct WildActor *a = &sWild[i];
        SetWorldSprite(a->sprite,120+(a->x-WORLD.playerX)*16+a->offsetX-sScrollX,
            80+(a->y-WORLD.playerY)*16+a->offsetY-sScrollY,PLAYER_TAG+i+1,a->direction,a->moving,visible&&a->active);
    }
    for (i = 0; i < PW_RESIDENTS; i++)
        SetWorldSprite(sResidentSprites[i],120+(WORLD.residents[i].x-WORLD.playerX)*16-sScrollX,
            80+(WORLD.residents[i].y-WORLD.playerY)*16-sScrollY,PLAYER_TAG+ACTORS+1+i,1,(sFrames%120<16),visible&&WORLD.residents[i].active);
    SetWorldSprite(sPartnerSprite,120+(sFollowX-WORLD.playerX)*16+sFollowOffsetX-sScrollX,
        80+(sFollowY-WORLD.playerY)*16+sFollowOffsetY-sScrollY,PLAYER_TAG+PARTNER_SLOT,sFollowDir,
        sFollowOffsetX||sFollowOffsetY,visible&&sUi!=UI_TITLE&&PARTY_COUNT>0);
    gSprites[sCursorSprite].x = 120 + sCursorX * 16;
    gSprites[sCursorSprite].y = 80 + sCursorY * 16;
    gSprites[sCursorSprite].invisible = sUi != UI_BUILD;
}
static u16 SpeciesForBiome(u8 biome, u32 r) {
    static const u16 pool[5][6] = {{16, 19, 21, 25, 29, 32},
                                   {10, 13, 25, 43, 46, 39},
                                   {54, 98, 116, 129, 19, 27},
                                   {54, 60, 98, 116, 129, 129},
                                   {66, 74, 95, 27, 41, 81}};
    return pool[biome][r % 6];
}
static void SpawnActors(void) {
    int i, t;
    for (i = 0; i < ACTORS; i++) {
        sWild[i].active = 0;
        for (t = 0; t < 32; t++) {
            u32 r = PwWorld_Hash(WORLD.seed, WORLD.playerX + i, WORLD.playerY + t, WORLD.steps + 0x711);
            int x = WORLD.playerX + (int)(r % 15) - 7, y = WORLD.playerY + (int)((r >> 9) % 9) - 4, j;
            u8 tile = PwGame_TileAt(&sGame, x, y);
            if (Abs(x - WORLD.playerX) + Abs(y - WORLD.playerY) < 4 || !PwGame_Passable(tile) ||
                tile >= PW_TILE_FLOOR)
                continue;
            for (j = 0; j < i; j++)
                if (sWild[j].active && sWild[j].x == x && sWild[j].y == y)
                    break;
            if (j < i)
                continue;
            if (ResidentAt(x,y) >= 0) continue;
            sWild[i].x = x;
            sWild[i].y = y;
            sWild[i].species = SpeciesForBiome(PwWorld_BiomeAtTile(x, y), r >> 16);
            sWild[i].level = 2 + (r >> 22) % 4;
            sWild[i].asset = AssetId(sWild[i].species);
            sWild[i].active = 1;
            sWild[i].friendly = (r & 3) == 0;
            sWild[i].direction = r % 4; sWild[i].moving = 0;
            sWild[i].offsetX = sWild[i].offsetY = 0;
            sWild[i].friendId = 255; sWild[i].delay = 20 + (r >> 4) % 80;
            break;
        }
    }
    if (Abs(WORLD.playerX) < 10 && Abs(WORLD.playerY) < 8) {
        static const s8 x[4] = {-3,4,3,-4}, y[4] = {2,3,-3,-3};
        static const u16 species[4] = {SPECIES_ODDISH,SPECIES_GEODUDE,SPECIES_CATERPIE,SPECIES_PIDGEY};
        for (i = 0; i < 4; i++) {
            if (WORLD.starterFriends & (1 << i)) {sWild[i].active = 0; continue;}
            sWild[i].x=x[i]; sWild[i].y=y[i]; sWild[i].species=species[i];
            sWild[i].asset=AssetId(species[i]); sWild[i].level=5; sWild[i].active=1;
            sWild[i].friendly=1; sWild[i].friendId=i; sWild[i].direction=1;
            sWild[i].moving=0; sWild[i].offsetX=sWild[i].offsetY=0;
        }
    }

}
static void ReloadActorGfx(void) {
    int i;
    for (i = 0; i < ACTORS; i++) {
        if (sWild[i].sprite < MAX_SPRITES)
            DestroySprite(&gSprites[sWild[i].sprite]);
        FreeSpriteTilesByTag(PLAYER_TAG + i + 1);
        FreeSpritePaletteByTag(PLAYER_TAG + i + 1);
        MakeActorSprite(i + 1, sWild[i].asset ? sWild[i].asset : 1);
    }
    ApplyLighting();
}
static int ActorAt(int x, int y) {
    int i;
    for (i = 0; i < ACTORS; i++)
        if (sWild[i].active && sWild[i].x == x && sWild[i].y == y)
            return i;
    return -1;
}
static void SetUi(u8 ui) {
    sNoticeFrames = 0;
    sUi = ui;
    sSelection = 0;
    sUiDirty = TRUE;
    PlaySE(SE_SELECT);
}
static void StartNew(void) {
    u32 seed = sSeed;
    SeedRng(seed ^ sFrames);
    Sav2_ClearSetDefault();
    NewGameInitData();
    StringCopy(gSaveBlock2Ptr->playerName, COMPOUND_STRING("WILDER"));
    gSaveBlock2Ptr->playerGender = MALE;
    SetTrainerId(Random32(), gSaveBlock2Ptr->playerTrainerId);
    gSaveBlock2Ptr->optionsTextSpeed = OPTIONS_TEXT_SPEED_FAST;
    gSaveBlock1Ptr->location.mapGroup = MAP_GROUP(MAP_ROUTE101);
    gSaveBlock1Ptr->location.mapNum = MAP_NUM(MAP_ROUTE101);
    gMapHeader = *Overworld_GetMapHeaderByGroupAndId(MAP_GROUP(MAP_ROUTE101), MAP_NUM(MAP_ROUTE101));
    PwWorld_BindSave(&WORLD);
    PwGame_New(&sGame, seed);
    SeedRng(seed);
    CreateRandomMon(&PARTY[0], SPECIES_MACHOP, 7);
    PARTY_COUNT = 1;
    SetMonMoveSlot(&PARTY[0], MOVE_KARATE_CHOP, 0);
    SetMonMoveSlot(&PARTY[0], MOVE_TACKLE, 1);
    SetMonMoveSlot(&PARTY[0], MOVE_LEER, 2);
    GetSetPokedexFlag(SpeciesToNationalPokedexNum(SPECIES_MACHOP), FLAG_SET_SEEN);
    GetSetPokedexFlag(SpeciesToNationalPokedexNum(SPECIES_MACHOP), FLAG_SET_CAUGHT);
    FlagSet(FLAG_SYS_POKEMON_GET);
    FlagSet(FLAG_SYS_POKEDEX_GET);
    EnableNationalPokedex();
    AddBagItem(ITEM_POKE_BALL, 15);
    AddBagItem(ITEM_POTION, 5);
    WORLD.wood = 6;
    WORLD.stone = 3;
    WORLD.fiber = 2;
    WORLD.follower = 0;
    sFollowX=0; sFollowY=1; sFollowOffsetX=sFollowOffsetY=0;
    PlayTimeCounter_Start();
    sScrollX = sScrollY = sMoveFrames = 0;
    SpawnActors();
    sUi = UI_WORLD;
    Notice("Meet friendly Oddish west of camp. Grass Pokemon use CUT.");
    InitScreen();
    PlayBGM(MUS_RG_VIRIDIAN_FOREST);
}
void CB2_PwBoot(void) {
    sSeed = 0x00483729;
    sMoveSpeed = 2;
    sCursorY = 1;
    SetSaveBlocksPointers(GetSaveBlocksPointersBaseOffset());
    ResetMenuAndMonGlobals();
    Save_ResetSaveCounters();
    LoadGameSave(SAVE_NORMAL);
    InitHeap(gHeap, HEAP_SIZE);
    PwWorld_BindSave(&WORLD);
    sHasSave = (gSaveFileStatus == SAVE_STATUS_OK || gSaveFileStatus == SAVE_STATUS_CORRUPT) &&
               PwWorld_Validate(&WORLD) && PwWorld_CheckIntegrity(&WORLD);
    if (!sHasSave) {
        Sav2_ClearSetDefault();
        PwWorld_Init(sSeed);
    }
    PwGame_Restore(&sGame);
    SpawnActors();
    sUi = UI_TITLE;
    sSelection = sHasSave ? 0 : 1;
    sNoticeFrames = 0;
    sScrollX = sScrollY = 0;
    InitScreen();
}
static void ResumeMusic(void) {
    if (WORLD.music)
        PlayBGM(MUS_RG_VIRIDIAN_FOREST);
    else
        StopMapMusic();
}
void CB2_PwResume(void) {
    sUi = UI_WORLD;
    sScrollX = sScrollY = sMoveFrames = 0;
    sFollowX=WORLD.playerX; sFollowY=WORLD.playerY+1;
    sFollowOffsetX=sFollowOffsetY=0;
    PwWorld_BindSave(&WORLD);
    PwGame_Restore(&sGame);
    CalculatePlayerPartyCount();
    if (sBattleReturning) {
        int i, alive = 0;
        for (i = 0; i < PARTY_COUNT; i++)
            alive += GetMonData(&PARTY[i], MON_DATA_HP) > 0;
        if (!alive) {
            HealPlayerParty();
            WORLD.playerX = 0;
            WORLD.playerY = 1;
            PwGame_Restore(&sGame);
            Notice("Your team recovered at the starting camp.");
            SpawnActors();
        } else if (gBattleOutcome == B_OUTCOME_CAUGHT)
            Notice("Pokemon caught! Check POKEMON or STORAGE.");
        sBattleReturning = FALSE;
        sEncounterCooldown = 80;
    }
    InitScreen();
    ResumeMusic();
}
u8 Pw_GetBattleEnvironment(void) {
    u8 b = PwWorld_BiomeAtTile(WORLD.playerX, WORLD.playerY);
    return b == PW_BIOME_BEACH      ? BATTLE_ENVIRONMENT_SAND
           : b == PW_BIOME_OCEAN    ? BATTLE_ENVIRONMENT_WATER
           : b == PW_BIOME_MOUNTAIN ? BATTLE_ENVIRONMENT_MOUNTAIN
                                    : BATTLE_ENVIRONMENT_GRASS;
}
static void BeginBattle(u16 species, u8 level) {
    SetVBlankCallback(NULL);
    FreeAllWindowBuffers();
    ResetTasks();
    ZeroEnemyPartyMons();
    CreateRandomMon(&gParties[B_TRAINER_OPPONENT_A][0], species, level);
    gPartiesCount[B_TRAINER_OPPONENT_A] = 1;
    gBattleTypeFlags = 0;
    gBattleOutcome = 0;
    gMain.savedCallback = CB2_PwResume;
    gMain.callback1 = NULL;
    sBattleReturning = TRUE;
    PlayBGM(MUS_VS_WILD);
    SetMainCallback2(CB2_InitBattle);
}
static void BeginBuild(void) {
    if (FindWorker(FIELD_BUILD) < 0) {Notice("A healthy Fighting Pokemon must help you BUILD."); return;}
    sCursorX = sDx[WORLD.facing];
    sCursorY = sDy[WORLD.facing];
    SetUi(UI_BUILD);
}
static u16 AddMaterial(u16 value, u16 amount) {
    u32 total = (u32)value + amount;
    return total > 65535 ? 65535 : total;
}
static void Place(void) {
    int x = WORLD.playerX + sCursorX, y = WORLD.playerY + sCursorY;
    u8 old = PwGame_TileAt(&sGame, x, y), t = sBuildTiles[sBuild];
    if ((!sCursorX && !sCursorY) || ActorAt(x, y) >= 0 || ResidentAt(x,y) >= 0) {
        Notice("Choose a free tile.");
        return;
    }
    if ((t == PW_TILE_BRIDGE && old != PW_TILE_WATER) ||
        (t == PW_TILE_ROOF && old != PW_TILE_FLOOR) ||
        (t != PW_TILE_BRIDGE && t != PW_TILE_ROOF && (!PwGame_Passable(old) ||
            (old >= PW_TILE_FLOOR && !(old == PW_TILE_FLOOR && (t == PW_TILE_WALL || t == PW_TILE_DOOR || t == PW_TILE_BED)))))) {
        Notice(t == PW_TILE_BRIDGE ? "Bridges go on water." : "Clear this tile first.");
        return;
    }
    if (t == PW_TILE_BED && (WORLD.thread < 2 || WORLD.feathers < 2)) {
        Notice("Bed: 6 wood, 2 thread, 2 feathers from camp Pokemon."); return;
    }
    if (WORLD.wood < sCosts[sBuild][0] || WORLD.stone < sCosts[sBuild][1] ||
        WORLD.fiber < sCosts[sBuild][2]) {
        Notice("Not enough materials. Gather more nearby.");
        return;
    }
    if (!PwGame_Edit(&sGame, x, y, t)) {
        Notice("World edit limit reached: 192 tiles.");
        return;
    }
    if (t == PW_TILE_BED) {WORLD.thread -= 2; WORLD.feathers -= 2;}
    WORLD.wood -= sCosts[sBuild][0];
    WORLD.stone -= sCosts[sBuild][1];
    WORLD.fiber -= sCosts[sBuild][2];
    sWorldDirty = TRUE;
    sUiDirty = TRUE;
    PlaySE(SE_SELECT);
}
static void Dismantle(void) {
    if (FindWorker(FIELD_CUT) < 0) {Notice("A Grass Pokemon must help you CUT buildings down."); return;}
    int x = WORLD.playerX + sCursorX, y = WORLD.playerY + sCursorY, i;
    u8 old = PwGame_TileAt(&sGame, x, y), base = old == PW_TILE_BRIDGE ? PW_TILE_WATER : PW_TILE_GRASS;
    if (!sCursorX && !sCursorY) {
        Notice("Step off it before dismantling.");
        return;
    }
    if (x == 0 && y == 3) {
        Notice("The starting camp stays here.");
        return;
    }
    for (i = 0; i < ARRAY_COUNT(sBuildTiles); i++)
        if (sBuildTiles[i] == old)
            break;
    if (i == ARRAY_COUNT(sBuildTiles)) {
        Notice("Select a placed object to dismantle.");
        return;
    }
    if (PwGame_Edit(&sGame, x, y, base)) {
        WORLD.wood = AddMaterial(WORLD.wood, sCosts[i][0] / 2);
        WORLD.stone = AddMaterial(WORLD.stone, sCosts[i][1] / 2);
        WORLD.fiber = AddMaterial(WORLD.fiber, sCosts[i][2] / 2);
        if (old == PW_TILE_BED) {
            if (WORLD.thread < 65535) WORLD.thread++;
            if (WORLD.feathers < 65535) WORLD.feathers++;
        }
        sWorldDirty = TRUE;
        Notice("Dismantled. Half the materials recovered.");
    }
}
static void Interact(void) {
    int x=WORLD.playerX+sDx[WORLD.facing], y=WORLD.playerY+sDy[WORLD.facing];
    int a=ActorAt(x,y), r=ResidentAt(x,y), worker=-1, result;
    u8 tile=PwGame_TileAt(&sGame,x,y);
    char message[72];
    if (a>=0) {sTalkActor=a; SetUi(UI_WILD); return;}
    if (r>=0) {sTalkResident=r; SetUi(UI_RESIDENT); return;}
    if (tile==PW_TILE_FIRE || tile==PW_TILE_BED) {
        HealPlayerParty(); WORLD.campX=x; WORLD.campY=y;
        Notice("Your team rested. HP, PP and status restored."); PlaySE(SE_SELECT); return;
    }
    if (tile==PW_TILE_SOIL) {
        if (!WORLD.seeds) {Notice("No seeds. Harvest a berry plant for more."); return;}
        if (!PwGame_Edit(&sGame,x,y,PW_TILE_SPROUT)) {Notice("World edit limit reached."); return;}
        WORLD.seeds--; sWorldDirty=TRUE; Notice("Seed planted. It will grow in two minutes of play."); return;
    }
    if (tile==PW_TILE_SPROUT) {Notice("A growing berry plant. Come back after exploring."); return;}
    if (tile==PW_TILE_BERRY) {
        if (!PwGame_Edit(&sGame,x,y,PW_TILE_SOIL)) return;
        if (WORLD.berries<65533) WORLD.berries+=3;
        if (WORLD.seeds<65534) WORLD.seeds+=2;
        sWorldDirty=TRUE; Notice("Harvested 3 berries and 2 seeds. Plant another!"); return;
    }
    if (tile==PW_TILE_TREE || tile==PW_TILE_TALL_GRASS || tile==PW_TILE_FLOWER) worker=FindWorker(FIELD_CUT);
    else if (tile==PW_TILE_ROCK) worker=FindWorker(FIELD_SMASH);
    else if (tile==PW_TILE_GRASS || tile==PW_TILE_SAND || tile==PW_TILE_DIRT) {
        worker=FindWorker(FIELD_DIG);
        if (worker<0) {Notice("Ground Pokemon use DIG to prepare a planting plot."); return;}
        if (!PwGame_Edit(&sGame,x,y,PW_TILE_SOIL)) {Notice("World edit limit reached."); return;}
        sWorldDirty=TRUE; Notice("DIG prepared the soil. Press A again to plant a seed."); return;
    } else {Notice("Face a resource, Pokemon, plant, bed or campfire."); return;}
    if (worker<0) {
        Notice(tile==PW_TILE_ROCK ? "Rock Pokemon use SMASH. Befriend Geodude near camp." :
            "Grass Pokemon use CUT. Befriend Oddish near camp."); return;
    }
    result=PwGame_GatherFacing(&sGame,WORLD.facing);
    if (result>0) {
        PwFormat(message,sizeof(message),"%s! Collected %s.",tile==PW_TILE_ROCK?"SMASH":"CUT",
            tile==PW_TILE_TREE?"3 wood":tile==PW_TILE_ROCK?"2 stone":"2 fiber");
        Notice(message); sWorldDirty=TRUE; PlaySE(SE_SELECT);
    } else Notice(result==-1?"World edit limit reached.":"Resource storage is full.");
}
static bool8 HasType(u16 species, u8 type) {
    return species && (gSpeciesInfo[species].types[0]==type || gSpeciesInfo[species].types[1]==type);
}
static bool8 CanUse(u16 species, int skill) {
    if (skill==FIELD_CUT) return HasType(species,TYPE_GRASS) || species==SPECIES_SCYTHER || species==SPECIES_PINSIR;
    if (skill==FIELD_SMASH) return HasType(species,TYPE_ROCK);
    if (skill==FIELD_BUILD) return HasType(species,TYPE_FIGHTING);
    if (skill==FIELD_DIG) return HasType(species,TYPE_GROUND);
    return species==SPECIES_GOLDUCK || species==SPECIES_BLASTOISE || species==SPECIES_GYARADOS ||
        species==SPECIES_LAPRAS || species==SPECIES_VAPOREON || species==SPECIES_POLIWRATH ||
        species==SPECIES_TENTACRUEL || species==SPECIES_KINGLER || species==SPECIES_STARMIE ||
        species==SPECIES_SEAKING || species==SPECIES_DEWGONG || species==SPECIES_SLOWBRO || species==SPECIES_CLOYSTER;
}
static int FindWorker(int skill) {
    int i;
    for (i=0;i<PARTY_COUNT;i++)
        if (GetMonData(&PARTY[i],MON_DATA_HP)>0 && CanUse(GetMonData(&PARTY[i],MON_DATA_SPECIES),skill)) return i;
    return -1;
}
static int ResidentAt(int x,int y) {
    int i;
    for (i=0;i<PW_RESIDENTS;i++) if (WORLD.residents[i].active && WORLD.residents[i].x==x && WORLD.residents[i].y==y) return i;
    return -1;
}
static void ReloadPartner(void) {
    if (WORLD.follower>=PARTY_COUNT) WORLD.follower=0;
    DestroySprite(&gSprites[sPartnerSprite]);
    FreeSpriteTilesByTag(PLAYER_TAG+PARTNER_SLOT); FreeSpritePaletteByTag(PLAYER_TAG+PARTNER_SLOT);
    MakeActorSprite(PARTNER_SLOT,AssetId(GetMonData(&PARTY[WORLD.follower],MON_DATA_SPECIES)));
    ApplyLighting();
}
static void DropPartner(void) {
    int i,j,x=WORLD.playerX+sDx[WORLD.facing],y=WORLD.playerY+sDy[WORLD.facing];
    u8 tile=PwGame_TileAt(&sGame,x,y);
    if (PARTY_COUNT<=1) {Notice("Keep at least one Pokemon with you."); return;}
    if (!PwGame_Passable(tile) || tile==PW_TILE_SPROUT || ActorAt(x,y)>=0 || ResidentAt(x,y)>=0) {
        Notice("Face a clear patch of ground before placing a Pokemon."); return;
    }
    for (i=0;i<PW_RESIDENTS;i++)
        if (!WORLD.residents[i].active && !GetBoxMonData(GetBoxedMonPtr(RESIDENT_BOX,RESIDENT_SLOT+i),MON_DATA_SPECIES)) break;
    if (i==PW_RESIDENTS) {Notice("All six habitat slots are occupied."); return;}
    *GetBoxedMonPtr(RESIDENT_BOX,RESIDENT_SLOT+i)=PARTY[sSelection].box;
    sResidentSpecies[i]=GetMonData(&PARTY[sSelection],MON_DATA_SPECIES);
    WORLD.residents[i]=(struct PwResident){x,y,90,1,0};
    for (j=sSelection;j<PARTY_COUNT-1;j++) PARTY[j]=PARTY[j+1];
    ZeroMonData(&PARTY[PARTY_COUNT-1]); CalculatePlayerPartyCount();
    WORLD.follower=0;
    DestroySprite(&gSprites[sResidentSprites[i]]);
    FreeSpriteTilesByTag(PLAYER_TAG+ACTORS+1+i); FreeSpritePaletteByTag(PLAYER_TAG+ACTORS+1+i);
    MakeActorSprite(ACTORS+1+i,AssetId(sResidentSpecies[i])); ReloadPartner();
    SetUi(UI_WORLD); Notice("Pokemon placed. A to visit. Happy residents give materials.");
}
static bool8 HabitatHappy(unsigned slot) {
    const struct PwResident *r=&WORLD.residents[slot];
    u16 sp=sResidentSpecies[slot];
    u8 t=PwGame_TileAt(&sGame,r->x,r->y), b=PwWorld_BiomeAtTile(r->x,r->y);
    if (HasType(sp,TYPE_WATER)) {
        int i; for(i=0;i<4;i++) if(PwGame_TileAt(&sGame,r->x+sDx[i],r->y+sDy[i])==PW_TILE_WATER) return TRUE;
        return FALSE;
    }
    if (HasType(sp,TYPE_ROCK)||HasType(sp,TYPE_GROUND)) return t==PW_TILE_DIRT||t==PW_TILE_SOIL||t==PW_TILE_SAND||b==PW_BIOME_MOUNTAIN;
    return b==PW_BIOME_PLAINS||b==PW_BIOME_FOREST;
}
static void ResidentAction(void) {
    unsigned i=sTalkResident;
    if (sSelection==0) {
        u16 *resource;
        const char *name;
        if (!HabitatHappy(i)) {Notice("This Pokemon needs a different habitat to make materials."); return;}
        if (WORLD.residents[i].cooldown) {Notice("Still resting. Happy residents make materials every 90 seconds."); return;}
        if (HasType(sResidentSpecies[i],TYPE_BUG)) {resource=&WORLD.thread;name="2 silky thread";}
        else if (HasType(sResidentSpecies[i],TYPE_FLYING)) {resource=&WORLD.feathers;name="2 soft feathers";}
        else if (HasType(sResidentSpecies[i],TYPE_GRASS)) {resource=&WORLD.berries;name="2 berries";}
        else if (HasType(sResidentSpecies[i],TYPE_ROCK)||HasType(sResidentSpecies[i],TYPE_GROUND)) {resource=&WORLD.stone;name="2 stone";}
        else {resource=&WORLD.fiber;name="2 fiber";}
        if (*resource>65533) {Notice("That material is full."); return;}
        *resource+=2; WORLD.residents[i].cooldown=90;
        {char b[72];PwFormat(b,sizeof(b),"Your Pokemon gave you %s!",name);Notice(b);}
    } else if (sSelection==1) {
        struct BoxPokemon *box=GetBoxedMonPtr(RESIDENT_BOX,RESIDENT_SLOT+i);
        if (PARTY_COUNT>=6) {Notice("Your party is full. Make room first."); return;}
        BoxMonToMon(box,&PARTY[PARTY_COUNT]); memset(box,0,sizeof(*box));
        WORLD.residents[i].active=0; sResidentSpecies[i]=0; CalculatePlayerPartyCount();
        SetUi(UI_WORLD); Notice("Your Pokemon joined you again.");
    } else SetUi(UI_WORLD);
}
static void WildAction(void) {
    struct WildActor *a=&sWild[sTalkActor];
    if (a->friendly && sSelection==0) {
        if (PARTY_COUNT>=6) {Notice("Party full. Put a Pokemon in storage first."); return;}
        CreateRandomMon(&PARTY[PARTY_COUNT],a->species,a->level); PARTY_COUNT++;
        GetSetPokedexFlag(SpeciesToNationalPokedexNum(a->species),FLAG_SET_SEEN);
        GetSetPokedexFlag(SpeciesToNationalPokedexNum(a->species),FLAG_SET_CAUGHT);
        if (a->friendId<4) WORLD.starterFriends|=1<<a->friendId;
        a->active=0; SetUi(UI_WORLD); Notice("A new friend joined your team. Check L for field skills.");
    } else if (sSelection==(a->friendly?1:0)) {
        a->active=0; BeginBattle(a->species,a->level);
    } else SetUi(UI_WORLD);
}
static void Wander(void) {
    int i;
    for (i=0;i<ACTORS;i++) {
        struct WildActor *a=&sWild[i];
        if (!a->active) continue;
        if (a->moving) {
            a->moving--;
            if (a->offsetX>0) a->offsetX-=2; else if(a->offsetX<0) a->offsetX+=2;
            if (a->offsetY>0) a->offsetY-=2; else if(a->offsetY<0) a->offsetY+=2;
            continue;
        }
        if (a->friendId<4) continue;
        if (a->delay) {a->delay--;continue;}
        {
            u32 r=PwWorld_Hash(WORLD.seed,a->x,a->y,sFrames+i);
            int dir=r%4,x=a->x+sDx[dir],y=a->y+sDy[dir];
            a->delay=35+(r>>8)%90; a->direction=dir;
            if (Abs(x-WORLD.playerX)>11 || Abs(y-WORLD.playerY)>7 ||
                (x==WORLD.playerX&&y==WORLD.playerY) || (x==sFollowX&&y==sFollowY) ||
                ActorAt(x,y)>=0 || ResidentAt(x,y)>=0 || !PwGame_Passable(PwGame_TileAt(&sGame,x,y))) continue;
            a->x=x;a->y=y;a->offsetX=-sDx[dir]*16;a->offsetY=-sDy[dir]*16;a->moving=8;
        }
    }
}
static void ApplyLighting(void) {
    unsigned phase=(WORLD.seconds%720)/180;
    u8 coefficient=phase==3?5:phase==2?2:0;
    BlendPalette(0,224,coefficient,RGB(4,7,13));
    BlendPalette(256,13*16,coefficient,RGB(4,7,13));
}
static void TickWorld(void) {
    if (++sSecondFrames<60) return;
    sSecondFrames=0;
    if (PwWorld_Tick()) {
        int x,y;
        for(y=0;y<3;y++)for(x=0;x<3;x++) if(sGame.active[y][x].valid) PwWorld_ApplyEdits(&sGame.active[y][x]);
        sWorldDirty=TRUE; Notice("Your berry plants have ripened.");
    }
    if (WORLD.seconds%60==0) {sUiDirty=TRUE;ApplyLighting();}
}
static void DoSave(void) {
    u8 result;
    PwWorld_Seal(&WORLD);
    SetVBlankCallback(NULL);
    result = TrySavingData(SAVE_NORMAL);
    SetVBlankCallback(VBlank);
    if (result == SAVE_STATUS_OK) {
        gDifferentSaveFile = FALSE;
        sHasSave = TRUE;
        if (sSaveAndTitle) {
            sSaveAndTitle = FALSE;
            SetMainCallback2(CB2_PwBoot);
            return;
        }
        SetUi(UI_WORLD);
        Notice("World saved. You can close the emulator.");
    } else {
        SetUi(UI_WORLD);
        Notice("Save failed. Use Flash 128K in your emulator.");
    }
}
static void Craft(void) {
    if (sSelection == 0) {
        if (WORLD.wood < 5 || WORLD.stone < 2) {
            Notice("Need 5 wood and 2 stone.");
            return;
        }
        if (!AddBagItem(ITEM_POKE_BALL, 1)) {
            Notice("Bag is full.");
            return;
        }
        WORLD.wood -= 5;
        WORLD.stone -= 2;
        Notice("Crafted 1 Poke Ball.");
    } else {
        if (WORLD.fiber < 3 || WORLD.berries < 1) {
            Notice("Need 3 fiber and 1 berry.");
            return;
        }
        if (!AddBagItem(ITEM_POTION, 1)) {
            Notice("Bag is full.");
            return;
        }
        WORLD.fiber -= 3; WORLD.berries--;
        Notice("Crafted 1 Potion.");
    }
    PlaySE(SE_SELECT);
}
static void UsePotion(void) {
    u32 hp = GetMonData(&PARTY[0], MON_DATA_HP), max = GetMonData(&PARTY[0], MON_DATA_MAX_HP);
    if (hp == 0 || hp == max) {
        Notice("Lead Pokemon cannot use a Potion now.");
        return;
    }
    if (!CheckBagHasItem(ITEM_POTION, 1)) {
        Notice("No Potions. Craft one with 3 fiber.");
        return;
    }
    hp += 20;
    if (hp > max)
        hp = max;
    SetMonData(&PARTY[0], MON_DATA_HP, &hp);
    RemoveBagItem(ITEM_POTION, 1);
    Notice("Potion restored your lead Pokemon's HP.");
}
static void StorageInput(u16 keys) {
    struct BoxPokemon *box = GetBoxedMonPtr(sBox, sBoxSlot);
    u16 species = GetBoxMonData(box, MON_DATA_SPECIES);
    if (!keys)
        return;
    if (keys & DPAD_LEFT)
        sBoxSlot = (sBoxSlot + 29) % 30;
    if (keys & DPAD_RIGHT)
        sBoxSlot = (sBoxSlot + 1) % 30;
    if (keys & DPAD_UP)
        sBoxSlot = (sBoxSlot + 24) % 30;
    if (keys & DPAD_DOWN)
        sBoxSlot = (sBoxSlot + 6) % 30;
    if (keys & L_BUTTON)
        sBox = (sBox + TOTAL_BOXES_COUNT - 1) % TOTAL_BOXES_COUNT;
    if (keys & R_BUTTON)
        sBox = (sBox + 1) % TOTAL_BOXES_COUNT;
    if (keys & START_BUTTON)
        sPartySlot = (sPartySlot + 1) % PARTY_COUNT;
    if ((keys & (A_BUTTON | SELECT_BUTTON)) && sBox==RESIDENT_BOX && sBoxSlot>=RESIDENT_SLOT && WORLD.residents[sBoxSlot-RESIDENT_SLOT].active) {
        Notice("This Pokemon lives at your camp. Visit it to pick it up."); sUiDirty=TRUE; return;
    }
    if (keys & A_BUTTON) {
        if (!species)
            Notice("This slot is empty.");
        else if (PARTY_COUNT >= 6)
            Notice("Party full. Deposit a Pokemon first.");
        else {
            BoxMonToMon(box, &PARTY[PARTY_COUNT]);
            memset(box, 0, sizeof(*box));
            CalculatePlayerPartyCount();
            Notice("Pokemon joined your party.");
        }
    }
    if (keys & SELECT_BUTTON) {
        if (species)
            Notice("Select an empty box slot.");
        else if (PARTY_COUNT <= 1)
            Notice("Keep at least one Pokemon in your party.");
        else {
            int i;
            *box = PARTY[sPartySlot].box;
            for (i = sPartySlot; i < PARTY_COUNT - 1; i++)
                PARTY[i] = PARTY[i + 1];
            ZeroMonData(&PARTY[PARTY_COUNT - 1]);
            CalculatePlayerPartyCount();
            sPartySlot = 0;
            Notice("Pokemon deposited.");
        }
    }
    if (keys & B_BUTTON)
        SetUi(UI_MENU);
    sUiDirty = TRUE;
}
static void Input(void) {
    u16 keys, repeat, ime = REG_IME;
    REG_IME = 0;
    keys = sPressed; repeat = sRepeated;
    sPressed = sRepeated = 0;
    REG_IME = ime;
    int max = 0;
    if (sUi == UI_SAVE) {
        DoSave();
        return;
    }
    if (sUi == UI_TITLE) {
        if (keys & (DPAD_UP | DPAD_DOWN)) {
            if (sHasSave)
                sSelection ^= 1;
            sUiDirty = TRUE;
        }
        if (keys & A_BUTTON) {
            if (!sSelection && sHasSave) {
                CopyPartyAndObjectsFromSave();
                gMapHeader =
                    *Overworld_GetMapHeaderByGroupAndId(MAP_GROUP(MAP_ROUTE101), MAP_NUM(MAP_ROUTE101));
                PlayTimeCounter_Start();
                CB2_PwResume();
            } else
                SetUi(UI_SEED);
        }
        return;
    }
    if (sUi == UI_SEED) {
        if (repeat & DPAD_LEFT)
            sDigit = (sDigit + 7) % 8;
        if (repeat & DPAD_RIGHT)
            sDigit = (sDigit + 1) % 8;
        if (repeat & DPAD_UP)
            sSeed += 1u << ((7 - sDigit) * 4);
        if (repeat & DPAD_DOWN)
            sSeed -= 1u << ((7 - sDigit) * 4);
        if (keys & R_BUTTON)
            sSeed = Random32() ^ sFrames;
        if (keys & B_BUTTON) {
            sUi = UI_TITLE;
            sSelection = sHasSave ? 0 : 1;
        }
        if (keys & START_BUTTON) {
            StartNew();
            return;
        }
        if (repeat)
            sUiDirty = TRUE;
        return;
    }
    if (sUi == UI_BUILD) {
        if (repeat & DPAD_LEFT && sCursorX > -6)
            sCursorX--;
        if (repeat & DPAD_RIGHT && sCursorX < 6)
            sCursorX++;
        if (repeat & DPAD_UP && sCursorY > -3)
            sCursorY--;
        if (repeat & DPAD_DOWN && sCursorY < 3)
            sCursorY++;
        if (keys & L_BUTTON)
            sBuild = (sBuild + ARRAY_COUNT(sBuildTiles) - 1) % ARRAY_COUNT(sBuildTiles);
        if (keys & R_BUTTON)
            sBuild = (sBuild + 1) % ARRAY_COUNT(sBuildTiles);
        if (keys & A_BUTTON)
            Place();
        if (keys & SELECT_BUTTON)
            Dismantle();
        if (keys & (B_BUTTON | START_BUTTON))
            SetUi(UI_WORLD);
        if (keys)
            sUiDirty = TRUE;
        return;
    }
    if (sUi == UI_WORLD) {
        int dir = -1, a;
        struct PwEncounter e;
        if (sMoveFrames) {
            sPendingKeys |= keys & (A_BUTTON | START_BUTTON | SELECT_BUTTON | L_BUTTON | R_BUTTON);
            return;
        }
        keys |= sPendingKeys; sPendingKeys = 0;
        if (keys & START_BUTTON) {
            SetUi(UI_MENU);
            return;
        }
        if (keys & SELECT_BUTTON) {
            SetUi(UI_MAP);
            return;
        }
        if (keys & L_BUTTON) {SetUi(UI_FIELD); return;}
        if (keys & R_BUTTON) {
            BeginBuild();
            return;
        }
        if (keys & A_BUTTON) {
            Interact();
            return;
        }
        if (sHeld & DPAD_UP)
            dir = 0;
        else if (sHeld & DPAD_DOWN)
            dir = 1;
        else if (sHeld & DPAD_LEFT)
            dir = 2;
        else if (sHeld & DPAD_RIGHT)
            dir = 3;
        if (dir < 0)
            return;
        WORLD.facing = dir;
        a = ActorAt(WORLD.playerX + sDx[dir], WORLD.playerY + sDy[dir]);
        if (a >= 0) {
            if (!sWild[a].friendly && !sEncounterCooldown) {
                sWild[a].active=0; BeginBattle(sWild[a].species,sWild[a].level);
            }
            return;
        }
        if (ResidentAt(WORLD.playerX+sDx[dir],WORLD.playerY+sDy[dir])>=0) return;
        if (PwGame_MoveWithSurf(&sGame, dir, FindWorker(FIELD_SURF)>=0)) {
            int oldX=WORLD.playerX-sDx[dir],oldY=WORLD.playerY-sDy[dir];
            sFollowOffsetX=(sFollowX-oldX)*16; sFollowOffsetY=(sFollowY-oldY)*16;
            sFollowDir=oldX>sFollowX?3:oldX<sFollowX?2:oldY<sFollowY?0:1;
            sFollowX=oldX;sFollowY=oldY;
            if ((PwGame_TileAt(&sGame,oldX,oldY)==PW_TILE_ROOF) != (PwGame_TileAt(&sGame,WORLD.playerX,WORLD.playerY)==PW_TILE_ROOF)) sWorldDirty=TRUE;
            sScrollX = -sDx[dir] * 16;
            sScrollY = -sDy[dir] * 16;
            sMoveSpeed = sHeld & B_BUTTON ? 4 : 2;
            sMoveFrames = 16 / sMoveSpeed;
            if (PwWorld_BiomeAtTile(WORLD.playerX,WORLD.playerY) != PwWorld_BiomeAtTile(WORLD.playerX-sDx[dir],WORLD.playerY-sDy[dir])) sUiDirty = TRUE;
            if (!sEncounterCooldown && PwGame_CheckEncounter(&sGame, &e)) {
                sMoveFrames = sScrollX = sScrollY = 0;
                BeginBattle(e.species, e.level);
                return;
            }
            if (WORLD.steps % 48 == 0) {
                SpawnActors();
                ReloadActorGfx();
            }
        }
        return;
    }
    if (sUi == UI_STORAGE) {
        StorageInput(repeat | keys);
        return;
    }
    if (keys & B_BUTTON) {
        SetUi(sUi == UI_MENU || sUi == UI_WILD || sUi == UI_RESIDENT || sUi == UI_FIELD ? UI_WORLD : UI_MENU);
        return;
    }
    if (sUi == UI_MENU)
        max = 9;
    else if (sUi == UI_CRAFT)
        max = 2;
    else if (sUi == UI_PARTY)
        max = PARTY_COUNT;
    else if (sUi == UI_OPTIONS || sUi == UI_RESIDENT)
        max = 3;
    else if (sUi == UI_WILD) max=sWild[sTalkActor].friendly?3:2;
    if (max) {
        if (repeat & DPAD_UP) {
            sSelection = (sSelection + max - 1) % max;
            sUiDirty = TRUE;
        }
        if (repeat & DPAD_DOWN) {
            sSelection = (sSelection + 1) % max;
            sUiDirty = TRUE;
        }
    }
    if (sUi == UI_PARTY && keys & START_BUTTON) {DropPartner();return;}
    if (sUi == UI_PARTY && keys & SELECT_BUTTON) {
        struct Pokemon temp = PARTY[0];
        PARTY[0] = PARTY[sSelection];
        PARTY[sSelection] = temp;
        sSelection = 0;
        WORLD.follower=0; ReloadPartner();
        Notice("Lead Pokemon changed. Your partner follows you.");
    }
    if (!(keys & A_BUTTON))
        return;
    if (sUi == UI_WILD) {WildAction();return;}
    if (sUi == UI_RESIDENT) {ResidentAction();return;}
    if (sUi == UI_MENU) {
        static const u8 destinations[] = {UI_PARTY,   UI_BAG,  UI_CRAFT,   UI_BUILD, UI_MAP,
                                          UI_STORAGE, UI_SAVE, UI_OPTIONS, UI_HELP};
        u8 next = destinations[sSelection];
        if (next == UI_BUILD)
            BeginBuild();
        else {
            SetUi(next);
            sPartySlot = 0;
        }
    } else if (sUi == UI_CRAFT)
        Craft();
    else if (sUi == UI_BAG)
        UsePotion();
    else if (sUi == UI_PARTY) {
        SetVBlankCallback(NULL);
        FreeAllWindowBuffers();
        ShowPokemonSummaryScreen(SUMMARY_MODE_NORMAL, PARTY, sSelection, PARTY_COUNT - 1, CB2_PwResume);
    } else if (sUi == UI_OPTIONS) {
        if (sSelection == 0) {
            WORLD.music ^= 1;
            ResumeMusic();
            sUiDirty = TRUE;
        } else if (sSelection == 1) {
            gSaveBlock2Ptr->optionsBattleSceneOff ^= 1;
            sUiDirty = TRUE;
        } else {
            sSaveAndTitle = TRUE;
            SetUi(UI_SAVE);
        }
    }
}
static void MainLoop(void) {
    sFrames++;
    if (sEncounterCooldown) sEncounterCooldown--;
    if (sNoticeFrames && !--sNoticeFrames) sUiDirty = TRUE;
    if (sMoveFrames) {
        sMoveFrames--;
        if (sScrollX > 0) sScrollX -= sMoveSpeed;
        else if (sScrollX < 0) sScrollX += sMoveSpeed;
        if (sScrollY > 0) sScrollY -= sMoveSpeed;
        else if (sScrollY < 0) sScrollY += sMoveSpeed;
    }
    if (sFollowOffsetX>0) sFollowOffsetX-=sMoveSpeed; else if(sFollowOffsetX<0) sFollowOffsetX+=sMoveSpeed;
    if (sFollowOffsetY>0) sFollowOffsetY-=sMoveSpeed; else if(sFollowOffsetY<0) sFollowOffsetY+=sMoveSpeed;
    Input();
    if (gMain.callback2 != MainLoop) return;
    if (sUi == UI_WORLD || sUi == UI_BUILD) {PwGame_Stream(&sGame, 2); TickWorld();}
    if (sUi == UI_WORLD) Wander();
    if (sWorldDirty || !sRenderValid || sRenderX != WORLD.playerX || sRenderY != WORLD.playerY) DrawWorld();
    ApplyCamera();
    if (sUiDirty) DrawUI();
    if (sFrames % 32 == 0) {
        u8 frame = (sFrames / 32) & 1;
        LoadBgTiles(1, sTerrain + (frame ? PW_GFX_WATER2 : PW_GFX_WATER) * 64, 256, PW_GFX_WATER);
        LoadBgTiles(1, sTerrain + (frame ? PW_GFX_FIRE2 : PW_GFX_FIRE) * 64, 256, PW_GFX_FIRE);
        LoadBgTiles(1, sTerrain + (frame ? PW_GFX_FIRE2_TOP : PW_GFX_FIRE_TOP) * 64, 256, PW_GFX_FIRE_TOP);
    }
    UpdateSprites(); AnimateSprites(); BuildOamBuffer(); UpdatePaletteFade();
}
