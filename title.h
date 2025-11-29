#define ZIPPY_LOGO "objects/zippylogo.BPL"
#define CITY_ATTRACT "images/city_attract.BPL"

extern RawMap *city_attract_map;
extern BitMapEx *city_attract_tiles;

enum TitleState {
    TITLE_ATTRACT_INIT = 0,
    TITLE_ATTRACT_START = 1, // draw scene. bike at 0 mph
    TITLE_ATTRACT_ACCEL = 2, // bike accelerating to top speed
    TITLE_ATTRACT_INTO_HORIZON = 3, // bike traveling into horizon.  drop and flash logo cut speed
    TITLE_ATTRACT_LOGO_DROP = 4,
    TITLE_ATTRACT_INSERT_COIN = 5,
    TITLE_ATTRACT_HIGHSCORE = 6
};

void Title_Initialize();
void Title_Free();
void Title_Draw();
void Title_Update();
void Title_Reset();
void Title_BlitLogo();
void Title_SaveBackground();
void Title_RestoreLogo();
void Title_OpenMap();
void Title_OpenBlocks();
void Title_DrawSkyline(BOOL deltas_only);
void Title_PreDrawSkyline();

// Attract Mode stuff
void AttractMode_Update(void);
void AttractMode_UpdateHiScores(void);
void AttractMode_ShowHiScores(void);
void AttractMode_DrawText(void);
