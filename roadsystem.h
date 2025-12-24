 
// Road viewport only (left 192 pixels)
#define ROAD_VIEWPORT_WIDTH 192
#define ROAD_VIEWPORT_HEIGHT 256
#define ROAD_BYTES 24         // 192 / 8

#define CITY_HEIGHT 72
#define ROAD_START_Y 72
#define ROAD_END_Y 255
#define ROAD_LINES 184

#define VANISHING_POINT_X 96  // Center of road viewport


// Define lane positions with more separation
#define FAR_LEFT_LANE 40     // Far left
#define LEFT_LANE 80         // Left lane
#define CENTER_LANE 96       // Center (where bike is)
#define RIGHT_LANE 112       // Right lane
#define FAR_RIGHT_LANE 128   // Far right

#define TILEATTRIB_MAP_WIDTH 20    // 320 / 16
#define TILEATTRIB_MAP_HEIGHT 18   // 288 / 16
#define TILEATTRIB_MAP_SIZE 360    // 20 * 18

typedef enum 
{
    TILEATTRIB_ROAD = 0,
    TILEATTRIB_CRASH = 1,
    TILEATTRIB_WATER = 2,
    TILEATTRIB_ROAD_LEFT_HALF = 3,
    TILEATTRIB_ROAD_RIGHT_HALF = 4,
    TILEATTRIB_SLOW = 5,
    TILEATTRIB_BOOST = 6,
    TILEATTRIB_WHEELIE = 7,
    TILEATTRIB_JUMP = 8
} TileAttribute;

 extern BYTE road_tile_idx;

 void UpdateRoadScroll(UWORD bike_speed, UWORD frame_count);

 extern UBYTE tile_attrib_map[TILEATTRIB_MAP_SIZE];

void TileAttrib_Load(void);
BOOL TileAttrib_IsDrivable(WORD tile_x, WORD tile_y);
TileAttribute Tile_GetAttrib(WORD world_x, WORD world_y);