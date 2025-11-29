#define HUD_START_X 192
#define HUD_WIDTH 64
#define HUD_HEIGHT 256

#define NEW_YORK    "\x7E\x7F"
#define CHICAGO     "\x88\x89"
#define ST_LOUIS    "\x86\x87"
#define HOUSTON     "\x84\x85"
#define LASVEGAS    "\x82\x83"
#define LASANGELES  "\x80\x81"

#define PROGRESS_PIECE0 "\x3C"      
#define PROGRESS_PIECE1 "\x3B"  
#define PROGRESS_PIECE2 "\x3D"
#define PROGRESS_PIECE3 "\x1E"
#define PROGRESS_PIECE4 "\x1F"
#define PROGRESS_PIECE5 "\x2E"
#define PROGRESS_PIECE6 "\x2F"

#define BOX_TOP_LEFT_PIECE  "\x03\x04"
#define BOX_TOP_MIDDLE_PIECE "\x04\x04"
#define BOX_TOP_RIGHT_PIECE "\x05"
#define BOX_BTM_LEFT_PIECE "\x06\x01"
#define BOX_BTM_MIDDLE_PIECE "\x01\x01"
#define BOX_BTM_RIGHT_PIECE "\x02"
#define BOX_LEFT_SIDE_PIECE "\x13"
#define BOX_RIGHT_SIDE_PIECE "\x15"

typedef struct 
{
    UWORD *sprite_data[4];  // Pointers to sprite data for sprites 4-7
    UWORD sprite_x, sprite_y;
} HUDSprites;

extern HUDSprites hud_sprites;

void HUD_InitSprites(void);
void HUD_InitSpriteData(UWORD *sprite_data, UWORD x, UWORD y, UWORD height);
void HUD_DrawCharToSprite(UWORD *sprite_data, char c, int x, int y);
void HUD_DrawCharToSpriteWithColor(UWORD *sprite_data, char c, int x, int y, int color_mode);
void HUD_SetSpritePointers(void);
void HUD_SetHUDWhite(void);
void HUD_DrawAll();
void HUD_DrawText(char *text, int sprite_col, int y_offset);
void HUD_DrawString(char *text, int start_sprite, int y_offset);
void HUD_DrawStringWithColor(char *text, int start_sprite, int y_offset, int color_mode);
void HUD_SetSpritePositions(void);
void HUD_UpdateScore(ULONG score);
void HUD_UpdateSpeed();
void HUD_UpdateRank(UBYTE rank);
char *GetOrdinalSuffix(UBYTE number);
void ULongToString(ULONG value, char *buffer, int width, char pad_char);
