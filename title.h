#ifndef _TITLE_
#define _TITLE_

#define ZIPPY_LOGO "objects/zippylogo.BPL"
#define CITY_ATTRACT "images/city_attract.BPL"
#define AMIGA_LOGO "images/amilogo.BPL"

 
extern UBYTE title_state;

#define ATTRACTMODE_Y_OFFSET 192

#define ZIPPY_LOGO_WIDTH 80
#define ZIPPY_LOGO_HEIGHT 48
#define ZIPPY_LOGO_WIDTH_WORDS 6            // (includes padding)
#define ATTRACT_MOTORBIKE_CENTER_X 80       // X position of the bike during attract

#define ATTRACTMODE_TILES_WIDTH 256
#define ATTRACTMODE_TILES_HEIGHT 512
#define NUM_ROAD_FRAMES 14

#define AMI_LOGO_WIDTH 32
#define AMI_LOGO_HEIGHT 32
#define AMI_LOGO_WIDTH_WORDS 2

#define ATTRACT_MAX_SPEED 210

enum TitleState {
    TITLE_ATTRACT_INIT = 0,
    TITLE_ATTRACT_START = 1, // draw scene. bike at 0 mph
    TITLE_ATTRACT_ACCEL = 2, // bike accelerating to top speed
    TITLE_ATTRACT_INTO_HORIZON = 3, // bike traveling into horizon.  drop and flash logo cut speed
    TITLE_ATTRACT_LOGO_DROP = 4,
    TITLE_ATTRACT_INSERT_COIN = 5,
    TITLE_ATTRACT_HIGHSCORE = 6,
    TITLE_ATTRACT_CREDITS = 7
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
void Title_LoadAllPalettes();

// Attract Mode stuff
void AttractMode_Update(void);
void AttractMode_UpdateHiScores(void);
void AttractMode_UpdateCredits(void);
void AttractMode_ShowHiScores(void);
void AttractMode_ShowCredits(void);
void AttractMode_DrawText(void);
void AttractMode_BlitAmiLogo();

#endif
