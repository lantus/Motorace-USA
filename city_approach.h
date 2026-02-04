
#define NYC_SKYLINE "images/skyline_nyc.BPL"   // demo and nyc are the same
#define STL_SKYLINE "images/skyline_stl.BPL"
#define HOUSTON_SKYLINE "images/skyline_hou.BPL"
#define VEGAS_SKYLINE "images/skyline_lv.BPL"
#define VEGAS_SKYLINE_ANIM1 "images/lv_anim1.BPL"
#define VEGAS_SKYLINE_ANIM2 "images/lv_anim2.BPL"
#define LA_SKYLINE "images/skyline_la.BPL"

#define ONCOMING_CAR_1 "objects/frontview/frontview_car9.BPL"
#define ONCOMING_CAR_2 "objects/frontview/frontview_car8.BPL"
#define ONCOMING_CAR_3 "objects/frontview/frontview_car7.BPL"
#define ONCOMING_CAR_4 "objects/frontview/frontview_car6.BPL"
#define ONCOMING_CAR_5 "objects/frontview/frontview_car5.BPL"
#define ONCOMING_CAR_6 "objects/frontview/left_frontview_car4.BPL"
#define ONCOMING_CAR_7 "objects/frontview/left_frontview_car3.BPL"
#define ONCOMING_CAR_8 "objects/frontview/left_frontview_car2.BPL"
#define ONCOMING_CAR_9 "objects/frontview/left_frontview_car1.BPL"

#define CITYSKYLINE_WIDTH   192
#define CITYSKYLINE_HEIGHT   56

#define NUM_CAR_SCALES 9
#define MAX_ONCOMING_CARS 5

typedef struct {
    WORD x;              // Screen X position
    WORD y;              // Screen Y position  
    WORD scale_index;    // Which scale bitmap (0=smallest, 8=largest)
    BOOL active;         // Is this car slot in use?
    WORD speed;          // How fast it approaches (pixels per frame)
    WORD lane;           // Which lane (0=left, 1=center, 2=right)
} OncomingCar;

extern OncomingCar oncoming_cars[MAX_ONCOMING_CARS];

// Car sprite data (9 scales)
extern UBYTE *car_scale_data[NUM_CAR_SCALES];
extern WORD car_scale_widths[NUM_CAR_SCALES];
extern WORD car_scale_heights[NUM_CAR_SCALES];

void City_Initialize();
void City_BlitHorizon();
void City_DrawRoad();
void City_PreDrawRoad();
 