
#define BPL0_USEBPLCON3_F  0x1
#define BPL0_COLOR_F       0x200
#define BPL0_BPU0_F        0x1000
#define BPL0_BPU3_F        0x10
#define BPL0_BPUMASK	   0x7000

#define KEY_P       0x19
#define KEY_SPACE   0x40
#define KEY_ESC     0x45
#define KEY_F1      0x50

extern BOOL os_disabled;
 
extern BOOL  g_is_pal;
extern UWORD g_sprite_hoffset;   // Horizontal offset (usually same for both)
extern UWORD g_sprite_voffset;   // Vertical offset (differs PAL/NTSC)
extern UWORD g_screen_height ;   // Screen height

void KillSystem(void);
void ActivateSystem(void);
void WaitVBL(void);
void WaitVBeam(ULONG line);
void HardWaitBlit(void);
void HardWaitLMB(void);

void Joy_ReadAll(void);

UBYTE ReadKeycode(void);
BOOL KeyPressed(UBYTE keycode);
void KeyRead(void);
BOOL JoyButton2(void);
BOOL JoyLeft(void);
BOOL JoyRight(void);
BOOL JoyUp(void);
BOOL JoyDown(void);
BOOL JoyFireHeld(void);
BOOL JoyFirePressed(void);
BOOL LMBDown(void);
 