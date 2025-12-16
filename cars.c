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
    car[0].visible = TRUE;
    car[0].x = FAR_LEFT_LANE;
    car[0].y = 20;
    car[0].off_screen = FALSE;
    car[0].needs_restore = FALSE;
    car[0].speed = 42;  // Faster than cruising (42) - will pull ahead
    car[0].accumulator = 0;

    // Car 1 - Middle left (matching pace)
    car[1].background = Mem_AllocChip(CAR_BG_SIZE);
    car[1].visible = TRUE;
    car[1].x = LEFT_LANE;
    car[1].y = 80;
    car[1].off_screen = FALSE;
    car[1].needs_restore = FALSE;
    car[1].speed = 42;  // Cruising speed - stays in place relative to bike
    car[1].accumulator = 0;
  
    // Car 2 - Middle right (slower traffic)
    car[2].background = Mem_AllocChip(CAR_BG_SIZE);
    car[2].visible = TRUE;
    car[2].x = CENTER_LANE;
    car[2].y = 100;
    car[2].off_screen = FALSE;
    car[2].needs_restore = FALSE;
    car[2].speed = 41;  // Slower - will fall behind
    car[2].accumulator = 0;
    
    // Car 3 - Near left (fast competitor)
    car[3].background = Mem_AllocChip(CAR_BG_SIZE);
    car[3].visible = TRUE;
    car[3].x = RIGHT_LANE;
    car[3].y = 140;
    car[3].off_screen = FALSE;
    car[3].needs_restore = FALSE;
    car[3].speed = 41;  // Faster - will overtake
    car[3].accumulator = 0;
      
    // Car 4 - Near right (slow traffic)
    car[4].background = Mem_AllocChip(CAR_BG_SIZE);
    car[4].visible = TRUE;
    car[4].x = FAR_RIGHT_LANE;
    car[4].y = 170;
    car[4].off_screen = FALSE;
    car[4].needs_restore = FALSE;
    car[4].speed = 41;  // Much slower - obstacle to avoid
    car[4].accumulator = 0;

    Cars_LoadSprites();
 
}
 
void Cars_UpdatePosition(BlitterObject *c)
{
  if (!c->visible) return;

  c->old_x = c->x;
  c->old_y = c->y;
  c->moved = TRUE;

  WORD old_world_y = c->y;

  // Car moves FORWARD in world space based on its own speed

  WORD car_movement = -(GetScrollAmount(c->speed) << 8);

  c->accumulator += car_movement;

  // Apply movement (negative = move forward/up, positive = move backward/down)
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
    if (!car->visible ) return;
    
    if (car->x < 0 || car->x > 160 || car->y < -32 || car->y > SCREENHEIGHT + 32) 
    {
        return;
    }
 
    // PARAMETERS FOR 32x32 CAR BOB:

    // Position
    WORD x = car->x;
    WORD y = car->y;

    // Modulos - Optimize these later
    UWORD dest_mod = 4; 
    UWORD source_mod =  (320 - 32) / 8;

    ULONG admod = ((ULONG)dest_mod << 16) | source_mod;

    UWORD bltsize = (128 << 6) | 2;

    // Source data
    UBYTE *source = (UBYTE*)draw_buffer;

    // Mask data
    UBYTE *mask = source + 32 / 8  * 1;

    // Destination
    UBYTE *dest = car->background;
 
    BlitBobSave(160, x , y ,  admod, bltsize, restore_ptr, &restore_ptrs[0], source, mask, dest);
 
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
    
    if (car->x < 0 || car->x > 160 || car->y < -32 || car->y > SCREENHEIGHT + 32) 
    {
        return;
    }
 
    // Position
    WORD x = car->x;
    WORD y = car->y;

    UWORD source_mod = 6;  // Tight-packed source
    UWORD dest_mod = 34;  // Screen modulo
    ULONG admod = ((ULONG)dest_mod << 16) | source_mod;
 
    UWORD bltsize = (128 << 6) | 3;   
    
    UBYTE *source = (UBYTE*)&car->data[0];
    UBYTE *mask = source + 6;  // Mask follows image data
    UBYTE *dest = draw_buffer;
 
    // Use cookie-cutter blitting with barrel shift (like logo)
    APTR car_restore_ptrs[4];
    //BlitBob2(160, x, y, admod, bltsize, BOB_WIDTH, car_restore_ptrs, source, mask, dest);
   
    // Draw to BOTH buffers to avoid tearing during buffer swap
    // Draw to first buffer - both halves
    BlitBob2(160, x, y, admod, bltsize, BOB_WIDTH, car_restore_ptrs, source, mask, screen.bitplanes);
    BlitBob2(160, x, y + HALFBITMAPHEIGHT * BLOCKSDEPTH, admod, bltsize, BOB_WIDTH, car_restore_ptrs, source, mask, screen.bitplanes);
    
  
    car->needs_restore = TRUE;
}

void Cars_RestoreSaved()
{
      // First pass: Restore backgrounds from previous frame using BlitBob's restore pointers
      
    for (int i = 0; i < MAX_CARS; i++) 
    {
      if (car[i].needs_restore && !car[i].off_screen)
      {

            restore_ptr = &restore_ptrs[0];
            UWORD source_mod = 0; 
            UWORD dest_mod = (320 - 32) / 8;
            ULONG admod = ((ULONG)dest_mod << 16) | source_mod;
            UWORD bltsize = (128 << 6) | 2;
 
            ULONG y_offset = car[i].y * 160;
            WORD x_byte_offset = car[i].x >> 3;
            ULONG total_offset = y_offset + x_byte_offset;

            restore_ptr[1] = car[i].background;
            
            // Restore to BOTH buffers
            restore_ptr[0] = screen.bitplanes + total_offset;
            BlitRestoreBobs(admod, bltsize, 1, restore_ptr);
            
            restore_ptr[0] = screen.bitplanes + total_offset + (HALFBITMAPHEIGHT * BLOCKSDEPTH * 160);
            BlitRestoreBobs(admod, bltsize, 1, restore_ptr);
 
       
      }

    car[i].needs_restore = FALSE;  // Clear restore flag

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
            
            // Check if car went off screen (top)
            if (car[i].y < -32)
            {
                car[i].off_screen = TRUE;
                car[i].visible = FALSE;  // Could respawn later
            }
            // Check if car went off screen (bottom)
            else if (car[i].y > SCREENHEIGHT + 32)
            {
                car[i].off_screen = TRUE;
                car[i].visible = FALSE;
            }
            else
            {
                car[i].off_screen = FALSE;
                SaveCarBOB(&car[i]);
            }
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
 