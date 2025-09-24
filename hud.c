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
#include "copper.h"
#include "memory.h"
#include "sprites.h"
#include "hud.h"
#include "font.h"

#define VALUE 1

HUDSprites hud_sprites;

void InitHUDSprites(void)
{
    // Allocate sprite data for 4 sprites
    // Each sprite: 2 words control + (height * 2 words data) + 2 words end
    int sprite_size = (HUD_HEIGHT + 2) * 2;  // Control + data + end words
    
    for (int i = 0; i < 4; i++) 
    {
        hud_sprites.sprite_data[i] = (UWORD*)Mem_AllocChip(sprite_size * sizeof(UWORD));

        if (!hud_sprites.sprite_data[i]) 
        {
            KPrintF("Failed to allocate HUD sprite data");
        }
        
        // Initialize sprite data
        InitSpriteData(hud_sprites.sprite_data[i], HUD_START_X + (i * 16), 44, HUD_HEIGHT);
    }

    LoadFontSheet("font/zippyfont.BPL");
}

void InitSpriteData(UWORD *sprite_data, UWORD x, UWORD y, UWORD height)
{
    // Set sprite position and size (first 2 words)
    SetSpritePosition(sprite_data, x, y, y + height - 1);
    
    // Clear sprite graphics data (all transparent)
    for (int line = 0; line < height; line++) 
    {
        sprite_data[2 + line * 2] = 0;      // Plane A
        sprite_data[2 + line * 2 + 1] = 0;  // Plane B
    }
    
    // End marker
    sprite_data[2 + height * 2] = 0;
    sprite_data[2 + height * 2 + 1] = 0;
}

void SetHUDSpritePointers(void)
{
    // Set sprites 4-7 to point to HUD data

    ULONG sprite_addr = (ULONG)hud_sprites.sprite_data[0];
    SetSpritePointers(&sprite_addr,1, SPRITEPTR_FOUR);
    sprite_addr = (ULONG)hud_sprites.sprite_data[1];
    SetSpritePointers(&sprite_addr,1, SPRITEPTR_FIVE); 
    sprite_addr = (ULONG)hud_sprites.sprite_data[2];
    SetSpritePointers(&sprite_addr,1, SPRITEPTR_SIX);
    sprite_addr = (ULONG)hud_sprites.sprite_data[3];   
    SetSpritePointers(&sprite_addr,1, SPRITEPTR_SEVEN);   
}

void SetHUDWhite(void)
{
    Copper_SetSpritePalette(9, 0xFFF);   // Color 25 - White (sprites 4&5)
    Copper_SetSpritePalette(13, 0xFFF);  // Color 29 - White (sprites 6&7)  
}
 
void DrawCharToSprite(UWORD *sprite_data, char c, int x, int y)
{
    UBYTE char_data[8];

    GetFontChar(c, char_data);
    
    for (int row = 0; row < 8; row++) 
    {
        int sprite_line = y + row;
        if (sprite_line >= HUD_HEIGHT) break;
        
        UBYTE font_row = char_data[row];
        UWORD plane_a = 0, plane_b = 0;
        
        for (int bit = 0; bit < 8; bit++) 
        {
            if (font_row & (0x80 >> bit)) 
            {
                // Use same bitplane pattern for all sprites to get consistent color
                plane_a |= (0x8000 >> (x + bit));  // Set plane A
                plane_b |= 0;                       // Clear plane B
                // This gives color index "01" which maps to colors 25, 27, 29, 31
            }
        }
        
        sprite_data[2 + sprite_line * 2] |= plane_a;
        sprite_data[2 + sprite_line * 2 + 1] |= plane_b;
    }
}

BOOL IsValidChar(char c)
{
    return TRUE;
}

void DrawHUDText(char *text, int sprite_col, int y_offset)
{
    UWORD *sprite = hud_sprites.sprite_data[sprite_col];

    int char_x = 0;  // Horizontal position within the 16-pixel sprite
    
    for (int i = 0; text[i] && char_x + 8 <= 16; i++) 
    {
        if (IsValidChar(text[i])) 
        {
            DrawCharToSprite(sprite, text[i], char_x, y_offset);
            char_x += 8;  // Move 8 pixels right for next character
        }
        // Invalid characters are simply skipped
    }
}

void UpdateScore(ULONG score)
{

}

void PreDrawHUD()
{
    SetHUDWhite();
    DrawHUDString("HI-SCORE", 0, 0);
    DrawHUDString("00", 2, 8);
    DrawHUDString("1UP", 0, 16);
    DrawHUDString("00", 2, 24);

    DrawHUDString(NEW_YORK , 3 , 48);
    DrawHUDString(CHICAGO , 3 , 64);
    DrawHUDString(ST_LOUIS , 3 , 84);
    DrawHUDString(HOUSTON , 3 , 104);
    DrawHUDString(LASVEGAS , 3 , 124);
    DrawHUDString(LASANGELES , 3 , 144);

    DrawHUDString("RANK", 1, 160);
    DrawHUDString("SPEED", 1, 184);
}

void DrawHUDString(char *text, int start_sprite, int y_offset)
{
    int text_len = strlen(text);
    int sprite_index = start_sprite;
    int char_pos = 0;
 
    while (char_pos < text_len && sprite_index < 4) 
    {
        char sprite_text[3] = {0}; // 2 chars per sprite + null terminator
        int chars_in_sprite = 0;
        
        // Fill current sprite with up to 2 characters
        while (chars_in_sprite < 2 && char_pos < text_len) 
        {
            if (IsValidChar(text[char_pos])) 
            {
                sprite_text[chars_in_sprite] = text[char_pos];
                chars_in_sprite++;
            }
            char_pos++;
        }
        
        // Draw to current sprite if we have characters
        if (chars_in_sprite > 0) 
        {
            DrawHUDText(sprite_text, sprite_index, y_offset);
        }
        
        sprite_index++;
    }
}

void SetHUDSpritePositions(void)
{
    for (int i = 0; i < 4; i++) 
    {
        UWORD logical_x = 192 + (i * 16);  // Where you want it on screen
        UWORD sprite_x = logical_x + 128;  // Actual sprite coordinate
        
        SetSpritePosition(hud_sprites.sprite_data[i], sprite_x, 44, 44 + HUD_HEIGHT - 1);
    }
    
    SetHUDSpritePointers();
}