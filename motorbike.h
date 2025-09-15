#define BIKE_MOVING1    "sprites/bike/bike1.spr"
#define BIKE_MOVING2    "sprites/bike/bike2.spr"
#define BIKE_LEFT1      "sprites/bike/bikeleft1.spr"
#define BIKE_LEFT2      "sprites/bike/bikeleft2.spr"
#define BIKE_RIGHT1     "sprites/bike/bikeright1.spr"
#define BIKE_RIGHT2     "sprites/bike/bikeright2.spr"

extern UWORD bike_speed;
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

void LoadMotorbikeSprites();
void CleanupMotorbikeSprites();
void UpdateMotorBikePosition(UWORD x, UWORD y, UBYTE state);
void ResetMotorBikeAnimFrames();

 