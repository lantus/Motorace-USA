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
#include "memory.h"
#include "sprites.h"
#include "game.h"
#include "font.h"

FontSheet game_font;
 
BOOL Font_LoadSheet(const char *filename)
{
    BPTR font_handle = Open(filename, MODE_OLDFILE);
    if (!font_handle) {
        KPrintF("Failed to open font file: %s\n", filename);
        return FALSE;
    }
    
    // Assuming font sheet is 128x80 pixels (16x16 characters)
    LONG font_size = (128 * 80 / 8) * 2;  // 2 bitplanes
    
    game_font.font_data = (UBYTE*)Mem_AllocChip(font_size);
    if (!game_font.font_data) {
        Close(font_handle);
        return FALSE;
    }
    
    if (Read(font_handle, game_font.font_data, font_size) != font_size) 
    {
        Mem_Free(game_font.font_data, font_size);
        Close(font_handle);
        return FALSE;
    }
    
    game_font.chars_per_row = 16;
    game_font.char_width = 8;
    game_font.char_height = 8;
    game_font.bitplanes = 2;
    
    Close(font_handle);
    return TRUE;
}

void Font_GetChar(char c, UBYTE *char_data)
{
    UBYTE char_index = (UBYTE)c;
    
    // Calculate position in font sheet
    UBYTE char_x = char_index % game_font.chars_per_row;
    UBYTE char_y = char_index / game_font.chars_per_row;
    
    UWORD sheet_width_bytes = (game_font.chars_per_row * game_font.char_width) / 8;
    
    // For 2 bitplanes, data is stored as plane 0, then plane 1
    ULONG plane_size = sheet_width_bytes * (game_font.chars_per_row * game_font.char_height);
    
    for (UBYTE row = 0; row < game_font.char_height; row++)
    {
        UWORD sheet_y = (char_y * game_font.char_height) + row;
        UWORD byte_offset = (sheet_y * sheet_width_bytes) + char_x;
        
        // Read from plane 0 (for now, can be extended for multi-color fonts)
        char_data[row] = game_font.font_data[byte_offset];
    }
}
 
// Draw single character to bitplane buffer
void Font_DrawChar(UBYTE *buffer, char c, UWORD x, UWORD y, UBYTE color)
{
    UBYTE char_data[8];
    Font_GetChar(c, char_data);
    
    if (x + CHAR_WIDTH > VIEWPORT_WIDTH || y + CHAR_HEIGHT > BITMAPHEIGHT)
        return;
    
    UWORD byte_offset = x >> 3;   
    
    for (int row = 0; row < CHAR_HEIGHT; row++)
    {
        UBYTE font_row = char_data[row];
 
        ULONG line_offset = ((ULONG)(y + row) << 6) + ((ULONG)(y + row) << 5);
        
        for (int bit = 0; bit < 8; bit++)
        {
            if (font_row & (0x80 >> bit))
            {
                UWORD pixel_x = x + bit;
                UWORD pixel_byte = pixel_x >> 3;
                UBYTE pixel_bit = 7 - (pixel_x & 7);
                UBYTE pixel_mask = 1 << pixel_bit;
                
     
                for (int plane = 0; plane < 4; plane++)
                {
                    ULONG plane_offset = line_offset + ((plane << 4) + (plane << 3)) + pixel_byte;
                    
                    if (color & (1 << plane))
                    {
                        buffer[plane_offset] |= pixel_mask;   // Set bit
                    }
                    else
                    {
                        buffer[plane_offset] &= ~pixel_mask;  // Clear bit
                    }
                }
            }
        }
    }
}

// Optimized version for byte-aligned characters (x is multiple of 8)
void Font_DrawCharAligned(UBYTE *buffer, char c, UWORD x, UWORD y, UBYTE color)
{
    UBYTE char_data[8];
    Font_GetChar(c, char_data);
    
    if (x + CHAR_WIDTH > VIEWPORT_WIDTH || y + CHAR_HEIGHT > BITMAPHEIGHT)
        return;

    UWORD byte_offset = x >> 3;
    
    for (int row = 0; row < CHAR_HEIGHT; row++)
    {
        UBYTE font_row = char_data[row];
        
        // For BITMAPBYTESPERROW = 24: 96 = 64 + 32
        ULONG line_offset = ((ULONG)(y + row) << 6) + ((ULONG)(y + row) << 5);
        
        // Write to each bitplane
        for (int plane = 0; plane < 4; plane++)
        { 
            // 24 = 16 + 8
            ULONG plane_offset = line_offset + ((plane << 4) + (plane << 3)) + byte_offset;
            
            if (color & (1 << plane))
            {
                buffer[plane_offset] |= font_row;   // Set bits
            }
            else
            {
                buffer[plane_offset] &= ~font_row;  // Clear bits
            }
        }
    }
}

// Draw string at position
void Font_DrawString(UBYTE *buffer, char *text, UWORD x, UWORD y, UBYTE color)
{
    WORD current_x = x;
    
    for (int i = 0; text[i]; i++)
    {
        // Check if character is byte-aligned for optimization
        if (current_x % 8 == 0)
            Font_DrawCharAligned(buffer, text[i], current_x, y, color);
        else
            Font_DrawChar(buffer, text[i], current_x, y, color);
        
        current_x += CHAR_WIDTH;
        
        // Stop if we've gone off screen
        if (current_x >= VIEWPORT_WIDTH)
            break;
    }
}

// Measure text width in pixels
UWORD Font_MeasureText(char *text)
{
    return strlen(text) << 3;
}

// Draw string centered horizontally
void Font_DrawStringCentered(UBYTE *buffer, char *text, UWORD y, UBYTE color)
{
    UWORD text_width = Font_MeasureText(text);
    UWORD x = (VIEWPORT_WIDTH - text_width) >> 1;
    
    x &= 0xFFF8;
    
    Font_DrawString(buffer, text, x, y, color);
}

// Draw string centered both ways
void Font_DrawStringCenteredBoth(UBYTE *buffer, char *text, UBYTE color)
{
    UWORD text_width = Font_MeasureText(text);
    UWORD x = (VIEWPORT_WIDTH - text_width) / 2;
    UWORD y = (VIEWPORT_HEIGHT - CHAR_HEIGHT) / 2;
    
    x = (x / 8) * 8;  // Align to byte boundary
    
    Font_DrawString(buffer, text, x, y, color);
}

// Clear rectangular area (set to color 0)
void Font_ClearArea(UBYTE *buffer, UWORD x, UWORD y, UWORD width, UWORD height)
{
    UWORD start_byte = x >> 3;  // x / 8
    UWORD end_byte = (x + width + 7) >> 3;  // Ceiling division

    if (end_byte > BITMAPBYTESPERROW) 
        end_byte = BITMAPBYTESPERROW;

    
    // For BITMAPBYTESPERROW = 24: 24 * 4 = 96 = 64 + 32
    ULONG line_offset = ((ULONG)y << 6) + ((ULONG)y << 5);

    for (UWORD row = 0; row < height; row++)
    {
        if (y + row >= VIEWPORT_HEIGHT) break;
        
        for (int plane = 0; plane < 4; plane++)
        {
           
            // 24 = 16 + 8 = 2^4 + 2^3
            ULONG plane_offset = line_offset + ((plane << 4) + (plane << 3));
            
            // Clear bytes for this line
            for (UWORD byte = start_byte; byte < end_byte; byte++)
            {
                buffer[plane_offset + byte] = 0;
            }
        }
        
       
        line_offset += 96;  // Compiler will optimize to (64 + 32)
    }
}

void Font_RestoreFromPristine(UBYTE *dest_buffer, UWORD x, UWORD y, UWORD width, UWORD height)
{
    UWORD start_byte = x >> 3;
    UWORD width_bytes = ((x + width + 7) >> 3) - start_byte;
    
    if (start_byte + width_bytes > BITMAPBYTESPERROW) 
        width_bytes = BITMAPBYTESPERROW - start_byte;
    
    ULONG line_offset = ((ULONG)y << 6) + ((ULONG)y << 5) + start_byte;
    
    for (UWORD row = 0; row < height; row++)
    {
        if (y + row >= BITMAPHEIGHT) break;
        
        for (int plane = 0; plane < 4; plane++)
        {
            ULONG plane_offset = line_offset + ((plane << 4) + (plane << 3));
            
            for (UWORD byte = 0; byte < width_bytes; byte++)
            {
                dest_buffer[plane_offset + byte] = screen.pristine[plane_offset + byte];
            }
        }
        
        line_offset += 96;
    }
}