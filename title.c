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
#include "hud.h"
#include "title.h"
#include "roadsystem.h"
#include "motorbike.h"
#include "timers.h"
#include "hiscore.h"
#include "font.h"
#include "city_approach.h"
 
#define ZIPPY_LOGO_WIDTH 80
#define ZIPPY_LOGO_HEIGHT 48

#define ATTRACTMODE_TILES_WIDTH 256
#define ATTRACTMODE_TILES_HEIGHT 512
#define NUM_ROAD_FRAMES 14
 
extern volatile struct Custom *custom;

BlitterObject zippy_logo;
 
APTR zippy_logo_restore_ptrs[4];
APTR *zippy_logo_restore_ptr;

static UWORD title_frames = 0;
static UWORD title_flash_counter = 0;
 
RawMap *city_attract_map;
BitMapEx *city_attract_tiles;

GameTimer attract_timer;
GameTimer logo_flash_timer;
GameTimer blink_timer;
GameTimer hiscore_timer;    // timer for high score table display

BOOL text_visible = TRUE;

UBYTE title_state = TITLE_ATTRACT_INIT;
 
void Title_LoadSprites()
{
    zippy_logo.data = Disk_AllocAndLoadAsset(ZIPPY_LOGO, MEMF_CHIP);
}

void Title_Initialize(void)
{
    // Load Zippy Race Logo Bob

    zippy_logo.background = Mem_AllocChip((ZIPPY_LOGO_WIDTH/8) * ZIPPY_LOGO_HEIGHT * 4);
    zippy_logo.background2 = Mem_AllocChip((ZIPPY_LOGO_WIDTH/8) * ZIPPY_LOGO_HEIGHT * 4);
    zippy_logo.visible = TRUE;
    zippy_logo.off_screen = FALSE;

    Title_LoadSprites();
 
    // Attract Mode Tiles/Tilemap
    Title_OpenMap();
    Title_OpenBlocks();

    // Palette
    Game_LoadPalette("palettes/intro.pal", intro_colors, BLOCKSCOLORS);
    Game_ApplyPalette(intro_colors,BLOCKSCOLORS);

    title_state = TITLE_ATTRACT_INIT;
    
    // Reset Positions
    Title_Reset();

    title_state = TITLE_ATTRACT_START;
}

void Title_Draw()
{
    if (title_state == TITLE_ATTRACT_START)
    {
        if (title_frames == 0)
        {
            Copper_SetPalette(0, 0x000);

            BlitClearScreen(draw_buffer, 320 << 6 | 16);
            BlitClearScreen(display_buffer, 320 << 6 | 16);

            // So we pre-draw the city skyline ones
            // then on update we only draw the tiles being replaced
      

            City_PreDrawRoad();
            Title_SaveBackground();
            // Draw the hud
            HUD_DrawAll();

            #if defined (DEBUG)
            char line_buffer[8] = {0};
            ULongToString(Mem_GetFreeChip(), line_buffer, 8, ' ');
            Font_DrawString(draw_buffer, line_buffer, 32, 170, 9);
            ULongToString(Mem_GetFreeFast(), line_buffer, 8, ' ');
            Font_DrawString(draw_buffer, line_buffer, 32, 180, 10);
            #endif

            title_state = TITLE_ATTRACT_ACCEL;            

        }
    }
 
    if (title_state < TITLE_ATTRACT_INSERT_COIN)
    {
        City_DrawRoad();
    }
    
    if (title_state == TITLE_ATTRACT_LOGO_DROP)
    { 
        // Blit Logo
        Title_RestoreLogo();
        Title_SaveBackground();
        Title_BlitLogo();
 
        if (Timer_HasElapsed(&logo_flash_timer) == FALSE)
        {
            if (title_flash_counter % 5 == 0) 
            {          
                Copper_SwapColors(7, 12);
            }
        }
        else
        {
            Timer_Stop(&logo_flash_timer);
            Timer_Start(&hiscore_timer,10);     // 10 Seconds 
            title_state = TITLE_ATTRACT_INSERT_COIN;

            BlitClearScreen(draw_buffer, 320 << 6 | 64);
            BlitClearScreen(display_buffer, 320 << 6 | 64);

            Title_BlitLogo();

            AttractMode_DrawText();
 
        }   
    }
    else if (title_state == TITLE_ATTRACT_INSERT_COIN)
    {
        if (Timer_HasElapsed(&hiscore_timer) == TRUE)
        {
            Timer_Reset(&hiscore_timer);
            title_state = TITLE_ATTRACT_HIGHSCORE;

            BlitClearScreen(draw_buffer, 320 << 6 | 64);
            BlitClearScreen(display_buffer, 320 << 6 | 64);
            AttractMode_ShowHiScores();
           
        }
 
    }  
 
    title_frames++;
    title_flash_counter++;
}

void AttractMode_DrawText(void)
{
    Font_DrawStringCentered(draw_buffer, "PRESS FIRE BUTTON", 120, 12);  // Color 15 = white

    Font_DrawString(draw_buffer, "1983 IREM CORP", 8, 188, 5);         
    Font_DrawString(draw_buffer, "2026 AMIGA PORT BY MVG", 8, 198, 4);       
}

void AttractMode_ShowHiScores(void)
{
    // Draw high score table starting at Y=10
    HiScore_Draw(draw_buffer, 10, 12);  // Color 15 = white
}

void AttractMode_UpdateHiScores(void)
{
     if (Timer_HasElapsed(&hiscore_timer) == TRUE)
     {
        title_state = TITLE_ATTRACT_INSERT_COIN;
        Timer_Reset(&hiscore_timer);

        BlitClearScreen(draw_buffer, 320 << 6 | 64);
        BlitClearScreen(display_buffer, 320 << 6 | 64);

        Title_BlitLogo();

        AttractMode_DrawText();
     }
}

void AttractMode_Update(void)
{
    if (!Timer_IsActive(&blink_timer))
    {
        Timer_StartMs(&blink_timer, 500);  // Blink every 500ms
    }
    
    if (Timer_HasElapsed(&blink_timer))
    {
        text_visible = !text_visible;
        
        if (text_visible)
        {
            Font_DrawStringCentered(draw_buffer, "PRESS FIRE BUTTON", 120, 12);  // Color 15 = white
        }
        else
        {
            Font_ClearArea(draw_buffer, 0, 120, VIEWPORT_WIDTH, 8);
        }
        
        Timer_Reset(&blink_timer);
    }
 
}


void Title_Update()
{
 
    if (title_state == TITLE_ATTRACT_ACCEL)
    {
        AccelerateMotorBike();
        UpdateRoadScroll(bike_speed, title_frames);

        if (bike_speed >= MAX_SPEED)
        {
            if (Timer_IsActive(&attract_timer) == FALSE)
            {
                Timer_Start(&attract_timer, 3);  // 2 second countdown
            }
        }
 
        if (Timer_IsActive(&attract_timer) == TRUE)
        {
            BYTE remaining = Timer_GetRemainingSeconds(&attract_timer);
         
            if (Timer_HasElapsed(&attract_timer))
            {
                Timer_Stop(&attract_timer);
                title_state = TITLE_ATTRACT_INTO_HORIZON;
            }     
        }
    }
    else if (title_state == TITLE_ATTRACT_INTO_HORIZON  || 
             title_state == TITLE_ATTRACT_LOGO_DROP)
    {
        // start decelerating
        BrakeMotorBike();

        if (title_state == TITLE_ATTRACT_INTO_HORIZON )
        {
            if (bike_speed <= 80)
            {
                // clamp it 

                bike_speed = 80;

                if (Timer_IsActive(&attract_timer) == FALSE)
                {
                    Timer_Start(&attract_timer, 5);  // 3 second countdown
                }
            }
        }

        UpdateRoadScroll(bike_speed, title_frames);

        if (Timer_IsActive(&attract_timer) == TRUE)
        {
            BYTE remaining = Timer_GetRemainingSeconds(&attract_timer);
         
            if (remaining == 4)
            {
                title_state = TITLE_ATTRACT_LOGO_DROP;
            }

            if (Timer_HasElapsed(&attract_timer))
            {
                Timer_Stop(&attract_timer);
            }     
        }

        if (title_state == TITLE_ATTRACT_LOGO_DROP)
        {
            
            if (title_frames % 4 == 0)
            {
                zippy_logo.y++;
            }

            if (zippy_logo.y >= 26)
            {
                zippy_logo.y = 26;

                if (Timer_IsActive(&logo_flash_timer) == FALSE)
                {
                    Timer_Start(&logo_flash_timer, 3);  // Logo Flash Timer
                }   
            }
        }
    }
    else if (title_state == TITLE_ATTRACT_INSERT_COIN)
    {
        AttractMode_Update();
    }
    else if (title_state == TITLE_ATTRACT_HIGHSCORE)
    {
        AttractMode_UpdateHiScores();
    } 

    if (road_tile_idx > NUM_ROAD_FRAMES)
    {
        road_tile_idx = 0;
    }
    else if (road_tile_idx < 0)
    {
        road_tile_idx = NUM_ROAD_FRAMES;
    }
 
}

void Title_Reset()
{
    zippy_logo.x = 64;
    zippy_logo.y = -32;
    zippy_logo.old_x = 64;   // Same as starting x
    zippy_logo.old_y = -32;  // Same as starting y
    road_tile_idx = 0;
    title_frames = 0;
 
}

void Title_BlitLogo()
{
    WORD x = zippy_logo.x;
    WORD y = zippy_logo.y;

    UWORD source_mod = 10;   // Width in bytes - skips the interleaved mask
    UWORD dest_mod = (320 - ZIPPY_LOGO_WIDTH) / 8;     // Screen modulo
    ULONG admod = ((ULONG)dest_mod << 16) | source_mod;
 
    UWORD bltsize = ((ZIPPY_LOGO_HEIGHT<<2) << 6) | ZIPPY_LOGO_WIDTH/16;
    
    UBYTE *source = (UBYTE*)&zippy_logo.data[0];
    
    // Mask is 10 bytes after image (interleaved)
    UBYTE *mask = source + ZIPPY_LOGO_WIDTH/8 * 1;  // +10 bytes
    
    // Destination
    UBYTE *dest = draw_buffer;
 
    // Use BlitBob2 for cookie-cutter masking
    BlitBob2(160, x, y, admod, bltsize, zippy_logo_restore_ptrs, source, mask, dest);
}

void Title_SaveBackground()
{
    WORD x = zippy_logo.x;
    WORD y = zippy_logo.y;

    APTR background = (current_buffer == 0) ? zippy_logo.background : zippy_logo.background2;
    
    UWORD source_mod = (320 - 80) / 8;   // Screen modulo: 30 bytes
    UWORD dest_mod = 0;                  // Background is tight-packed
    ULONG admod = ((ULONG)dest_mod << 16) | source_mod;
    
    UWORD bltsize = ((ZIPPY_LOGO_HEIGHT<<2) << 6) | ZIPPY_LOGO_WIDTH/16;
    
    UBYTE *source = draw_buffer;
    UBYTE *dest = background;
    
    BlitBobSimpleSave(160, x, y, admod, bltsize, source, dest);
}

void Title_RestoreLogo()
{
    // Restore at OLD position
    WORD old_x = zippy_logo.old_x;
    WORD old_y = zippy_logo.old_y;

    APTR background = (current_buffer == 0) ? zippy_logo.background : zippy_logo.background2;
    
    // Blit from background buffer to screen
    UWORD source_mod = 0;                  // Background: tight-packed
    UWORD dest_mod = (320 - ZIPPY_LOGO_WIDTH) / 8;      // Screen modulo: 30 bytes
    ULONG admod = ((ULONG)dest_mod << 16) | source_mod;
    
    UWORD bltsize = ((ZIPPY_LOGO_HEIGHT<<2) << 6) | ZIPPY_LOGO_WIDTH/16;
    
    UBYTE *source = background;             // Source at 0,0 in buffer
    UBYTE *dest = draw_buffer;              // Dest will be offset by function
    
    BlitBobSimple(160, old_x, old_y, admod, bltsize, source, dest);
    
    // Save current position for next restore
    zippy_logo.old_x = zippy_logo.x;
    zippy_logo.old_y = zippy_logo.y;

}

void Title_OpenMap(void)
{
   
	city_attract_map = Disk_AllocAndLoadAsset("maps/city_attract.map",MEMF_PUBLIC);
	
	mapdata = (UWORD *)city_attract_map->data;
	mapwidth = city_attract_map->mapwidth;
	mapheight = city_attract_map->mapheight;   
}
 
void Title_OpenBlocks(void)
{
	city_attract_tiles = BitMapEx_Create(BLOCKSDEPTH, ATTRACTMODE_TILES_WIDTH, ATTRACTMODE_TILES_HEIGHT+64);
	Disk_LoadAsset((UBYTE *)city_attract_tiles->planes[0],"tiles/city_attract.BPL");  
}
 