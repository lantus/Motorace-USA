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
    // Car 0 - Pole position (ahead, slightly left of center)
   // car[0].bob = BitMapEx_Create(BLOCKSDEPTH, BOB_WIDTH, BOB_HEIGHT);
    car[0].background = Mem_AllocChip(CAR_BG_SIZE);
    car[0].visible = TRUE;
    car[0].x = 8;  // Slightly left of center
    car[0].y = 20;         // Higher up
    car[0].off_screen = FALSE;
    car[0].needs_restore = FALSE;

    // Car 1 - Middle left (competitor with #1)
  //  car[1].bob = BitMapEx_Create(BLOCKSDEPTH, BOB_WIDTH, BOB_HEIGHT);
    car[1].background = Mem_AllocChip(CAR_BG_SIZE);
    car[1].visible = TRUE;
    car[1].x = 12;   // Further left
    car[1].y = 80;
    car[1].off_screen = FALSE;
    car[1].needs_restore = FALSE;
  
    // Car 2 - Middle right (competitor with #2)
   // car[2].bob = BitMapEx_Create(BLOCKSDEPTH, BOB_WIDTH, BOB_HEIGHT);
    car[2].background = Mem_AllocChip(CAR_BG_SIZE);
    car[2].visible = TRUE;
    car[2].x = 16;  // Further right
    car[2].y = 100;
    car[2].off_screen = FALSE;
    car[2].needs_restore = FALSE;
    
    // Car 3 - Near left (beside/slightly behind player)
   // car[3].bob = BitMapEx_Create(BLOCKSDEPTH, BOB_WIDTH, BOB_HEIGHT);
    car[3].background = Mem_AllocChip(CAR_BG_SIZE);
    car[3].visible = TRUE;
    car[3].x = 20;
    car[3].y = 140;
    car[3].off_screen = FALSE;
    car[3].needs_restore = FALSE;
      
    // Car 4 - Near right (beside/slightly behind player)
   // car[4].bob = BitMapEx_Create(BLOCKSDEPTH, BOB_WIDTH, BOB_HEIGHT);
    car[4].background = Mem_AllocChip(CAR_BG_SIZE);
    car[4].visible = TRUE;
    car[4].x = 24;
    car[4].y = 170;
    car[4].off_screen = FALSE;
    car[4].needs_restore = FALSE;

    Cars_LoadSprites();
 
}
 
void Cars_UpdatePosition(BlitterObject *c)
{
    if (!c->visible) return;
    
    // Store old position for background restore
    c->old_x = c->x;
    c->old_y = c->y;
    c->moved = TRUE;
    
    // Move car down the screen (towards player)
    c->y += 2;  // Move 2 pixels down per frame
    
    // Wrap around when car goes off bottom of screen
    if (c->y > SCREENHEIGHT ) 
    {
        c->y = 0;  // Start above screen
        
        // Randomize X position within road bounds (16-160 pixels)
        // Simple pseudo-random using car's current position
        //c->x =  ((c->x * 17 + c->y * 13) % 128);
        
        // Keep within road boundaries
        if (c->x < 16) c->x = 16;
        if (c->x > 160) c->x = 160;
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
    BlitBob2(160, x, y, admod, bltsize, BOB_WIDTH, car_restore_ptrs, source, mask, dest);
   
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
        UWORD dest_mod =  (320 - 32) / 8;
        ULONG admod = ((ULONG)dest_mod << 16) | source_mod;
        UWORD bltsize = (128 << 6) | 2;
 
        ULONG y_offset =  car[i].y * 160;
        WORD x_byte_offset = car[i].x >> 3;  // Divide by 8 for byte offset
        ULONG total_offset = y_offset + x_byte_offset;

        restore_ptr[1] = car[i].background;
        restore_ptr[0] = draw_buffer + total_offset ;
        
        BlitRestoreBobs(admod,bltsize,1,restore_ptr);
       
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
 