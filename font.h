typedef struct 
{
    UBYTE *font_data;     // Raw font bitmap data
    WORD chars_per_row;   // Characters per row in font sheet
    WORD char_width;      // 8 pixels
    WORD char_height;     // 8 pixels
} FontSheet;

extern FontSheet game_font;

BOOL LoadFontSheet(const char *filename);
void GetFontChar(char c, UBYTE *char_data);