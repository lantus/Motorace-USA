#include "support/gcc8_c_support.h"
 
#include "game.h"
#include "copper.h"
#include "memory.h"
#include "sprites.h"
#include "copper.h"
#include "hiscore.h"
#include "hud.h"
#include "timers.h"
#include "fuel.h"
#include "font.h"

#define VALUE 1

HUDSprites hud_sprites;

static UBYTE last_speed;

static GameTimer oneup_flash_timer;
static BOOL oneup_visible = TRUE;

void HUD_InitSprites(void)
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
        HUD_InitSpriteData(hud_sprites.sprite_data[i], HUD_START_X + (i * 16), 44, HUD_HEIGHT);
    }

    Font_LoadSheet("font/zippyfont.BPL");
}

void HUD_InitSpriteData(UWORD *sprite_data, UWORD x, UWORD y, UWORD height)
{
    // Set sprite position and size (first 2 words)
    Sprites_SetPosition(sprite_data, x, y, y + height - 1);
    
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

void HUD_Init1UPFlash(void)
{
    oneup_visible = TRUE;
    Timer_StartMs(&oneup_flash_timer, 300);
}

void HUD_Update1UPFlash(void)
{
    if (Timer_HasElapsed(&oneup_flash_timer))
    {
        oneup_visible = !oneup_visible;
        Timer_Reset(&oneup_flash_timer);
        
        UWORD color = oneup_visible ? 0x0FFF : 0x0000;
        
        Cop1UP_C25[1] = color;
        Cop1UP_C26[1] = color;
        Cop1UP_C27[1] = color;
        Cop1UP_C29[1] = color;
        Cop1UP_C30[1] = color;
        Cop1UP_C31[1] = color;
    }
}

void HUD_Show1UP(void)
{
    Cop1UP_C25[1] = 0x0FFF;
    Cop1UP_C26[1] = 0x0FFF;
    Cop1UP_C27[1] = 0x0FFF;
    Cop1UP_C29[1] = 0x0FFF;
    Cop1UP_C30[1] = 0x0FFF;
    Cop1UP_C31[1] = 0x0FFF;
}

void HUD_SetSpritePointers(void)
{
    // Set sprites 4-7 to point to HUD data

    ULONG sprite_addr = (ULONG)hud_sprites.sprite_data[0];
    Sprites_SetPointers(&sprite_addr,1, SPRITEPTR_FOUR);
    sprite_addr = (ULONG)hud_sprites.sprite_data[1];
    Sprites_SetPointers(&sprite_addr,1, SPRITEPTR_FIVE); 
    sprite_addr = (ULONG)hud_sprites.sprite_data[2];
    Sprites_SetPointers(&sprite_addr,1, SPRITEPTR_SIX);
    sprite_addr = (ULONG)hud_sprites.sprite_data[3];   
    Sprites_SetPointers(&sprite_addr,1, SPRITEPTR_SEVEN);   
}

void HUD_SetWhite(void)
{
    Copper_SetSpritePalette(9, 0xFFF);   
    Copper_SetSpritePalette(10, 0x00F);   
    Copper_SetSpritePalette(11, 0xde0);   
    
    Copper_SetSpritePalette(12, 0xFFF);   
    Copper_SetSpritePalette(13, 0xFFF);  
    Copper_SetSpritePalette(14, 0xF53);    
    Copper_SetSpritePalette(15, 0xF53);

    Copper_SetScoreRestoreColors();
}

 
void HUD_DrawCharToSprite(UWORD *sprite_data, char c, int x, int y)
{
    UBYTE char_data[2][8];  
    Font_GetCharCol(c, char_data);

    UWORD mask = 0xFF00 >> x;  // Mask for 8 bits starting at position x

    for (int row = 0; row < 8; row++) {
    int sprite_line = y + row;
    if (sprite_line >= HUD_HEIGHT) break; 

    UBYTE font_row_p0 = char_data[0][row];  // Bitplane 0 from font
    UBYTE font_row_p1 = char_data[1][row];  // Bitplane 1 from font
    UWORD plane_a = 0, plane_b = 0;

    for (int bit = 0; bit < 8; bit++) 
    {
     
        if (font_row_p0 & (0x80 >> bit)) {
            plane_a |= (0x8000 >> (x + bit));
        }
        if (font_row_p1 & (0x80 >> bit)) {
            plane_b |= (0x8000 >> (x + bit));
        }
    }

        sprite_data[2 + sprite_line * 2] &= ~mask;      // Clear old bits
        sprite_data[2 + sprite_line * 2] |= plane_a;     // Set new bits

        sprite_data[2 + sprite_line * 2 + 1] &= ~mask;  // Clear old bits
        sprite_data[2 + sprite_line * 2 + 1] |= plane_b; // Set new bits
    }
}

void Font_GetCharCol(char c, UBYTE char_data[2][8])  
{
    UBYTE char_index = (UBYTE)c;
    
    // Optimize: 16 chars per row (power of 2)
    UBYTE char_x = char_index & 15;        // char_index % 16
    UBYTE char_y = char_index >> 4;        // char_index / 16
    
    // Pre-calculated constants
    // sheet_width_bytes = (16 * 8) / 8 = 16
    // plane_size = 16 * 80 = 1280
    #define SHEET_WIDTH_BYTES 16
    #define PLANE_SIZE 1280
    
    // Calculate base offset for this character's row
    UWORD base_y = char_y << 3;  // char_y * 8
    
    // Pre-calculate plane 1 base pointer
    UBYTE *plane0 = game_font.font_data;
    UBYTE *plane1 = game_font.font_data + PLANE_SIZE;
    
    // Unrolled loop for 8 rows 
    UWORD offset0 = (base_y << 4) + char_x;
    char_data[0][0] = plane0[offset0];
    char_data[1][0] = plane1[offset0];
    
    offset0 += SHEET_WIDTH_BYTES;
    char_data[0][1] = plane0[offset0];
    char_data[1][1] = plane1[offset0];
    
    offset0 += SHEET_WIDTH_BYTES;
    char_data[0][2] = plane0[offset0];
    char_data[1][2] = plane1[offset0];
    
    offset0 += SHEET_WIDTH_BYTES;
    char_data[0][3] = plane0[offset0];
    char_data[1][3] = plane1[offset0];
    
    offset0 += SHEET_WIDTH_BYTES;
    char_data[0][4] = plane0[offset0];
    char_data[1][4] = plane1[offset0];
    
    offset0 += SHEET_WIDTH_BYTES;
    char_data[0][5] = plane0[offset0];
    char_data[1][5] = plane1[offset0];
    
    offset0 += SHEET_WIDTH_BYTES;
    char_data[0][6] = plane0[offset0];
    char_data[1][6] = plane1[offset0];
    
    offset0 += SHEET_WIDTH_BYTES;
    char_data[0][7] = plane0[offset0];
    char_data[1][7] = plane1[offset0];
}
 
 
void HUD_DrawText(char *text, int sprite_col, int y_offset)
{
    UWORD *sprite = hud_sprites.sprite_data[sprite_col];

    int char_x = 0;  // Horizontal position within the 16-pixel sprite
    
    for (int i = 0; text[i] && char_x + 8 <= 16; i++) 
    {
        HUD_DrawCharToSprite(sprite, text[i], char_x, y_offset);
        char_x += 8;  // Move 8 pixels right for next character
      
    }
}

void HUD_ClearSpriteBlock(int sprite_index, WORD y_start)
{
 
    if (sprite_index < 0 || sprite_index >= 4) return;
    if (!hud_sprites.sprite_data[sprite_index]) return;
    
    UWORD *sprite_data = hud_sprites.sprite_data[sprite_index];
    
    // Clear 8 rows (each row = 2 words: plane A, plane B)
    for (int row = 0; row < 8; row++)
    {
        int sprite_line = y_start + row;
        if (sprite_line >= HUD_HEIGHT) break;
        
        sprite_data[2 + sprite_line * 2] = 0;      // Clear plane A
        sprite_data[2 + sprite_line * 2 + 1] = 0;  // Clear plane B
    }
}
 
int counter = 0;

void ULongToString(ULONG value, char *buffer, int width, char pad_char)
{
    int pos = width;
    buffer[pos] = '\0';  // Null terminate once at start
    pos--;
    
    // Fast path for zero
    if (value == 0)
    {
        buffer[pos--] = '0';
    }
    else
    {
        // Optimized digit extraction
        ULONG q;
        while (value > 0 && pos >= 0)
        {
            q = value / 10;
            buffer[pos--] = '0' + (value - (q * 10));
            value = q;
        }
    }
    
    // Fill padding (if any)
    if (pad_char != '\0')
    {
        while (pos >= 0)
        {
            buffer[pos--] = pad_char;
        }
    }
}

void HUD_ClearSpriteArea(int start_sprite, int end_sprite, int y_offset, int height)
{
    // Clear specific area of sprites
    for (int sprite = start_sprite; sprite < end_sprite && sprite < start_sprite + end_sprite; sprite++) {
        UWORD *sprite_data = hud_sprites.sprite_data[sprite];
        
        for (int line = y_offset; line < y_offset + height && line < HUD_HEIGHT; line++) {
            sprite_data[2 + line * 2] = 0;      // Plane A = 0  
            sprite_data[2 + line * 2 + 1] = 0;  // Plane B = 0
        }
    }
}

void HUD_DrawScore(ULONG score, int start_sprite, int y_offset)
{
    char score_string[7];   
    if (score > 999999) score = 999999;
    ULongToString(score, score_string, 6, ' ');  
    HUD_DrawString(score_string, start_sprite, y_offset);
}

void HUD_DrawRank(UBYTE rank, int start_sprite, int y_offset)
{
    char rank_string[3];  
    
    ULongToString(rank, rank_string, 2,  0x10);
    
    // Draw the formatted score
    HUD_DrawString(rank_string, start_sprite, y_offset);
}

void HUD_DrawBikeSpeed(UBYTE speed, int start_sprite, int y_offset)
{
    char speed_string[4];  
 
    ULongToString(speed, speed_string, 3, ' ');
    
    // Draw the bike speed
    HUD_DrawString(speed_string, start_sprite, y_offset);
}

void HUD_UpdateScore(ULONG score)
{
    char score_buffer[6];
    ULongToString(score, score_buffer, 6, ' ');
    HUD_DrawString(score_buffer, 1, 24);

     /* Update hi-score if beaten */
    if (score >= HiScore_GetTopScore())
    {
        ULongToString(score, score_buffer, 6, ' ');
        HUD_DrawString(score_buffer, 1, 8);
    }
}

void HUD_UpdateBikeSpeed(ULONG bike_speed)
{
    last_speed = bike_speed;
    HUD_DrawBikeSpeed(last_speed, 1, 192); 
    HUD_DrawString(g_is_pal ? KMH_PIECE : MPH_PIECE, 3,192);
}

 
void HUD_UpdateRank(UBYTE rank)
{
 
    if (game_rank <= 1)
    {
        game_rank = 1;
    }

    char *suffix = GetOrdinalSuffix(game_rank);
    HUD_DrawRank(game_rank,1,168);
    HUD_DrawString(suffix, 2,168);
}

char* GetOrdinalSuffix(UBYTE number)
{
    // Handle special cases for 11th, 12th, 13th
    UBYTE last_two_digits = number % 100;

    if (last_two_digits >= 11 && last_two_digits <= 13) 
    {
        return "\x9c\x9d";
    }
    
    // Check the last digit for other cases
    UBYTE last_digit = number % 10;

    switch (last_digit) 
    {
        case 1:
            return "\x9e\x9f";
        case 2:
            return "\x98\x99";
        case 3:
            return "\x9a\x9b";
        default:
            return "\x9c\x9d";
    }
}

void HUD_DrawAll()
{   
    // Draw the HUD one time. then update smaller
    // things (Rank, Score etc) as needed

    HUD_SetWhite();

    const UBYTE status_y = 40;

    ULONG top_score = HiScore_GetTopScore();
    char score_buffer[6];
    
    // Format the high score (6 digits with leading zeros or spaces)
    ULongToString(top_score, score_buffer, 6, ' ');
    HUD_DrawString("HI-SCORE", 0, 0);
    HUD_DrawString(score_buffer, 1, 8);  // Draw the actual high score
   

    HUD_DrawString("1UP", 0, 16);
    HUD_UpdateScore(game_score);
    HUD_DrawString(NEW_YORK , 3 , status_y+3);
    HUD_DrawString(CHICAGO , 3 , status_y+21);
    HUD_DrawString(ST_LOUIS , 3 , status_y+41);
    HUD_DrawString(HOUSTON , 3 , status_y+61);
    HUD_DrawString(LASVEGAS , 3 , status_y+81);
    HUD_DrawString(LASANGELES , 3 , status_y+101);

    // Fuel Text

    HUD_DrawString("F" , 1 , status_y+12);
    HUD_DrawString("U" , 1 , status_y+20);
    HUD_DrawString("E" , 1 , status_y+28);
    HUD_DrawString("L" , 1 , status_y+36);

    // Fuel Gauge
    Fuel_DrawAll(); 
 
    // Stages

    // NY -> STL
  
    HUD_DrawString(PROGRESS_PIECE0, 2, status_y+8);
    HUD_DrawString(PROGRESS_PIECE3, 2, status_y+16);
    HUD_DrawString(PROGRESS_PIECE2, 2, status_y+20);
    // Chicago -> STL
    HUD_DrawString(PROGRESS_PIECE4, 2, status_y+28);
    HUD_DrawString(PROGRESS_PIECE4, 2, status_y+36);
    HUD_DrawString(PROGRESS_PIECE5, 2, status_y+40);
    // STL -> Houston
    HUD_DrawString(PROGRESS_PIECE3, 2, status_y+48);
    HUD_DrawString(PROGRESS_PIECE3, 2, status_y+56);
    HUD_DrawString(PROGRESS_PIECE2, 2, status_y+60);
    // Houston -> LV
    HUD_DrawString(PROGRESS_PIECE4, 2, status_y+68);
    HUD_DrawString(PROGRESS_PIECE4, 2, status_y+76);
    HUD_DrawString(PROGRESS_PIECE5, 2, status_y+80);
    // LV -> LA
    HUD_DrawString(PROGRESS_PIECE3, 2, status_y+88);
    HUD_DrawString(PROGRESS_PIECE3, 2, status_y+96);
    HUD_DrawString(PROGRESS_PIECE1, 2, status_y+100);
 
    // Rank
    HUD_DrawString(BOX_TOP_LEFT_PIECE,0,160-8);
    HUD_DrawString(BOX_TOP_MIDDLE_PIECE,1,160-8);
    HUD_DrawString(BOX_TOP_MIDDLE_PIECE,2,160-8);
    HUD_DrawString(BOX_TOP_RIGHT_PIECE,3,160-8);
 
    HUD_DrawString(BOX_LEFT_SIDE_PIECE,0,168-8);

    HUD_DrawString(BOX_RIGHT_SIDE_PIECE,3,168-8);
    HUD_DrawString(BOX_LEFT_SIDE_PIECE,0,176-8);
    HUD_DrawString(BOX_RIGHT_SIDE_PIECE,3,176-8);
 
    HUD_DrawString(BOX_BTM_LEFT_PIECE,0,184-8);
    HUD_DrawString(BOX_BTM_MIDDLE_PIECE,1,184-8);
    HUD_DrawString(BOX_BTM_MIDDLE_PIECE,2,184-8);
    HUD_DrawString(BOX_BTM_RIGHT_PIECE,3,184-8);
    
    // Speed
    HUD_DrawString("SPEED", 1, 184);
    HUD_DrawString("000", 1, 192);
    HUD_DrawString(g_is_pal ? KMH_PIECE : MPH_PIECE, 3,192);

    HUD_DrawString("\x13""RANK", 0, 168-8);
    HUD_UpdateRank(90);
}

void HUD_DrawTextRight(UBYTE ch, int sprite_index, int y_offset)
{
    /* For each sprite word line:
     * Preserve left 8 bits (FUEL), overwrite right 8 bits (progress)
     * 
     * word = (word & 0xFF00) | (tile_byte & 0x00FF)
     */
}

void HUD_DrawString(char *text, int start_sprite, int y_offset)
{
    ULONG text_len = strlen(text);
    ULONG sprite_index = start_sprite;
    ULONG char_pos = 0;
 
    while (char_pos < text_len && sprite_index < 4) 
    {
        char sprite_text[3] = {0}; // 2 chars per sprite + null terminator
        ULONG chars_in_sprite = 0;
        
        // Fill current sprite with up to 2 characters
        while (chars_in_sprite < 2 && char_pos < text_len) 
        {
            sprite_text[chars_in_sprite] = text[char_pos];
            chars_in_sprite++;
            char_pos++;
        }
        
        // Draw to current sprite if we have characters
        if (chars_in_sprite > 0) 
        {
            HUD_DrawText(sprite_text, sprite_index, y_offset);
        }
        
        sprite_index++;
    }
}

void HUD_SetSpritePositions(void)
{
    for (int i = 0; i < 4; i++) 
    {
        UWORD logical_x = 192 + 32 + (i * 16);  // Where you want it on screen
        UWORD sprite_x = logical_x + 128;  // Actual sprite coordinate
        
        Sprites_SetPosition(hud_sprites.sprite_data[i], sprite_x, 40, 40 + HUD_HEIGHT - 1);
    }
    
    HUD_SetSpritePointers();
}
 