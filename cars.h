typedef struct 
{
    BitMapEx *bob;
    UBYTE *background;      // Saved background data  
    UBYTE *mask;            // Add this
    WORD x, y;              // Current screen position
    WORD old_x, old_y;      // Previous position for restore
    BOOL visible;           // Is BOB active
    BOOL moved;             // Position changed this frame
    BOOL needs_restore;   // Only restore if we drew last frame
    BOOL off_screen;      // Skip blits entirely if off-screen
} Car;

#define CAR1_FILE "objects/car.BPL"
#define CAR1_MASK "objects/carmask.BPL"

// BOB system definitions
#define MAX_CARS 4
#define BOB_WIDTH 32
#define BOB_HEIGHT 32
#define BOB_PLANES 4
#define BOB_DATA_SIZE (BOB_WIDTH * BOB_HEIGHT * BOB_PLANES / 8)
extern Car car[MAX_CARS];

void Cars_Update(void);
void Cars_Initialize(void);
void Cars_UpdatePosition(Car *obj_car);
void Cars_LoadSprites();