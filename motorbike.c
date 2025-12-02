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
#include "sprites.h"
#include "memory.h"
#include "motorbike.h"

static Sprite spr_rsrc_bike_moving1;
static Sprite spr_rsrc_bike_moving2;
static Sprite spr_rsrc_bike_moving3;
static Sprite spr_rsrc_bike_turn_left1;
static Sprite spr_rsrc_bike_turn_left2;
static Sprite spr_rsrc_bike_turn_right1;
static Sprite spr_rsrc_bike_turn_right2;

static Sprite spr_rsrc_approach_bike_frame1;
static Sprite spr_rsrc_approach_bike_frame1_left;
static Sprite spr_rsrc_approach_bike_frame1_right;
static Sprite spr_rsrc_approach_bike_frame2;
static Sprite spr_rsrc_approach_bike_frame3;
static Sprite spr_rsrc_approach_bike_frame4;
static Sprite spr_rsrc_approach_bike_frame5;
static Sprite spr_rsrc_approach_bike_frame6;
static Sprite spr_rsrc_approach_bike_frame7;

ULONG *spr_bike_moving1;
ULONG *spr_bike_moving2;
ULONG *spr_bike_moving3;
ULONG *spr_bike_turn_left1;
ULONG *spr_bike_turn_left2;
ULONG *spr_bike_turn_right1;
ULONG *spr_bike_turn_right2;

ULONG *spr_approach_bike_frame1;
ULONG *spr_approach_bike_frame1_left;
ULONG *spr_approach_bike_frame1_right;
ULONG *spr_approach_bike_frame2;
ULONG *spr_approach_bike_frame3;
ULONG *spr_approach_bike_frame4;
ULONG *spr_approach_bike_frame5;
ULONG *spr_approach_bike_frame6;
ULONG *spr_approach_bike_frame7;

ULONG *current_bike_sprite;

UBYTE bike_state = BIKE_STATE_MOVING;

UWORD bike_position_x;
UWORD bike_position_y;

ULONG bike_anim_frames = 0;
static ULONG bike_anim_lr_frames = 0;
static UBYTE bike_frame = 0;

// Add these to your global variables
WORD bike_speed = 42;        // Current speed in mph (starts at idle)
WORD bike_acceleration = 0;  // Current acceleration
WORD scroll_accumulator = 0;  // Fractional scroll position (fixed point)

void MotorBike_Initialize()
{
    Sprites_LoadFromFile(BIKE_MOVING1,&spr_rsrc_bike_moving1);
    Sprites_LoadFromFile(BIKE_MOVING2,&spr_rsrc_bike_moving2);
    Sprites_LoadFromFile(BIKE_MOVING3,&spr_rsrc_bike_moving3);
    Sprites_LoadFromFile(BIKE_LEFT1,&spr_rsrc_bike_turn_left1);
    Sprites_LoadFromFile(BIKE_LEFT2,&spr_rsrc_bike_turn_left2);
    Sprites_LoadFromFile(BIKE_RIGHT1,&spr_rsrc_bike_turn_right1);
    Sprites_LoadFromFile(BIKE_RIGHT2,&spr_rsrc_bike_turn_right2);

    // Approach City Bike Sprites

    Sprites_LoadFromFile(APPROACH_BIKE1,&spr_rsrc_approach_bike_frame1);
    Sprites_LoadFromFile(APPROACH_BIKE2,&spr_rsrc_approach_bike_frame2);
    Sprites_LoadFromFile(APPROACH_BIKE3,&spr_rsrc_approach_bike_frame3);
    Sprites_LoadFromFile(APPROACH_BIKE4,&spr_rsrc_approach_bike_frame4);
    Sprites_LoadFromFile(APPROACH_BIKE5,&spr_rsrc_approach_bike_frame5);
    Sprites_LoadFromFile(APPROACH_BIKE6,&spr_rsrc_approach_bike_frame6);
    Sprites_LoadFromFile(APPROACH_BIKE7,&spr_rsrc_approach_bike_frame7);

    Sprites_LoadFromFile(APPROACH_BIKE_LEFT,&spr_rsrc_approach_bike_frame1_left);
    Sprites_LoadFromFile(APPROACH_BIKE_RIGHT,&spr_rsrc_approach_bike_frame1_right);

    spr_bike_moving1 = Mem_AllocChip(4);
    spr_bike_moving2 = Mem_AllocChip(4);
    spr_bike_moving3 = Mem_AllocChip(4);
    spr_bike_turn_left1 = Mem_AllocChip(4);
    spr_bike_turn_left2 = Mem_AllocChip(4);
    spr_bike_turn_right1 = Mem_AllocChip(4);
    spr_bike_turn_right2 = Mem_AllocChip(4);

    // Approach City Bike Sprites Mem Alloc

    spr_approach_bike_frame1 = Mem_AllocChip(4);
    spr_approach_bike_frame1_left = Mem_AllocChip(4);
    spr_approach_bike_frame1_right = Mem_AllocChip(4);
    spr_approach_bike_frame2 = Mem_AllocChip(4);
    spr_approach_bike_frame3 = Mem_AllocChip(4);
    spr_approach_bike_frame4 = Mem_AllocChip(4);
    spr_approach_bike_frame5 = Mem_AllocChip(4);  
    spr_approach_bike_frame6 = Mem_AllocChip(4);  
    spr_approach_bike_frame7 = Mem_AllocChip(4);       

   

    Sprites_BuildComposite(spr_bike_moving1,2,&spr_rsrc_bike_moving1);
    Sprites_BuildComposite(spr_bike_moving2,2,&spr_rsrc_bike_moving2);
    Sprites_BuildComposite(spr_bike_moving3,2,&spr_rsrc_bike_moving3);
    Sprites_BuildComposite(spr_bike_turn_left1,2,&spr_rsrc_bike_turn_left1);
    Sprites_BuildComposite(spr_bike_turn_left2,2,&spr_rsrc_bike_turn_left2);
    Sprites_BuildComposite(spr_bike_turn_right1,2,&spr_rsrc_bike_turn_right1);
    Sprites_BuildComposite(spr_bike_turn_right2,2,&spr_rsrc_bike_turn_right2);

    // Approach City Bike Sprites Composites

    Sprites_BuildComposite(spr_approach_bike_frame1_left,4,&spr_rsrc_approach_bike_frame1_left);
    Sprites_BuildComposite(spr_approach_bike_frame1_right,4,&spr_rsrc_approach_bike_frame1_right);
    Sprites_BuildComposite(spr_approach_bike_frame1,4,&spr_rsrc_approach_bike_frame1);
    Sprites_BuildComposite(spr_approach_bike_frame2,4,&spr_rsrc_approach_bike_frame2);

    Sprites_ApplyPalette(&spr_rsrc_bike_moving1);
 
}
 
void MotorBike_Reset()
{
    if (game_map == MAP_ATTRACT_INTRO)
    {
        MotorBike_SetFrame(BIKE_FRAME_APPROACH1);
        Sprites_ApplyPalette(&spr_rsrc_approach_bike_frame1);
    }
    else
    {
        MotorBike_SetFrame(BIKE_FRAME_MOVING1);
        Sprites_ApplyPalette(&spr_rsrc_bike_moving1);
    }
 
}

void MotorBike_Draw(WORD x, UWORD y, UBYTE state)
{
    Sprites_SetPointers(current_bike_sprite, 4, SPRITEPTR_ZERO_AND_ONE);

    Sprites_SetScreenPosition((UWORD *)current_bike_sprite[0], x, y, 32);  // height = 32
    Sprites_SetScreenPosition((UWORD *)current_bike_sprite[1], x, y, 32);
    Sprites_SetScreenPosition((UWORD *)current_bike_sprite[2], x+16, y, 32);
    Sprites_SetScreenPosition((UWORD *)current_bike_sprite[3], x+16, y, 32);
}

void UpdateMotorBikePosition(UWORD x, UWORD y, UBYTE state)
{
    if (state == BIKE_STATE_MOVING)
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

    Sprites_SetPointers(current_bike_sprite,2, SPRITEPTR_ZERO_AND_ONE);

    Sprites_SetPosition((UWORD *)current_bike_sprite[0],x,y,y+32);
    Sprites_SetPosition((UWORD *)current_bike_sprite[1],x,y,y+32);

    bike_anim_frames++;
}

void AccelerateMotorBike()
{
    bike_state = BIKE_STATE_ACCELERATING;
    bike_speed += ACCEL_RATE;
    if (bike_speed > MAX_SPEED)
    {
        bike_speed = MAX_SPEED;
    }
}

void BrakeMotorBike()
{
    if (bike_speed > MIN_SPEED)
    {
        bike_speed -= DECEL_RATE;
    }

    if (bike_speed < MIN_SPEED)
    {
        bike_speed = MIN_SPEED;
    }
}
 
void MotorBike_SetFrame(BikeFrame frame)
{
    UBYTE num_sprites;
    
    switch(frame)
    {
        case BIKE_FRAME_MOVING1:
            current_bike_sprite = spr_bike_moving1;
            num_sprites = 2;
            break;
        case BIKE_FRAME_MOVING2:
            current_bike_sprite = spr_bike_moving2;
            num_sprites = 2;
            break;
        case BIKE_FRAME_MOVING3:
            current_bike_sprite = spr_bike_moving3;
            num_sprites = 2;
            break;
        case BIKE_FRAME_LEFT1:
            current_bike_sprite = spr_bike_turn_left1;
            num_sprites = 2;
            break;
        case BIKE_FRAME_LEFT2:
            current_bike_sprite = spr_bike_turn_left2;
            num_sprites = 2;
            break;
        case BIKE_FRAME_RIGHT1:
            current_bike_sprite = spr_bike_turn_right1;
            num_sprites = 2;
            break;
        case BIKE_FRAME_RIGHT2:
            current_bike_sprite = spr_bike_turn_right2;
            num_sprites = 2;
            break;
        case BIKE_FRAME_APPROACH1:
            current_bike_sprite = spr_approach_bike_frame1;
            num_sprites = 4;
            break;
        case BIKE_FRAME_APPROACH2:
            current_bike_sprite = spr_approach_bike_frame2;
            num_sprites = 4;
            break;
        case BIKE_FRAME_APPROACH1_LEFT:
            current_bike_sprite = spr_approach_bike_frame1_left;
            num_sprites = 4;
            break;
        case BIKE_FRAME_APPROACH1_RIGHT:
            current_bike_sprite = spr_approach_bike_frame1_right;
            num_sprites = 4;
            break;
        default:
            return;
    }
    
    Sprites_SetPointers(current_bike_sprite, num_sprites, SPRITEPTR_ZERO_AND_ONE);
}