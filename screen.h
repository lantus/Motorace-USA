#ifndef _SCREEN_
#define _SCREEN_

#include <clib/exec_protos.h>

// Diwstart e stop
#define DIWSTRT_VALUE      0x2c81
#define DIWSTOP_VALUE_PAL  0x2cc1

// Data fetch
#define DDFSTRT_VALUE      0x0038
#define DDFSTOP_VALUE      0x00d0

// copper instruction macros
#define COP_MOVE(addr, data) addr, data
#define COP_WAIT_END  0xffff, 0xfffe

// Indici array copperlist
#define BPLCON0_VALUE_IDX (43)
#define BPL1PTH_VALUE_IDX (49)
#define SPR0PTH_VALUE_IDX (3)
#define BPL1MOD_VALUE_IDX (45)
#define BPL2MOD_VALUE_IDX (47)
 
#define BPLCON0_4BPP_COMPOSITE_COLOR 0x4200

struct AmigaScreen {
    UWORD   bytes_width;
    UWORD   height;
    UBYTE   depth;
    UBYTE   left_margin_bytes;
    UBYTE   right_margin_bytes;
    UBYTE   display_start_offset_bytes;
    BOOL    dual_playfield;
    UBYTE*  bitplanes;
    UBYTE*  offscreen_bitplanes;
    ULONG   framebuffer_size;
    UWORD   row_size;
    ULONG   offset;
};

extern struct AmigaScreen screen;
extern struct AmigaScreen off_screen;
extern UBYTE screenbuffer_idx;
extern UBYTE* current_screen_bitplanes;


void Screen_Initialize(
    UWORD   width,
    UWORD   height,
    UBYTE   depth,
    BOOL    dual_playfield);

void Screen_Initialize_DoubleBuff(
UWORD   width,
UWORD   height,
UBYTE   depth,
BOOL    dual_playfield);

void Screen_FadePalette(UWORD* rawPalette,UWORD* copPalette,USHORT frame,USHORT colors);


#endif