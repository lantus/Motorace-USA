#define ZIPPY_LOGO "objects/zippylogo.BPL"
#define CITY_ATTRACT "images/city_attract.BPL"

extern RawMap *city_attract_map;
extern BitMapEx *city_attract_tiles;

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


