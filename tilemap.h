#define TILES_SIZE  368648
#define MAP_SIZE    320

#define TILES   144
#define TILE_BITS 4

#define LINEBLOCKS 16
#define WBLOCK 16
#define BITMAPLINEBYTES 40
#define BITMAPLINEBYTESI 160
#define BITMAPLINEBYTESB 38

typedef struct TileMap 
{
    BitMapEx* map;
    UBYTE *tilemap;
    UBYTE twidth;
    UBYTE theight;
    UBYTE tdepth;
    UWORD mwidth;
    UWORD mheight;
    UBYTE *offset[TILES];
    UBYTE dirty[MAP_SIZE];

} TileMap;


TileMap* TileMap_Create(BitMapEx *bitmap, UBYTE *tilemap, UBYTE tile_width, UBYTE tile_height, UWORD map_width, UWORD map_height, UBYTE depth);
void TileMap_DrawTile(TileMap *tm, UBYTE *dst, WORD x, WORD y, WORD tile);