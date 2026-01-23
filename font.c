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
    LONG font_size = 128 * 80 / 8;  // 1 bitplane, packed
    
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
    
    Close(font_handle);
    return TRUE;
}

void Font_GetChar(char c, UBYTE *char_data)
{
    // Calculate character position in font sheet
    UBYTE char_index = (UBYTE)c;
    
    WORD char_x = (char_index % game_font.chars_per_row) << 3;
    WORD char_y = (char_index / game_font.chars_per_row) << 3;
    
    // Font sheet is 128 pixels wide = 16 bytes per row
    WORD bytes_per_row = 128 >> 3;
    
    // Extract 8x8 character
    for (int row = 0; row < 8; row++) 
    {
        WORD src_offset = ((char_y + row) * bytes_per_row) + (char_x >> 3);
        char_data[row] = game_font.font_data[src_offset];
        
        // If character spans byte boundary, handle bit shifting
        if (char_x % 8 != 0) 
        {
            UBYTE shift = char_x % 8;
            char_data[row] = (game_font.font_data[src_offset] << shift) |
                            (game_font.font_data[src_offset + 1] >> (8 - shift));
        }
    }
}
 
// Draw single character to bitplane buffer
void Font_DrawChar(UBYTE *buffer, char c, UWORD x, UWORD y, UBYTE color)
{
    UBYTE char_data[8];
    Font_GetChar(c, char_data);
    
    // Clipping
    if (x + CHAR_WIDTH > VIEWPORT_WIDTH || y + CHAR_HEIGHT > VIEWPORT_HEIGHT)
        return;
    
    UWORD byte_offset = x >> 1;  // Which byte horizontally
    UBYTE bit_shift = 7 - (x % 8);  // Bit position within byte
    
    for (int row = 0; row < CHAR_HEIGHT; row++)
    {
        UBYTE font_row = char_data[row];
        
        // Calculate offset for this scanline in the buffer
        // Format: line0_plane0, line0_plane1, line0_plane2, line0_plane3,
        //         line1_plane0, line1_plane1, line1_plane2, line1_plane3, ...
        ULONG line_offset = (y + row) * BITMAPBYTESPERROW << 2;
        
        // Process each pixel in the character row
        for (int bit = 0; bit < 8; bit++)
        {
            if (font_row & (0x80 >> bit))
            {
                UWORD pixel_byte = byte_offset + (bit >> 3);
                UBYTE pixel_bit = 7 - ((x + bit) % 8);
                
                // Set bits in each bitplane based on color value
                for (int plane = 0; plane < 4; plane++)
                {
                    if (color & (1 << plane))
                    {
                        ULONG plane_offset = line_offset + (plane * BITMAPBYTESPERROW) + pixel_byte;
                        buffer[plane_offset] |= (1 << pixel_bit);
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
    
    if (x + CHAR_WIDTH > VIEWPORT_WIDTH || y + CHAR_HEIGHT > VIEWPORT_HEIGHT)
        return;
    
    UWORD byte_offset = x / 8;
    
    for (int row = 0; row < CHAR_HEIGHT; row++)
    {
        UBYTE font_row = char_data[row];
        ULONG line_offset = (y + row) * BITMAPBYTESPERROW * 4;
        
        // Write to each bitplane
        for (int plane = 0; plane < 4; plane++)
        {
            if (color & (1 << plane))
            {
                ULONG plane_offset = line_offset + (plane * BITMAPBYTESPERROW) + byte_offset;
                buffer[plane_offset] |= font_row;
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
    return strlen(text) * CHAR_WIDTH;
}

// Draw string centered horizontally
void Font_DrawStringCentered(UBYTE *buffer, char *text, UWORD y, UBYTE color)
{
    UWORD text_width = Font_MeasureText(text);
    UWORD x = (VIEWPORT_WIDTH - text_width) / 2;
    
    // Align to byte boundary for speed
    x = (x / 8) * 8;
    
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
    for (UWORD row = 0; row < height; row++)
    {
        if (y + row >= VIEWPORT_HEIGHT) break;
        
        ULONG line_offset = (y + row) * BITMAPBYTESPERROW * 4;
        
        for (int plane = 0; plane < 4; plane++)
        {
            ULONG plane_offset = line_offset + (plane * BITMAPBYTESPERROW);
            
            // Clear bytes for this line
            UWORD start_byte = x / 8;
            UWORD end_byte = (x + width + 7) / 8;
            
            for (UWORD byte = start_byte; byte < end_byte && byte < BITMAPBYTESPERROW; byte++)
            {
                buffer[plane_offset + byte] = 0;
            }
        }
    }
}