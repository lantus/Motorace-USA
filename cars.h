 
#define CAR1_FILE "objects/car.BPL"
#define CAR1_MASK "objects/carmask.BPL"

// BOB system definitions
#define MAX_CARS 5
#define BOB_WIDTH 32
#define BOB_HEIGHT 32
#define BOB_PLANES 4
#define BOB_DATA_SIZE (BOB_WIDTH * BOB_HEIGHT * BOB_PLANES / 8)
extern BlitterObject car[MAX_CARS];

void Cars_Update(void);
void Cars_Initialize(void);
void Cars_UpdatePosition(BlitterObject *obj_car);
void Cars_LoadSprites(void);
void Cars_RestoreSaved(void);
void Cars_PreDraw(void);
void Cars_ResetPositions(void);