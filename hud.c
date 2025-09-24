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

    for (int i = 0; i < 4; i++) 
    {
        ULONG sprite_addr = (ULONG)hud_sprites.sprite_data[i];
        
        switch(i) 
        {
            case 0: // Sprite 4
                CopSPR4PTH[VALUE] = sprite_addr >> 16;
                CopSPR4PTL[VALUE] = sprite_addr & 0xFFFF;
                break;
            case 1: // Sprite 5
                CopSPR5PTH[VALUE] = sprite_addr >> 16;
                CopSPR5PTL[VALUE] = sprite_addr & 0xFFFF;
                break;
            case 2: // Sprite 6
                CopSPR6PTH[VALUE] = sprite_addr >> 16;
                CopSPR6PTL[VALUE] = sprite_addr & 0xFFFF;
                break;
            case 3: // Sprite 7
                CopSPR7PTH[VALUE] = sprite_addr >> 16;
                CopSPR7PTL[VALUE] = sprite_addr & 0xFFFF;
                break;
        }
    }
}

void SetHUDSpriteColors(void)
{
    // Sprites 4&5 colors (24-27)
    Copper_SetSpritePalette(8, 0x000);   // Color 24 - Black
    Copper_SetSpritePalette(9, 0xFFF);   // Color 25 - White  
    Copper_SetSpritePalette(10, 0xF80);  // Color 26 - Orange
    Copper_SetSpritePalette(11, 0x0F0);  // Color 27 - Green
    
    // Sprites 6&7 colors (28-31)  
    Copper_SetSpritePalette(12, 0x00F);  // Color 28 - Blue
    Copper_SetSpritePalette(13, 0xFF0);  // Color 29 - Yellow
    Copper_SetSpritePalette(14, 0xF0F);  // Color 30 - Magenta
    Copper_SetSpritePalette(15, 0x888);  // Color 31 - Gray
}
 
void DrawCharToSprite(UWORD *sprite_data, char c, int x, int y)
{
   UBYTE char_data[8];
    GetFontChar(c, char_data);
    
    for (int row = 0; row < 8; row++) {
        int sprite_line = y + row;
        if (sprite_line >= HUD_HEIGHT) break;
        
        UBYTE font_row = char_data[row];
        UWORD plane_a = 0, plane_b = 0;
        
        for (int bit = 0; bit < 8; bit++) {
            if (font_row & (0x80 >> bit)) {
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
    return (c == ' ' ||        // Space (32)
            c == '-' ||        // Hyphen (45)
            (c >= '0' && c <= '9') ||  // Digits (48-57)
            (c >= 'A' && c <= 'Z') ||  // Uppercase (65-90)
            (c >= 'a' && c <= 'z'));   // Lowercase (97-122)
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

void DrawHUD()
{
   
}

void PreDrawHUD()
{
    DrawHUDString("SCORE", 0, 10);  
}

void DrawHUDString(char *text, int start_sprite, int y_offset)
{
    int text_len = strlen(text);
    int sprite_index = start_sprite;
    int char_pos = 0;
 
    while (char_pos < text_len && sprite_index < 4) {
        char sprite_text[3] = {0}; // 2 chars per sprite + null terminator
        int chars_in_sprite = 0;
        
        // Fill current sprite with up to 2 characters
        while (chars_in_sprite < 2 && char_pos < text_len) {
            if (IsValidChar(text[char_pos])) {
                sprite_text[chars_in_sprite] = text[char_pos];
                chars_in_sprite++;
            }
            char_pos++;
        }
        
        // Draw to current sprite if we have characters
        if (chars_in_sprite > 0) {
            DrawHUDText(sprite_text, sprite_index, y_offset);
        }
        
        sprite_index++;
    }
}