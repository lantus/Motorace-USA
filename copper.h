// Custom chipset register addresses
#define COLOR00     0x180
#define COLOR01     0x182
#define COLOR02     0x184
#define COLOR03     0x186
#define COLOR04     0x188
#define COLOR05     0x18A
#define COLOR06     0x18C
#define COLOR07     0x18E

#define COLOR08     0x190
#define COLOR09     0x192
#define COLOR10     0x194
#define COLOR11     0x196
#define COLOR12     0x198
#define COLOR13     0x19A
#define COLOR14     0x19C
#define COLOR15     0x19E

#define COLOR16     0x1A0
#define COLOR17     0x1A2
#define COLOR18     0x1A4
#define COLOR19     0x1A6
#define COLOR20     0x1A8
#define COLOR21     0x1AA
#define COLOR22     0x1AC
#define COLOR23     0x1AE

#define COLOR24     0x1B0
#define COLOR25     0x1B2
#define COLOR26     0x1B4
#define COLOR27     0x1B6
#define COLOR28     0x1B8
#define COLOR29     0x1BA
#define COLOR30     0x1BC
#define COLOR31     0x1BE
 

#define FETCHMODE   0x1FC
#define BPLCON0     0x100
#define BPLCON1     0x102
#define BPLCON3     0x106
#define BPLMODA     0x108
#define BPLMODB     0x10A
#define DIWSTRT     0x8E
#define DIWSTOP     0x90
#define DDFSTRT     0x92
#define DDFSTOP     0x94
#define BPL1PTH     0xE0
#define BPL1PTL     0xE2
#define BPL2PTH     0xE4
#define BPL2PTL     0xE6
#define BPL3PTH     0xE8
#define BPL3PTL     0xEA
#define BPL4PTH     0xEC
#define BPL4PTL     0xEE
#define BPL5PTH     0xF0
#define BPL5PTL     0xF2
#define BPL6PTH     0xF4
#define BPL6PTL     0xF6
#define BPL7PTH     0xF8
#define BPL7PTL     0xFA
#define BPL8PTH     0xFC
#define BPL8PTL     0xFE

#define DIWSTARTVAL 0x29A1  // V:$29 (41), H:$A1 (161) 
#define DIWSTOPVAL  0x29E1  // V:$29 (297), H:$E1 (225)

#define val 1

extern WORD	CopperList[];

extern WORD	CopSPR0PTH[];
extern WORD	CopSPR0PTL[];
extern WORD	CopSPR1PTH[];
extern WORD	CopSPR1PTL[];
extern WORD	CopSPR2PTH[];
extern WORD	CopSPR2PTL[];
extern WORD	CopSPR3PTH[];
extern WORD	CopSPR3PTL[];
extern WORD	CopSPR4PTH[];
extern WORD	CopSPR4PTL[];
extern WORD	CopSPR5PTH[];
extern WORD	CopSPR5PTL[];
extern WORD	CopSPR6PTH[];
extern WORD	CopSPR6PTL[];
extern WORD	CopSPR7PTH[];
extern WORD	CopSPR7PTL[];

extern WORD	CopCOLOR00[];
extern WORD	CopCOLOR01[];
extern WORD	CopCOLOR02[];
extern WORD	CopCOLOR03[];
extern WORD	CopCOLOR04[];
extern WORD	CopCOLOR05[];
extern WORD	CopCOLOR06[];
extern WORD	CopCOLOR07[];
extern WORD	CopCOLOR08[];
extern WORD	CopCOLOR09[];
extern WORD	CopCOLOR10[];
extern WORD	CopCOLOR11[];
extern WORD	CopCOLOR12[];
extern WORD	CopCOLOR13[];
extern WORD	CopCOLOR14[];
extern WORD	CopCOLOR15[];
extern WORD	CopCOLOR16[];
extern WORD	CopCOLOR17[];
extern WORD	CopCOLOR18[];
extern WORD	CopCOLOR19[];
extern WORD	CopCOLOR20[];
extern WORD	CopCOLOR21[];
extern WORD	CopCOLOR22[];
extern WORD	CopCOLOR23[];
extern WORD	CopCOLOR24[];
extern WORD	CopCOLOR25[];
extern WORD	CopCOLOR26[];
extern WORD	CopCOLOR27[];
extern WORD	CopCOLOR28[];
extern WORD	CopCOLOR29[];
extern WORD	CopCOLOR30[];
extern WORD	CopCOLOR31[];
extern WORD CopFETCHMODE[];
extern WORD CopBPLCON0[];
extern WORD	CopBPLCON1[];
extern WORD CopBPLCON3[];
extern WORD CopBPLMODA[];
extern WORD CopBPLMODB[];
extern WORD CopDIWSTART[];
extern WORD CopDIWSTOP[];
extern WORD CopDDFSTART[];
extern WORD CopDDFSTOP[];
extern WORD CopPLANE1H[];
extern WORD CopPLANE1L[];
extern WORD CopPLANE2H[];
extern WORD CopPLANE2L[];
extern WORD CopPLANE3H[];
extern WORD CopPLANE3L[];
extern WORD CopPLANE4H[];
extern WORD CopPLANE4L[];
extern WORD CopPLANE5H[];
extern WORD CopPLANE5L[];
extern WORD CopPLANE6H[];
extern WORD CopPLANE6L[];
extern WORD CopPLANE7H[];
extern WORD CopPLANE7L[];
extern WORD CopPLANE8H[];
extern WORD CopPLANE8L[];

extern WORD CopVIDEOSPLIT[];
extern WORD CopVIDEOSPLITMODULO[];
extern WORD CopVIDEOSPLIT2[];

extern WORD CopPLANE2_1H[];
extern WORD CopPLANE2_2H[];
extern WORD CopPLANE2_3H[];
extern WORD CopPLANE2_4H[];
extern WORD CopPLANE2_5H[];
extern WORD CopPLANE2_6H[];
extern WORD CopPLANE2_7H[];
extern WORD CopPLANE2_8H[];

extern WORD CopVIDEOSPLITRESETMODULO[];
 
void Copper_SetBitplanePointer(int plane, const UBYTE **planes, LONG offset);
void Copper_SetPalette(int index,USHORT color);
void Copper_SetSpritePalette(int index, USHORT color);
void Copper_SetHUDBitplanes(const UBYTE *hud_buffer) ;
void Copper_SwapColors(UWORD color1, UWORD color2);