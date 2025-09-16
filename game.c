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

    bike_position_x = 180;
    bike_position_y = 180;
}

void GameLoop()
{

}

void SetBackGroundColor(UWORD color)
{
    Copper_SetPalette(0, color);
}
