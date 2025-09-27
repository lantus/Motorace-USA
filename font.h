typedef struct 
{
    UBYTE *font_data;     // Raw font bitmap data
    WORD chars_per_row;   // Characters per row in font sheet
    WORD char_width;      // 8 pixels
    WORD char_height;     // 8 pixels
} FontSheet;

extern FontSheet game_font;

BOOL Font_LoadSheet(const char *filename);
void Font_GetChar(char c, UBYTE *char_data);