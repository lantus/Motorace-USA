#include "support/gcc8_c_support.h"
#include <exec/types.h>
#include <exec/exec.h>
#include <graphics/gfx.h>
#include <graphics/gfxbase.h>
#include <hardware/custom.h>
#include <hardware/intbits.h>
#include <hardware/dmabits.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/dos.h>
#include "game.h"
#include "map.h"
#include "hardware.h"
#include "bitmap.h"
#include "copper.h"
#include "pixel.h"
#include "sprites.h"
#include "motorbike.h"

UBYTE game_stage = STAGE_LASANGELES;
UBYTE game_state = TITLE_SCREEN;
UBYTE game_difficulty = FIVEHUNDEDCC;
 
WORD mapposy,videoposy;
LONG mapwidth,mapheight;

// One time startup for everything

void InitializeGame()
{
    InitializeSprites();
    LoadMotorbikeSprites();

    SetBackGroundColor(0x125);
}

void NewGame(UBYTE difficulty)
{
    game_stage = STAGE_LASANGELES;
    game_state = GAME_START;
    game_difficulty = difficulty;
    bike_speed = 0;

    // Position bike near bottom of screen
    bike_position_x = SCREENWIDTH / 2;  // Center horizontally (around pixel 96 for 192px display)
    bike_position_y = SCREENHEIGHT - 64;  // Near bottom
    bike_state = BIKE_STATE_MOVING; 

    mapposy = (mapheight * BLOCKHEIGHT) - SCREENHEIGHT - BLOCKHEIGHT;
    videoposy = mapposy % HALFBITMAPHEIGHT;

}

void GameLoop()
{

}

void SetBackGroundColor(UWORD color)
{
    Copper_SetPalette(0, color);
}
