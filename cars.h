typedef struct {
    UBYTE *bob_data;        // 32×32×3 car graphics
    UBYTE *background;      // Saved background data  
    WORD x, y;              // Current screen position
    WORD old_x, old_y;      // Previous position for restore
    BOOL visible;           // Is BOB active
    BOOL moved;             // Position changed this frame
    BOOL needs_restore;   // Only restore if we drew last frame
    BOOL off_screen;      // Skip blits entirely if off-screen
} CarBOB;

// BOB system definitions
#define MAX_CARS 5
#define BOB_WIDTH 32
#define BOB_HEIGHT 32
#define BOB_PLANES 3
#define BOB_DATA_SIZE (BOB_WIDTH * BOB_HEIGHT * BOB_PLANES / 8)  // 384 bytes
//#define BOB_DATA_SIZE (BOB_HEIGHT * 4 * BLOCKSDEPTH)  // 32 * 4 * 4 = 512 bytes
extern CarBOB car_bobs[MAX_CARS];

void UpdateCars(void);
void InitTestBOBs(void);
void UpdateCarPosition(CarBOB *car);