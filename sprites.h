// Structure to hold sprite data
#define FILE_ID_LEN (8)

#define SPRITEPTR_ZERO_AND_ONE 0
#define SPRITEPTR_TWO_AND_THREE 2
#define SPRITEPTR_FOUR_AND_FIVE 4
#define SPRITEPTR_SIX_AND_SEVEN 6

#define SPRITEPTR_ZERO 0
#define SPRITEPTR_ONE 1
#define SPRITEPTR_TWO 2
#define SPRITEPTR_THREE 3
#define SPRITEPTR_FOUR 4
#define SPRITEPTR_FIVE 5
#define SPRITEPTR_SIX 6
#define SPRITEPTR_SEVEN 7

#define COUNTDOWN_ZERO            "sprites/misc/num0.spr"
#define COUNTDOWN_ONE             "sprites/misc/num1.spr"
#define COUNTDOWN_TWO             "sprites/misc/num2.spr"
#define COUNTDOWN_THREE           "sprites/misc/num3.spr"

typedef struct SpriteSheetHeader 
{
    UBYTE id[FILE_ID_LEN];
    UBYTE version, flags;
    UBYTE reserved1, palette_size;
    UWORD num_sprites;
    ULONG imgdata_size;
    UWORD checksum;
} SpriteSheetHeader;

#define MAX_SPRITE_PALETTE_SIZE (16)
#define MAX_SPRITES_PER_SHEET (16)

typedef struct SpriteSheet 
{
    struct SpriteSheetHeader header;
    UWORD palette[MAX_SPRITE_PALETTE_SIZE];
    UWORD sprite_offsets[MAX_SPRITES_PER_SHEET];
    UBYTE *imgdata;
} Sprite;

BOOL Sprites_LoadFromFile(char *filename,Sprite *sheet);
BOOL Sprites_ApplyPalette(Sprite *sheet);
void Sprites_Extract(UBYTE sprite_sheet_index) ;
void Sprites_Initialize();
void Sprites_SetPointers(ULONG *sprite, UBYTE n, UBYTE sprite_index);
void Sprites_SetPosition(UWORD *sprite_data, UWORD hstart, UWORD vstart, UWORD vstop);
void Sprites_Free();
void Sprites_BuildComposite(ULONG *sprite, int n, Sprite *sheet);
void Sprites_SetScreenPosition(UWORD *sprite_data, WORD x, UWORD y, UWORD height);
void Sprites_ClearLower();
void Sprites_ClearHigher();
