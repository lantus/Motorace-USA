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
#include "blitter.h"
#include "timers.h"
#include "cars.h"
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
static Sprite spr_rsrc_approach_bike_frame8;

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
ULONG *spr_approach_bike_frame8;

ULONG *current_bike_sprite;

UBYTE bike_state = BIKE_STATE_MOVING;

UWORD bike_position_x;
UWORD bike_position_y;
LONG  bike_world_y = 0;
ULONG bike_anim_frames = 0;
static ULONG bike_anim_lr_frames = 0;
static UBYTE bike_frame = 0;

// Add these to your global variables
WORD bike_speed = 42;        // Current speed in mph (starts at idle)
WORD bike_acceleration = 0;  // Current acceleration
WORD scroll_accumulator = 0;  // Fractional scroll position (fixed point)

static UBYTE approach_frame_index = 1;  // Track which approach frame we're on
static UBYTE vibration_counter = 0;
static UBYTE current_sprite_count = 2;  // Track whether using 2 or 4 sprites

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
    Sprites_LoadFromFile(APPROACH_BIKE8,&spr_rsrc_approach_bike_frame8); 

    Sprites_LoadFromFile(APPROACH_BIKE_LEFT,&spr_rsrc_approach_bike_frame1_left);
    Sprites_LoadFromFile(APPROACH_BIKE_RIGHT,&spr_rsrc_approach_bike_frame1_right);

    spr_bike_moving1 = Mem_AllocChip(32);
    spr_bike_moving2 = Mem_AllocChip(32);
    spr_bike_moving3 = Mem_AllocChip(32);
    spr_bike_turn_left1 = Mem_AllocChip(32);
    spr_bike_turn_left2 = Mem_AllocChip(32);
    spr_bike_turn_right1 = Mem_AllocChip(32);
    spr_bike_turn_right2 = Mem_AllocChip(32);

    // Approach City Bike Sprites Mem Alloc

    spr_approach_bike_frame1 = Mem_AllocChip(32);
    spr_approach_bike_frame1_left = Mem_AllocChip(32);
    spr_approach_bike_frame1_right = Mem_AllocChip(32);
    spr_approach_bike_frame2 = Mem_AllocChip(32);
    spr_approach_bike_frame3 = Mem_AllocChip(32);
    spr_approach_bike_frame4 = Mem_AllocChip(32);
    spr_approach_bike_frame5 = Mem_AllocChip(32);  
    spr_approach_bike_frame6 = Mem_AllocChip(32);  
    spr_approach_bike_frame7 = Mem_AllocChip(32);       
    spr_approach_bike_frame8 = Mem_AllocChip(32);     
 
    Sprites_BuildComposite(spr_bike_moving1,2,&spr_rsrc_bike_moving1);
    Sprites_BuildComposite(spr_bike_moving2,2,&spr_rsrc_bike_moving2);
    Sprites_BuildComposite(spr_bike_moving3,2,&spr_rsrc_bike_moving3);
    Sprites_BuildComposite(spr_bike_turn_left1,2,&spr_rsrc_bike_turn_left1);
    Sprites_BuildComposite(spr_bike_turn_left2,2,&spr_rsrc_bike_turn_left2);
    Sprites_BuildComposite(spr_bike_turn_right1,2,&spr_rsrc_bike_turn_right1);
    Sprites_BuildComposite(spr_bike_turn_right2,2,&spr_rsrc_bike_turn_right2);

    // Approach City Bike Sprites Composites

    Sprites_BuildComposite(spr_approach_bike_frame1,4,&spr_rsrc_approach_bike_frame1);
    Sprites_BuildComposite(spr_approach_bike_frame2,4,&spr_rsrc_approach_bike_frame2);
    Sprites_BuildComposite(spr_approach_bike_frame3,4,&spr_rsrc_approach_bike_frame3);
    Sprites_BuildComposite(spr_approach_bike_frame4,4,&spr_rsrc_approach_bike_frame4); 
    Sprites_BuildComposite(spr_approach_bike_frame5,4,&spr_rsrc_approach_bike_frame5);
    Sprites_BuildComposite(spr_approach_bike_frame6,4,&spr_rsrc_approach_bike_frame6);   
    Sprites_BuildComposite(spr_approach_bike_frame7,4,&spr_rsrc_approach_bike_frame7);   
    Sprites_BuildComposite(spr_approach_bike_frame8,4,&spr_rsrc_approach_bike_frame8);   

    Sprites_ApplyPalette(&spr_rsrc_bike_moving1);
 
}
 
void MotorBike_Reset()
{
    if (game_map == MAP_ATTRACT_INTRO ||
        game_map == MAP_APPROACH_LASANGELES)
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
    if (current_sprite_count == 2)
    {
        // 16-pixel wide sprite (2 hardware sprites)
        Sprites_SetScreenPosition((UWORD *)current_bike_sprite[0], x, y, 32);
        Sprites_SetScreenPosition((UWORD *)current_bike_sprite[1], x, y, 32);
        Sprites_ClearHigher();
    }
    else // current_sprite_count == 4
    {
        // 32-pixel wide sprite (4 hardware sprites)
        Sprites_SetScreenPosition((UWORD *)current_bike_sprite[0], x, y, 32);
        Sprites_SetScreenPosition((UWORD *)current_bike_sprite[1], x, y, 32);
        Sprites_SetScreenPosition((UWORD *)current_bike_sprite[2], x+16, y, 32);
        Sprites_SetScreenPosition((UWORD *)current_bike_sprite[3], x+16, y, 32);
    }
}

void MotorBike_UpdatePosition(UWORD x, UWORD y, UBYTE state)
{
    if (state == BIKE_STATE_MOVING || 
        state == BIKE_STATE_ACCELERATING  || 
        state == BIKE_STATE_BRAKING)
    {
        bike_anim_lr_frames = 0;

        if (bike_anim_frames % (state == BIKE_STATE_ACCELERATING ? 5 : 10) == 0)
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
    else if (state == BIKE_STATE_LEFT)
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
    else if (state == BIKE_STATE_RIGHT)
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
    else if (state == BIKE_STATE_CRASHED)
    {
         current_bike_sprite = spr_bike_moving1;
         KPrintF("Bike Crashed\n");
    }

    Sprites_SetPointers(current_bike_sprite,2,SPRITEPTR_ZERO_AND_ONE);
    Sprites_SetScreenPosition((UWORD *)current_bike_sprite[0],x,y,32);
    Sprites_SetScreenPosition((UWORD *)current_bike_sprite[1],x,y,32);
   
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
            num_sprites = 4;
            break;
        case BIKE_FRAME_MOVING2:
            current_bike_sprite = spr_bike_moving2;
            num_sprites = 4;
            break;
        case BIKE_FRAME_MOVING3:
            current_bike_sprite = spr_bike_moving3;
            num_sprites = 4;
            break;
        case BIKE_FRAME_LEFT1:
            current_bike_sprite = spr_bike_turn_left1;
            num_sprites = 4;
            break;
        case BIKE_FRAME_LEFT2:
            current_bike_sprite = spr_bike_turn_left2;
            num_sprites = 4;
            break;
        case BIKE_FRAME_RIGHT1:
            current_bike_sprite = spr_bike_turn_right1;
            num_sprites = 4;
            break;
        case BIKE_FRAME_RIGHT2:
            current_bike_sprite = spr_bike_turn_right2;
            num_sprites = 4;
            break;
        case BIKE_FRAME_APPROACH1:
            current_bike_sprite = spr_approach_bike_frame1;
            num_sprites = 4;
            break;
        case BIKE_FRAME_APPROACH2:
            current_bike_sprite = spr_approach_bike_frame2;
            num_sprites = 4;
            break;
        case BIKE_FRAME_APPROACH3:
            current_bike_sprite = spr_approach_bike_frame3;
            num_sprites = 4;
            break;
        case BIKE_FRAME_APPROACH4:
            current_bike_sprite = spr_approach_bike_frame4;
            num_sprites = 4;
            break;
        case BIKE_FRAME_APPROACH5:
            current_bike_sprite = spr_approach_bike_frame5;
            num_sprites = 4;
            break;
        case BIKE_FRAME_APPROACH6:
            current_bike_sprite = spr_approach_bike_frame6;
            num_sprites = 4;
            break;
        case BIKE_FRAME_APPROACH7:
            current_bike_sprite = spr_approach_bike_frame7;
            num_sprites = 4;
            break;          
        case BIKE_FRAME_APPROACH8:
            current_bike_sprite = spr_approach_bike_frame8;
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
    
    current_sprite_count = num_sprites;  // Store for MotorBike_Draw

    Sprites_SetPointers(current_bike_sprite, num_sprites, SPRITEPTR_ZERO_AND_ONE);
}

void MotorBike_UpdateApproachFrame(UWORD y)
{
    BikeFrame new_frame;
    UBYTE new_index = 1;  // Default to frame 1
    
    // Determine frame based on Y position (perspective scaling)
    if (y >= 176)
    {
        new_frame = BIKE_FRAME_APPROACH1;
        new_index = 1;
    }
    else if (y >= 144)
    {
        new_frame = BIKE_FRAME_APPROACH3;
        new_index = 3;
    }
    else if (y >= 128)
    {
        new_frame = BIKE_FRAME_APPROACH4;
        new_index = 4;
    }
    else if (y >= 112)
    {
        new_frame = BIKE_FRAME_APPROACH5;
        new_index = 5;
    }
    else if (y >= 102)
    {
        new_frame = BIKE_FRAME_APPROACH6;
        new_index = 6;
    }
    else if (y >= 96)
    {
        new_frame = BIKE_FRAME_APPROACH7;
        new_index = 7;
    }    
    else if (y >= 88)
    {
        new_frame = BIKE_FRAME_APPROACH8;
        new_index = 8;
    }       
    else   
    {
        // turn off sprites
        Sprites_ClearLower();
        Sprites_ClearHigher();
        new_frame = -1;
        new_index = -1;
    }
    
    approach_frame_index = new_index;
    MotorBike_SetFrame(new_frame);
}

WORD MotorBike_GetVibrationOffset(void)
{
    // Only vibrate for frames 2 and beyond (smaller/distant sprites)
    if (approach_frame_index < 2)
    {
        return 0;
    }
    
    // Alternate between -1, 0, +1 for subtle vibration
    vibration_counter++;
    
    switch (vibration_counter % 2)
    {
        case 0: return -1;
        case 1: return 0;
        case 2: return 1;
        case 3: return 0;
        default: return 0;
    }
}

void MotorBike_AutoAccelerate(void)
{
    // Auto-accelerate to minimum cruising speed (42 mph)
    if (bike_speed < MIN_CRUISING_SPEED)
    {
        bike_speed += ACCEL_RATE;
        if (bike_speed > MIN_CRUISING_SPEED)
        {
            bike_speed = MIN_CRUISING_SPEED;
        }
        bike_state = BIKE_STATE_ACCELERATING;
    }
    else
    {
        bike_state = BIKE_STATE_MOVING;
    }
}

void MotorBike_DecelerateToCruising(void)
{
    // Decelerate back to cruising speed (42 mph)
    if (bike_speed > MIN_CRUISING_SPEED)
    {
        bike_speed -= DECEL_RATE;
        if (bike_speed < MIN_CRUISING_SPEED)
        {
            bike_speed = MIN_CRUISING_SPEED;
        }
        bike_state = BIKE_STATE_BRAKING;
    }
}

CollisionState MotorBike_CheckCollision(int *hit_car_index)
{
    // Convert bike sprite coords to playfield coords
    WORD bike_screen_x = bike_position_x;
    WORD bike_screen_y = bike_position_y - g_sprite_voffset;
    WORD bike_width = 16;
    WORD bike_height = 32;
    
    // Check car collisions
    extern BlitterObject car[MAX_CARS];  // Access car array
    
    for (int i = 0; i < MAX_CARS; i++)
    {
        if (!car[i].visible || car[i].off_screen) continue;
        
        WORD car_screen_y = car[i].y - mapposy;
        WORD car_screen_x = car[i].x;
        WORD car_width = 32;
        WORD car_height = 32;
        
        if (bike_screen_x < car_screen_x + car_width &&
            bike_screen_x + bike_width > car_screen_x &&
            bike_screen_y < car_screen_y + car_height &&
            bike_screen_y + bike_height > car_screen_y)
        {
            *hit_car_index = i;
            return COLLISION_TRAFFIC;
        }
    }
    
    // Check offroad collisions (edge of road or medians)
    if (bike_position_x < 32 || bike_position_x > 288)
    {
        *hit_car_index = -1;
        return COLLISION_OFFROAD;
    }
    
    *hit_car_index = -1;
    return COLLISION_NONE;
}