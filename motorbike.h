#define ACCEL_RATE 2         // Speed increase per frame when accelerating
#define DECEL_RATE 1         // Speed decrease per frame when coasting

#define MIN_SPEED 0
#define MAX_SPEED 210

#define BIKE_MOVING1    "sprites/bike/bike1.spr"
#define BIKE_MOVING2    "sprites/bike/bike2.spr"
#define BIKE_MOVING3    "sprites/bike/bike3.spr"
#define BIKE_LEFT1      "sprites/bike/bikeleft1.spr"
#define BIKE_LEFT2      "sprites/bike/bikeleft2.spr"
#define BIKE_RIGHT1     "sprites/bike/bikeright1.spr"
#define BIKE_RIGHT2     "sprites/bike/bikeright2.spr"
 
extern UWORD bike_position_x;
extern UWORD bike_position_y;

// TODO: Crashing, Jump, Wheelie

enum BikeState
{
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
extern WORD scroll_accumulator;  // Fractional scroll position (fixed poin

void LoadMotorbikeSprites();
void CleanupMotorbikeSprites();
void UpdateMotorBikePosition(UWORD x, UWORD y, UBYTE state);
void ResetMotorBikeAnimFrames();
void AccelerateMotorBike();
void BrakeMotorBike();

 