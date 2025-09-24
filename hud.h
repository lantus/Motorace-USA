#define HUD_START_X 192
#define HUD_WIDTH 64
#define HUD_HEIGHT 256

#define NEW_YORK    "\x7E\x7F"
#define CHICAGO     "\x88\x89"
#define ST_LOUIS    "\x86\x87"
#define HOUSTON     "\x84\x85"
#define LASVEGAS    "\x82\x83"
#define LASANGELES  "\x80\x81"

#define PROGRESS_PIECE1 "\x3B"
#define PROGRESS_PIECE2 "\x1E"
#define PROGRESS_PIECE3 "\x1F"
#define PROGRESS_PIECE4 "\x2E"
#define PROGRESS_PIECE5 "\x2F"

typedef struct 
{
    UWORD *sprite_data[4];  // Pointers to sprite data for sprites 4-7
    UWORD sprite_x, sprite_y;
} HUDSprites;

extern HUDSprites hud_sprites;

void InitHUDSprites(void);
void InitSpriteData(UWORD *sprite_data, UWORD x, UWORD y, UWORD height);
void DrawCharToSprite(UWORD *sprite_data, char c, int x, int y);
void SetHUDSpritePointers(void);
void SetHUDWhite(void);
void PreDrawHUD();
void DrawHUDText(char *text, int sprite_col, int y_offset);
void DrawHUDString(char *text, int start_sprite, int y_offset);
void SetHUDSpritePositions(void);
void UpdateScore(ULONG score);
void UpdateSpeed();
