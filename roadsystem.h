 
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
#define FAR_LEFT_LANE 46     // Far left
#define LEFT_LANE 80         // Left lane
#define CENTER_LANE 86       // Center 
#define RIGHT_LANE 112       // Right lane
#define FAR_RIGHT_LANE 128   // Far right

#define TILEATTRIB_MAP_WIDTH 20    // 320 / 16
#define TILEATTRIB_MAP_HEIGHT 18   // 288 / 16
#define TILEATTRIB_MAP_SIZE 360    // 20 * 18

/* Packed collision byte: [LLLL SSSS] */
/* Lane = high nibble, Surface = low nibble */

#define LANE_OFFROAD      0
#define LANE_SHOULDER_L   1
#define LANE_1            2
#define LANE_2            3
#define LANE_3            4
#define LANE_4            5
#define LANE_SHOULDER_R   6
#define LANE_MERGE        7

#define SURFACE_NORMAL    0
#define SURFACE_PUDDLE    1
#define SURFACE_WATER     2
#define SURFACE_WHEELIE   3
#define SURFACE_GASCAN    4
#define SURFACE_GRAVEL    5
#define SURFACE_BOOST     6
#define SURFACE_JUMP      7


#define GET_LANE(packed)    ((packed) >> 4)
#define GET_SURFACE(packed) ((packed) & 0x0F)
#define IS_DRIVABLE(packed) (GET_LANE(packed) >= LANE_SHOULDER_L)
#define IS_LANE(packed)     (GET_LANE(packed) >= LANE_1 && GET_LANE(packed) <= LANE_4)

#define ROAD_CACHE_SIZE 256

#define ROAD_TILE_PLAIN 11

extern BYTE road_tile_idx;
extern WORD road_center_cache[ROAD_CACHE_SIZE];

void UpdateRoadScroll(UWORD bike_speed, UWORD frame_count);
UBYTE Collision_Get(WORD world_x, WORD world_y);
UBYTE Collision_GetLane(WORD world_x, WORD world_y);
UBYTE Collision_GetSurface(WORD world_x, WORD world_y);
void Collision_Set(WORD world_x, WORD world_y, UBYTE value);
void CollisionMap_Load(void);
void CollisionMap_SetStage(UBYTE stage);

void Road_CacheRow(WORD world_y);
void Road_CacheFillVisible(void);
