#ifndef _GAME_
#define _GAME_

#include <clib/exec_protos.h>
#include "disk.h"
#include "screen.h"

#include "memory.h"
#include "bitplanes.h"

// Math
#define ABS(x) ((x)<0 ? -(x) : (x))
#define SGN(x) ((x) > 0 ? 1 : ((x) < 0 ? -1 : 0))
// #define SGN(x) ((x > 0) - (x < 0)) // Branchless, subtracting is slower?
#define MIN(x,y) ((x)<(y)? (x): (y))
#define MAX(x,y) ((x)>(y)? (x): (y))
#define CLAMP(x, min, max) ((x) < (min)? (min) : ((x) > (max) ? (max) : (x)))
#define CEIL_TO_FACTOR(x, m) ((((x) + (m) - 1) / (m)) * (m))
#define FLOOR_TO_FACTOR(x, m) (((x) / (m)) * (m))
#define ROUND_TO_FACTOR(x, m) ((((x) + (x) / 2) / (m)) * (m))
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

#define SCREENWIDTH  320
#define SCREENHEIGHT 256
#define BLOCKHEIGHT 16
#define EXTRAHEIGHT 32
#define SCREENBYTESPERROW (SCREENWIDTH / 8)

#define BITMAPWIDTH SCREENWIDTH
#define BITMAPBYTESPERROW (BITMAPWIDTH / 8)
#define BITMAPHEIGHT ((SCREENHEIGHT + EXTRAHEIGHT) * 2)
#define HALFBITMAPHEIGHT (BITMAPHEIGHT / 2)

#define BLOCKSWIDTH 320
#define BLOCKSHEIGHT 256
#define BLOCKSDEPTH 4
#define BLOCKSCOLORS (1L << BLOCKSDEPTH)
#define BLOCKWIDTH 16
#define BLOCKHEIGHT 16
#define BLOCKSBYTESPERROW (BLOCKSWIDTH / 8)
#define BLOCKSPERROW (BLOCKSWIDTH / BLOCKWIDTH)

#define NUMSTEPS BLOCKHEIGHT

#define BITMAPBLOCKSPERROW (BITMAPWIDTH / BLOCKWIDTH)
#define BITMAPBLOCKSPERCOL (BITMAPHEIGHT / BLOCKHEIGHT)
#define HALFBITMAPBLOCKSPERCOL (BITMAPBLOCKSPERCOL / 2)

#define VISIBLEBLOCKSX (SCREENWIDTH / BLOCKWIDTH)
#define VISIBLEBLOCKSY (SCREENHEIGHT / BLOCKHEIGHT)

#define BITMAPPLANELINES (BITMAPHEIGHT * BLOCKSDEPTH)
#define BLOCKPLANELINES  (BLOCKHEIGHT * BLOCKSDEPTH)
 
#define PALSIZE (BLOCKSCOLORS * 2)
#define BLOCKSFILESIZE (BLOCKSWIDTH * BLOCKSHEIGHT * BLOCKSPLANES / 8 )

#define TWOBLOCKS (BITMAPBLOCKSPERROW - NUMSTEPS)
#define TWOBLOCKSTEP (NUMSTEPS - TWOBLOCKS)

#define ROUND2BLOCKWIDTH(x)  ((x) & ~(BLOCKWIDTH - 1))
#define ROUND2BLOCKHEIGHT(x) ((x) & ~(BLOCKHEIGHT - 1))
 

enum GameState
{
	TITLE_SCREEN = 0,
	ROLLING_DEMO = 1,
    GAME_START = 2,
    GAME_OVER = 3,
    HIGH_SCORE = 4
};

enum GameStages
{
    STAGE_LASANGELES = 0,
    STAGE_LASVEGAS = 1,
    STAGE_HOUSTON = 2,
    STAGE_STLOUIS = 3,
    STAGE_CHICAGO = 4,
    STAGE_NEWYORK = 5
};

enum GameDifficulty
{
    FIVEHUNDEDCC = 0,
    SEVENFIFTYCC = 1,
    TWELVEHUNDREDCC = 2
};

extern UBYTE game_stage;
extern UBYTE game_state;
extern UBYTE game_difficulty;

extern WORD mapposy,videoposy;
extern LONG	mapwidth,mapheight;

extern UBYTE *frontbuffer,*blocksbuffer;
extern UWORD *mapdata;

extern ULONG game_score;
extern UBYTE game_rank;

extern struct BitMapEx *BlocksBitmap,*ScreenBitmap;
extern void DrawBlock(LONG x,LONG y,LONG mapx,LONG mapy);

void Game_Initialize();
void Game_NewGame(UBYTE difficulty);
void Game_Loop();
void Game_SetBackGroundColor(UWORD color);
void Game_FillScreen();
void Game_CheckJoyScroll();

#endif