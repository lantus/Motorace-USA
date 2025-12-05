#define ACCEL_RATE 2         // Speed increase per frame when accelerating
#define DECEL_RATE 2         // Speed decrease per frame when coasting

#define MIN_SPEED 0
#define MAX_SPEED 210

typedef enum {
    BIKE_FRAME_MOVING1 = 0,
    BIKE_FRAME_MOVING2,
    BIKE_FRAME_MOVING3,
    BIKE_FRAME_LEFT1,
    BIKE_FRAME_LEFT2,
    BIKE_FRAME_RIGHT1,
    BIKE_FRAME_RIGHT2,
    BIKE_FRAME_APPROACH1,
    BIKE_FRAME_APPROACH2,
    BIKE_FRAME_APPROACH3,
    BIKE_FRAME_APPROACH4,
    BIKE_FRAME_APPROACH5,
    BIKE_FRAME_APPROACH6,       
    BIKE_FRAME_APPROACH7,  
    BIKE_FRAME_APPROACH8,    
    BIKE_FRAME_APPROACH1_LEFT,
    BIKE_FRAME_APPROACH1_RIGHT
} BikeFrame;

#define BIKE_MOVING1            "sprites/bike/bike1.spr"
#define BIKE_MOVING2            "sprites/bike/bike2.spr"
#define BIKE_MOVING3            "sprites/bike/bike3.spr"
#define BIKE_LEFT1              "sprites/bike/bikeleft1.spr"
#define BIKE_LEFT2              "sprites/bike/bikeleft2.spr"
#define BIKE_RIGHT1             "sprites/bike/bikeright1.spr"
#define BIKE_RIGHT2             "sprites/bike/bikeright2.spr"

#define APPROACH_BIKE1          "sprites/bike/3dbike1.spr"
#define APPROACH_BIKE_LEFT      "sprites/bike/3dbike1-left.spr"
#define APPROACH_BIKE_RIGHT     "sprites/bike/3dbike1-right.spr"
#define APPROACH_BIKE2          "sprites/bike/3dbike2.spr"
#define APPROACH_BIKE3          "sprites/bike/3dbike3.spr"
#define APPROACH_BIKE4          "sprites/bike/3dbike4.spr"
#define APPROACH_BIKE5          "sprites/bike/3dbike5.spr"
#define APPROACH_BIKE6          "sprites/bike/3dbike6.spr"
#define APPROACH_BIKE7          "sprites/bike/3dbike7.spr"
#define APPROACH_BIKE8          "sprites/bike/3dbike8.spr"
 
extern UWORD bike_position_x;
extern UWORD bike_position_y;
 

// TODO: Crashing, Jump, Wheelie

enum BikeState
{
    BIKE_STATE_STOPPED = 0,
    BIKE_STATE_MOVING = 1,
    BIKE_STATE_LEFT = 2,
    BIKE_STATE_RIGHT = 3,
    BIKE_STATE_ACCELERATING = 4,
    BIKE_STATE_BRAKING = 5,
    BIKE_STATE_CRASHED = 6,
    BIKE_STATE_JUMP = 7,
    BIKE_STATE_WHEELIE = 8
};

extern UBYTE bike_state;
extern ULONG bike_anim_frames;
extern WORD bike_speed;        // Current speed in mph (starts at idle)
extern WORD bike_acceleration;  // Current acceleration
extern WORD scroll_accumulator;  // Fractional scroll position (fixed point)

#define MIN_CRUISING_SPEED 42  // Minimum automatic speed
 
void MotorBike_Initialize();
void CleanupMotorbikeSprites();
void MotorBike_UpdatePosition(UWORD x, UWORD y, UBYTE state);
void ResetMotorBikeAnimFrames();
void AccelerateMotorBike();
void BrakeMotorBike();
void MotorBike_Reset();
void MotorBike_Draw(WORD x, UWORD y, UBYTE state);
void MotorBike_SetFrame(BikeFrame frame); 
void MotorBike_UpdateApproachFrame(UWORD y);
WORD MotorBike_GetVibrationOffset(void);
void MotorBike_AutoAccelerate(void);
void MotorBike_DecelerateToCruising(void);