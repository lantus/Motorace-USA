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
#include "sprites.h"
#include "motorbike.h"

static Sprite spr_rsrc_bike_moving1;
static Sprite spr_rsrc_bike_moving2;
static Sprite spr_rsrc_bike_moving3;
static Sprite spr_rsrc_bike_turn_left1;
static Sprite spr_rsrc_bike_turn_left2;
static Sprite spr_rsrc_bike_turn_right1;
static Sprite spr_rsrc_bike_turn_right2;

ULONG *spr_bike_moving1;
ULONG *spr_bike_moving2;
ULONG *spr_bike_moving3;
ULONG *spr_bike_turn_left1;
ULONG *spr_bike_turn_left2;
ULONG *spr_bike_turn_right1;
ULONG *spr_bike_turn_right2;
ULONG *current_bike_sprite;

UBYTE bike_state = BIKE_STATE_MOVING;
UWORD bike_speed;
UWORD bike_position_x;
UWORD bike_position_y;

ULONG bike_anim_frames = 0;
static ULONG bike_anim_lr_frames = 0;

static UBYTE bike_frame = 0;

void LoadMotorbikeSprites()
{
    LoadSpriteSheet(BIKE_MOVING1,&spr_rsrc_bike_moving1);
    LoadSpriteSheet(BIKE_MOVING2,&spr_rsrc_bike_moving2);
    LoadSpriteSheet(BIKE_MOVING3,&spr_rsrc_bike_moving3);
    LoadSpriteSheet(BIKE_LEFT1,&spr_rsrc_bike_turn_left1);
    LoadSpriteSheet(BIKE_LEFT2,&spr_rsrc_bike_turn_left2);
    LoadSpriteSheet(BIKE_RIGHT1,&spr_rsrc_bike_turn_right1);
    LoadSpriteSheet(BIKE_RIGHT2,&spr_rsrc_bike_turn_right2);

    spr_bike_moving1 = AllocMem(4,MEMF_CHIP);
    spr_bike_moving2 = AllocMem(4,MEMF_CHIP);
    spr_bike_moving3 = AllocMem(4,MEMF_CHIP);
    spr_bike_turn_left1 = AllocMem(4,MEMF_CHIP);
    spr_bike_turn_left2 = AllocMem(4,MEMF_CHIP);
    spr_bike_turn_right1 = AllocMem(4,MEMF_CHIP);
    spr_bike_turn_right2 = AllocMem(4,MEMF_CHIP);

    BuildCompositeSprite(spr_bike_moving1,2,&spr_rsrc_bike_moving1);
    BuildCompositeSprite(spr_bike_moving2,2,&spr_rsrc_bike_moving2);
    BuildCompositeSprite(spr_bike_moving3,2,&spr_rsrc_bike_moving3);
    BuildCompositeSprite(spr_bike_turn_left1,2,&spr_rsrc_bike_turn_left1);
    BuildCompositeSprite(spr_bike_turn_left2,2,&spr_rsrc_bike_turn_left2);
    BuildCompositeSprite(spr_bike_turn_right1,2,&spr_rsrc_bike_turn_right1);
    BuildCompositeSprite(spr_bike_turn_right2,2,&spr_rsrc_bike_turn_right2);

    current_bike_sprite = spr_bike_moving1;

    ApplySpritePalette(&spr_rsrc_bike_moving1);
 	SetSpritePointers(current_bike_sprite,2, SPRITEPTR_ZERO_AND_ONE);

}

void UpdateMotorBikePosition(UWORD x, UWORD y, UBYTE state)
{
    if (state == BIKE_STATE_MOVING)
    {
         bike_anim_lr_frames = 0;
         current_bike_sprite = spr_bike_moving1;
    }

    if (state == BIKE_STATE_ACCELERATING)
    {
        bike_anim_lr_frames = 0;

        if (bike_anim_frames % 10 == 0)
        {
            bike_frame ^= 1;

            if (bike_frame == 0)
            {
                current_bike_sprite = spr_bike_moving2;
            }
            else
            {
                current_bike_sprite = spr_bike_moving3;
            }
        }
    }

    if (state == BIKE_STATE_LEFT)
    {
         if (bike_anim_lr_frames >= 6)
         {
            current_bike_sprite = spr_bike_turn_left2;
         }
         else
         {
            current_bike_sprite = spr_bike_turn_left1;
         }

         bike_anim_lr_frames++;
    }

    if (state == BIKE_STATE_RIGHT)
    {
         if (bike_anim_lr_frames >= 6)
         {
            current_bike_sprite = spr_bike_turn_right2;
         }
         else
         {
            current_bike_sprite = spr_bike_turn_right1;
         }

         bike_anim_lr_frames++;
    }

    SetSpritePointers(current_bike_sprite,2, SPRITEPTR_ZERO_AND_ONE);

    SetSpritePosition((UWORD *)current_bike_sprite[0],x,y,y+32);
    SetSpritePosition((UWORD *)current_bike_sprite[1],x,y,y+32);

    bike_anim_frames++;
}

void AccelerateMotorBike(UWORD speed)
{
    speed++;

    //if (speed > )
}

void BrakeMotorBike()
{

}
 