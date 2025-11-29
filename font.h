#define VIEWPORT_WIDTH 192
#define VIEWPORT_HEIGHT 256
#define CHAR_WIDTH 8
#define CHAR_HEIGHT 8

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
// Draw single character to viewport at pixel position
void Font_DrawChar(UBYTE *buffer, char c, UWORD x, UWORD y, UBYTE color);

// Draw string to viewport at pixel position
void Font_DrawString(UBYTE *buffer, char *text, UWORD x, UWORD y, UBYTE color);

// Draw string centered horizontally on screen
void Font_DrawStringCentered(UBYTE *buffer, char *text, UWORD y, UBYTE color);

// Draw string centered both horizontally and vertically
void Font_DrawStringCenteredBoth(UBYTE *buffer, char *text, UBYTE color);

// Measure text width in pixels
UWORD Font_MeasureText(char *text);

// Clear a rectangular area (useful for erasing text)
void Font_ClearArea(UBYTE *buffer, UWORD x, UWORD y, UWORD width, UWORD height);