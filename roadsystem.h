 
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

 extern BYTE road_tile_idx;

 void UpdateRoadScroll(UWORD bike_speed, UWORD frame_count);