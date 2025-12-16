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
#include "cars.h"

#define CAR_BG_SIZE 768
extern volatile struct Custom *custom;

BlitterObject car[MAX_CARS];
 
/* Restore pointers */
APTR restore_ptrs[MAX_CARS * 4];
APTR *restore_ptr;
 
void Cars_LoadSprites()
{
    car[0].data = Disk_AllocAndLoadAsset(CAR1_FILE, MEMF_CHIP);
    car[1].data = Disk_AllocAndLoadAsset(CAR1_FILE, MEMF_CHIP);
    car[2].data = Disk_AllocAndLoadAsset(CAR1_FILE, MEMF_CHIP);
    car[3].data = Disk_AllocAndLoadAsset(CAR1_FILE, MEMF_CHIP);
    car[4].data = Disk_AllocAndLoadAsset(CAR1_FILE, MEMF_CHIP);   
}

void Cars_Initialize(void)
{
  // Car 0 - Pole position (fast car, pulling ahead)
    car[0].background = Mem_AllocChip(CAR_BG_SIZE);
 
    // Car 1 - Middle left (matching pace)
    car[1].background = Mem_AllocChip(CAR_BG_SIZE);
     
    // Car 2 - Middle right (slower traffic)
    car[2].background = Mem_AllocChip(CAR_BG_SIZE);
     
    // Car 3 - Near left (fast competitor)
    car[3].background = Mem_AllocChip(CAR_BG_SIZE);
    
    // Car 4 - Near right (slow traffic)
    car[4].background = Mem_AllocChip(CAR_BG_SIZE);
  
    Cars_LoadSprites();
 
}
 
void Cars_ResetPositions(void)
{
    car[0].visible = TRUE;
    car[0].x = LEFT_LANE;
    car[0].y = mapposy + 200;  // Start at screen Y=100
    car[0].speed = 70;  // Same as cruising speed!
    car[0].accumulator = 0;
    car[0].off_screen = FALSE;
    car[0].needs_restore = FALSE;

    car[1].visible = FALSE;
    car[2].visible = FALSE;
    car[3].visible = FALSE;
    car[4].visible = FALSE;
 
}

void Cars_UpdatePosition(BlitterObject *c)
{
 if (!c->visible) return;

    c->old_x = c->x;
    c->old_y = c->y;
    c->moved = TRUE;

    // GetScrollAmount now returns fixed-point directly, no need to << 8
    WORD car_movement = -GetScrollAmount(c->speed);

    c->accumulator += car_movement;

    while (c->accumulator <= -256)
    {
        c->y--;  // Move forward in world space
        c->accumulator += 256;
    }

    while (c->accumulator >= 256)
    {
        c->y++;  // Move backward in world space
        c->accumulator -= 256;
    }
}

void SaveCarBOB(BlitterObject *car)
{

    if (!car->visible) return;
    
    WORD screen_y = car->y - mapposy;
    if (screen_y < -100 || screen_y > SCREENHEIGHT + 100) 
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
    
    if (screen_y < -100 || screen_y > SCREENHEIGHT + 100) 
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
    
    UBYTE *source = (UBYTE*)&car->data[0];
    UBYTE *mask = source + 6;
    APTR car_restore_ptrs[4];
    
    WaitBlit();
    BlitBob2(160, x, buffer_y, admod, bltsize, BOB_WIDTH, car_restore_ptrs, source, mask, screen.bitplanes);

}

void Cars_RestoreSaved()
{
     
    for (int i = 0; i < MAX_CARS; i++) 
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
    // Draw static cars  
    for (int i = 0; i < MAX_CARS; i++)
    {
        if (car[i].visible)
        {
            DrawCarBOB(&car[i]);
        }
    }
}


// Main BOB update function
void Cars_Update(void)
{
  // Second pass: Update positions for all cars
  for (int i = 0; i < MAX_CARS; i++) 
  {
        if (car[i].visible) 
        {
            Cars_UpdatePosition(&car[i]);
            SaveCarBOB(&car[i]);
        }
  }

    // Third pass: Draw all cars (BlitBob handles background saving automatically)
    for (int i = 0; i < MAX_CARS; i++)
    {
        if (car[i].visible && !car[i].off_screen) 
        {
            DrawCarBOB(&car[i]);   
        }
    }
}
 