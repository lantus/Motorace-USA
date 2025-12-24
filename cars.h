#define CAR0_FRAME0 "objects/car0_0.BPL"
#define CAR0_FRAME1 "objects/car0_1.BPL"
#define CAR1_FRAME0 "objects/car1_0.BPL"
#define CAR1_FRAME1 "objects/car1_1.BPL"
#define CAR2_FRAME0 "objects/car2_0.BPL"
#define CAR2_FRAME1 "objects/car2_1.BPL"
#define CAR3_FRAME0 "objects/car3_0.BPL"
#define CAR3_FRAME1 "objects/car3_1.BPL"
#define CAR4_FRAME0 "objects/car4_0.BPL"
#define CAR4_FRAME1 "objects/car4_1.BPL"


// BOB system definitions
#define MAX_CARS 5
#define BOB_WIDTH 32
#define BOB_HEIGHT 32
#define BOB_PLANES 4
#define BOB_DATA_SIZE (BOB_WIDTH * BOB_HEIGHT * BOB_PLANES / 8)

extern BlitterObject car[MAX_CARS];
extern WORD bike_speed;

void Cars_Update(void);
void Cars_Initialize(void);
void Cars_UpdatePosition(BlitterObject *obj_car);
void Cars_LoadSprites(void);
void Cars_RestoreSaved(void);
void Cars_PreDraw(void);
void Cars_ResetPositions(void);
void Cars_CopyPristineBackground(BlitterObject *car);
void Cars_HandleSpinout(UBYTE car_index);
void Cars_CheckBikeOvertake(BlitterObject *car, WORD bike_x);
void Cars_Tick(BlitterObject *car);
void Cars_Update(void);