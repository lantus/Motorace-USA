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
#include "memory.h"
#include "blitter.h"
#include "roadsystem.h"
#include "motorbike.h"
#include "cars.h"

#define CAR_BG_SIZE 768
extern volatile struct Custom *custom;

BlitterObject car[MAX_CARS];
 
/* Restore pointers */
APTR restore_ptrs[MAX_CARS * 4];
APTR *restore_ptr;
 
void Cars_LoadSprites()
{
    car[0].data = Disk_AllocAndLoadAsset(CAR0_FRAME0, MEMF_CHIP);
    car[0].data_frame2 = Disk_AllocAndLoadAsset(CAR0_FRAME1, MEMF_CHIP);

    car[1].data = Disk_AllocAndLoadAsset(CAR1_FRAME0, MEMF_CHIP);
    car[1].data_frame2 = Disk_AllocAndLoadAsset(CAR1_FRAME1, MEMF_CHIP);

    car[2].data = Disk_AllocAndLoadAsset(CAR2_FRAME0, MEMF_CHIP);
    car[2].data_frame2 = Disk_AllocAndLoadAsset(CAR2_FRAME1, MEMF_CHIP);

    car[3].data = Disk_AllocAndLoadAsset(CAR3_FRAME0, MEMF_CHIP);
    car[3].data_frame2 = Disk_AllocAndLoadAsset(CAR3_FRAME1, MEMF_CHIP);

    car[4].data = Disk_AllocAndLoadAsset(CAR4_FRAME0, MEMF_CHIP);
    car[4].data_frame2 = Disk_AllocAndLoadAsset(CAR4_FRAME1, MEMF_CHIP);   
}

void Cars_Initialize(void)
{
    car[0].background = Mem_AllocChip(CAR_BG_SIZE);
    car[1].background = Mem_AllocChip(CAR_BG_SIZE);
    car[2].background = Mem_AllocChip(CAR_BG_SIZE);
    car[3].background = Mem_AllocChip(CAR_BG_SIZE);
    car[4].background = Mem_AllocChip(CAR_BG_SIZE);
  
    Cars_LoadSprites();
}
 
void Cars_ResetPositions(void)
{
    car[0].visible = TRUE;
    car[0].x = FAR_LEFT_LANE;
    car[0].y = mapposy + 180;   
    car[0].speed = 81;
    car[0].accumulator = 0;
    car[0].off_screen = FALSE;
    car[0].needs_restore = FALSE;
    car[0].anim_frame = 0;
    car[0].anim_counter = 0;
    car[0].has_blocked_bike = FALSE;
    car[0].block_timer = 0;

  
    car[1].visible = FALSE;
    car[1].x = CENTER_LANE;
    car[1].y = mapposy + 160;
    car[1].speed = 63;
    car[1].accumulator = 0;
    car[1].off_screen = FALSE;
    car[1].needs_restore = FALSE;
    car[1].anim_frame = 0;
    car[1].anim_counter = 0;
    car[1].has_blocked_bike = FALSE;
    car[1].block_timer = 0;
   
    car[2].visible = FALSE;
    car[2].x = FAR_RIGHT_LANE;
    car[2].y = mapposy + 140;
    car[2].speed = 61;
    car[2].accumulator = 0;
    car[2].off_screen = FALSE;
    car[2].needs_restore = FALSE;
    car[2].anim_frame = 0;
    car[2].anim_counter = 0;
    car[2].has_blocked_bike = FALSE;
    car[2].block_timer = 0;  
   
    car[3].visible = TRUE;
    car[3].x = CENTER_LANE;
    car[3].y = mapposy + 80;  // Behind bike
    car[3].speed = 64;
    car[3].accumulator = 0;
    car[3].off_screen = FALSE;
    car[3].needs_restore = FALSE;
    car[3].anim_frame = 0;
    car[3].anim_counter = 0;
    car[3].has_blocked_bike = FALSE;
    car[3].block_timer = 0;  
   
    car[4].visible = FALSE;
    car[4].x = FAR_RIGHT_LANE;
    car[4].y = mapposy + 100;
    car[4].speed = 68;
    car[4].accumulator = 0;
    car[4].off_screen = FALSE;
    car[4].needs_restore = FALSE;
    car[4].anim_frame = 0;
    car[4].anim_counter = 0;
    car[4].has_blocked_bike = FALSE;
    car[4].block_timer = 0;
}

 

void SaveCarBOB(BlitterObject *car)
{

    if (!car->visible) return;
    
    WORD screen_y = car->y - mapposy;
    if (screen_y < -8|| screen_y > SCREENHEIGHT + 8  ) 
    {
     //   car->off_screen = TRUE;
           
        return;
    }
    
    car->off_screen = FALSE;
    
    WORD x = car->x;
    WORD buffer_y = (videoposy + BLOCKHEIGHT + screen_y);
    
    if (buffer_y < 0 || buffer_y > (BITMAPHEIGHT ))
    {
        car->off_screen = TRUE;
        return;
    }
    
    // Detect if buffer_y jumped significantly (wrap happened)
    static WORD last_buffer_y = -1;
    if (last_buffer_y >= 0)
    {
        WORD y_diff = buffer_y - last_buffer_y;
        // If jumped > 200 lines, videoposy wrapped - skip restore this frame
        if (y_diff > 200 || y_diff < -200)
        {
            //KPrintF("WRAP DETECTED: last=%ld current=%ld, skipping restore\n", last_buffer_y, buffer_y);
            car->needs_restore = FALSE;
        }
    }
    last_buffer_y = buffer_y;
    
    // Calculate and STORE pointer
    ULONG y_offset = (ULONG)buffer_y * 160;
    WORD x_byte_offset = x >> 3;
    UBYTE *bitmap_ptr = screen.bitplanes + y_offset + x_byte_offset;
    
    car->restore.screen_ptr = bitmap_ptr;
    car->restore.background_ptr = car->background;
    
    UWORD bltsize = (128 << 6) | 3;

    WaitBlit();
    custom->bltcon0 = 0x9F0;
    custom->bltcon1 = 0;
    custom->bltamod = 34;
    custom->bltdmod = 0;
    custom->bltapt = bitmap_ptr;
    custom->bltdpt = car->background;
    custom->bltsize = bltsize;
    WaitBlit();  // Wait BEFORE starting
    car->needs_restore = TRUE;  // Only set flag AFTER successful save
}
 
void DrawCarBOB(BlitterObject *car)
{

    /* 32x32
    Barrel Shifting is a pain...
    bitplanes = 4
    words per scanline - 2 words (32/16)
              - 3 words with -AW padding
              
    Bytes per scanline = (48/8) = 6 bytes + 6 bytes mask
    Total scanline = 32

    BlitSize (32x4) << 6 | 3
 
    src mod = 3 words x 2 Bytes
    dst mod = 40 - 6 = 34;
    */

 
    if (!car->visible) return;
    
    WORD screen_y = car->y - mapposy;
    
    if (screen_y < -8 || screen_y > SCREENHEIGHT + 8) 
    {
        car->off_screen = TRUE;
        return;
    }
    
    car->off_screen = FALSE;
    
    WORD x = car->x;
    WORD buffer_y = (videoposy + BLOCKHEIGHT + screen_y);
    
    if (buffer_y < 0 || buffer_y > (BITMAPHEIGHT - 32))
    {
        car->off_screen = TRUE;
        return;
    }
  
    
    UWORD source_mod = 6;
    UWORD dest_mod = 34;
    ULONG admod = ((ULONG)dest_mod << 16) | source_mod;
    UWORD bltsize = (128 << 6) | 3;

 // Select animation frame
    UBYTE *source = (car->anim_frame == 0) ? 
                    (UBYTE*)&car->data[0] : 
                    (UBYTE*)&car->data_frame2[0];

    UBYTE *mask = source + 6;
    APTR car_restore_ptrs[4];

        // Calculate and STORE pointer
    ULONG y_offset = (ULONG)buffer_y * 160;
    WORD x_byte_offset = x >> 3;
    UBYTE *bitmap_ptr = draw_buffer + y_offset + x_byte_offset;
 
 
    BlitBob2(160, x, buffer_y, admod, bltsize, BOB_WIDTH, car_restore_ptrs, source, mask, draw_buffer);
    
}

void Cars_RestoreSaved()
{
   //Restore in reverse order  
    for (int i = MAX_CARS - 1; i >= 0; i--) 
    {
        if (car[i].needs_restore && !car[i].off_screen && car[i].restore.screen_ptr)
        {    
            UWORD bltsize = (128 << 6) | 3;
            
            WaitBlit();
            custom->bltcon0 = 0x9F0;
            custom->bltcon1 = 0;
            custom->bltamod = 0;
            custom->bltdmod = 34;
            custom->bltapt = car[i].restore.background_ptr;
            custom->bltdpt = car[i].restore.screen_ptr;
            custom->bltsize = bltsize;

            car[i].needs_restore = FALSE;
        }
    }
}

void Cars_PreDraw(void)
{
    
    for (int i = 0; i < MAX_CARS; i++)
    {
        if (car[i].visible && !car[i].off_screen)
        {
            // Use prev_old position (2 frames back)
            WORD temp_x = car[i].old_x;
            WORD temp_y = car[i].old_y;
            
            car[i].old_x = car[i].prev_old_x;
            car[i].old_y = car[i].prev_old_y;
            
            Cars_CopyPristineBackground(&car[i]);
            
            car[i].old_x = temp_x;
            car[i].old_y = temp_y;

            // Save position history
            car[i].prev_old_x = car[i].old_x;
            car[i].prev_old_y = car[i].old_y;
            
            car[i].old_x = car[i].x;
            car[i].old_y = car[i].y;
            
    
            DrawCarBOB(&car[i]);
        }
    }

}


void Cars_Update(void)
{
    for (int i = 0; i < MAX_CARS; i++)
    {
        if (!car[i].visible) continue;
        
        // Check if car should block bike
        Cars_CheckBikeOvertake(&car[i], bike_position_x);
        
        // Update and draw car
        Cars_Tick(&car[i]);
    }  
 
}

void Cars_CopyPristineBackground(BlitterObject *car)
{
    if (!car->needs_restore) return;
    
    WORD old_screen_y = car->old_y - mapposy;
    WORD buffer_y = (videoposy + BLOCKHEIGHT + old_screen_y);
    
    if (buffer_y < 0 || buffer_y > (BITMAPHEIGHT - 32))
        return;
    
    ULONG y_offset = (ULONG)buffer_y * 160;
    WORD x_byte_offset = car->old_x >> 3;
    
    // Calculate X but aligned to word boundary for full coverage
    WORD x_word_aligned = (car->old_x >> 4) << 1;  // Align to 16-pixel boundary
    
    UBYTE *pristine_ptr = screen.pristine + y_offset + x_word_aligned;
    UBYTE *screen_ptr = draw_buffer + y_offset + x_word_aligned;
    
    WaitBlit();
    custom->bltcon0 = 0x9F0;
    custom->bltcon1 = 0;
    custom->bltafwm = 0xFFFF;
    custom->bltalwm = 0xFFFF;
    custom->bltamod = 34;   
    custom->bltdmod = 34;   
    custom->bltapt = pristine_ptr;
    custom->bltdpt = screen_ptr;
    custom->bltsize = (128 << 6) | 3;  // 32 lines (128/4) x 3 words    
}
 

void Cars_HandleSpinout(UBYTE car_index)
{
    if (car_index < 0 || car_index >= MAX_CARS) return;
    
    BlitterObject *crashed_car = &car[car_index];
    
    // Mark car as crashed
    crashed_car->crashed = TRUE;
    crashed_car->crash_timer = 120;  // Stopped for 60 frames (1 second)
    
    // Decelerate car extremely quickly
    crashed_car->speed = 0;
    crashed_car->accumulator = 0;
}

void Cars_CheckLaneAndSteer(BlitterObject *car)
{
    if (!car->visible || car->crashed) return;
 
    
    // Debug counter - only output every 30 frames
    static int debug_frame = 0;
    debug_frame++;
    
    // Determine which car this is
    int car_index = car - &car[0];
    
    // Look ahead distance in pixels
    WORD lookahead = 32;  // Check 3 tiles ahead
    WORD check_y = car->y - lookahead;
    
    // Use CENTER of car sprite (32x32 sprite, so center is +16 in both X and Y)
    WORD center_x = car->x + 16;
    WORD center_y = check_y;
   
    // Check if current path is safe
    BOOL current_safe = TileAttrib_IsDrivable(center_x, center_y);
 
    if (!current_safe)
    {
        // Need to change lanes! Check left and right
        BOOL left_safe = TileAttrib_IsDrivable(center_x - 32, center_y);
        BOOL right_safe = TileAttrib_IsDrivable(center_x + 32, center_y);
        
        if (left_safe && !right_safe)
        {
            car->x -= 1;
            if (car->x < FAR_LEFT_LANE) car->x = FAR_LEFT_LANE;
   
        }
        else if (right_safe && !left_safe)
        {
            car->x += 1;
            if (car->x > FAR_RIGHT_LANE) car->x = FAR_RIGHT_LANE;
 
        }
        else if (left_safe && right_safe)
        {
            if (car->x < CENTER_LANE)
                car->x -= 1;
            else
                car->x += 1;
 
        }
        
    }
}

void Cars_UpdatePosition(BlitterObject *c)
{
    if (!c->visible) return;
    
    // Handle crash state
    if (c->crashed)
    {
        if (c->crash_timer > 0)
        {
            c->crash_timer--;
            c->speed = 0;
            return;
        }
        else
        {
            c->crashed = FALSE;
            c->speed = 20;
        }
    }
    
    c->old_x = c->x;
    c->old_y = c->y;
    c->moved = TRUE;
 
    // Normal movement
    WORD car_movement = -GetScrollAmount(c->speed);
    c->accumulator += car_movement;

    while (c->accumulator <= -256)
    {
        c->y--;
        c->accumulator += 256;
    }

    while (c->accumulator >= 256)
    {
        c->y++;
        c->accumulator -= 256;
    }

    
    // AI steering - check ahead and adjust lane
    Cars_CheckLaneAndSteer(c);

    // Animation
    c->anim_counter++;
    WORD frame_delay = (c->speed > 0) ? (100 / c->speed) : 10;
    if (frame_delay < 2) frame_delay = 2;
    
    if (c->anim_counter >= frame_delay)
    {
        c->anim_counter = 0;
        c->anim_frame = (c->anim_frame == 0) ? 1 : 0;
    }
}

void Cars_CheckBikeOvertake(BlitterObject *car, WORD bike_x)
{
    if (car->off_screen || car->crashed || car->has_blocked_bike) return;
    
    WORD car_screen_y = car->y - mapposy;
    
    // Only if car is ahead of bike (screen Y <= 192)
    if (car_screen_y > 192) return;
    
    WORD x_diff = ABS(car->x - bike_x);
    
    if (x_diff <= game_car_block_x_threshold)
    {
        car->block_timer++;
        
        if (car->block_timer >= game_car_block_move_rate)
        {
            car->block_timer = 0;
            
            // Move toward bike's X
            if (car->x < bike_x)
            {
                car->x += game_car_block_move_speed;
                if (car->x > bike_x) car->x = bike_x;
            }
            else if (car->x > bike_x)
            {
                car->x -= game_car_block_move_speed;
                if (car->x < bike_x) car->x = bike_x;
            }
            
            if (car->x == bike_x)
            {
                car->has_blocked_bike = TRUE;
                int car_index = car - &car[0];
            }
        }
    }
}

void Cars_Tick(BlitterObject *car)
{
    // Erase old position
    WORD temp_x = car->old_x;
    WORD temp_y = car->old_y;
    
    car->old_x = car->prev_old_x;
    car->old_y = car->prev_old_y;
    
    Cars_CopyPristineBackground(car);
    
    car->old_x = temp_x;
    car->old_y = temp_y;

    car->prev_old_x = car->old_x;
    car->prev_old_y = car->old_y;
    
    car->old_x = car->x;
    car->old_y = car->y;
    
    Cars_UpdatePosition(car);

    DrawCarBOB(car);
    car->needs_restore = TRUE;
}