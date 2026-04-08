#include <exec/exec.h>
#include <dos/dos.h>
#include <intuition/intuition.h>
#include <graphics/gfx.h>
#include <graphics/gfxbase.h>
#include <devices/input.h>
#include <hardware/custom.h>
#include <hardware/intbits.h>
#include <hardware/dmabits.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/graphics.h>
#include <proto/intuition.h>
#include "sprites.h"
#include "hardware.h"




static UWORD old_dmacon;
static UWORD old_intena;
static UWORD old_adkcon;
static UWORD old_intreq;

static struct MsgPort *inputmp;
static struct IOStdReq *inputreq;
static struct Interrupt inputhandler;
static BOOL	inputdeviceok;

static struct View	*oldview;
static struct Window *old_processwinptr;
static struct Process *thisprocess;

extern struct IntuitionBase *IntuitionBase;
extern struct GfxBase *GfxBase;
extern volatile struct Custom *custom;

// Read once per frame — store in a global
static UWORD joy1dat_cache;
static UBYTE cia_cache;
static UBYTE prev_fire_state = 0;

static volatile ULONG *custom_vposr = (volatile ULONG *) 0xdff004;

 
BOOL os_disabled = FALSE;

// VERY SLOW but seems to be working RNG (good old xorshift)
ULONG rand() {
    static ULONG seed = 0xAA532051;
    seed ^= seed >> 6;
    seed ^= seed << 12;
    seed ^= seed >> 15;
    return seed;
}

static LONG NullInputHandler(void)
{
	// kills all input
	return 0;
}

void KillSystem()
{
	thisprocess = (struct Process *)FindTask(0);

	// safe actual view and install null view

	oldview = GfxBase->ActiView;
	
	LoadView(0);
	WaitTOF();
	WaitTOF();
 

	// lock blitter

	OwnBlitter();
	WaitBlit();
	
	// no multitasking/interrupts

	Disable();

	// save important custom registers
	
	old_dmacon = custom->dmaconr | 0x8000;
	old_intena = custom->intenar | 0x8000;
	old_adkcon = custom->adkconr | 0x8000;
	old_intreq = custom->intreqr | 0x8000;
    
    os_disabled = TRUE;

}

void ActivateSystem(void)
{
	// reset important custom registers
	
	custom->dmacon = 0x7FFF;
	custom->intena = 0x7FFF;
	custom->adkcon = 0x7FFF;
	custom->intreq = 0x7FFF;

	custom->dmacon = old_dmacon;
	custom->intena = old_intena;
	custom->adkcon = old_adkcon;
	custom->intreq = old_intreq;

	// enable multitasking/interrupts

	Enable();

	// unlock blitter

	WaitBlit();
	DisownBlitter();

	// enable requesters for our process
	
	thisprocess->pr_WindowPtr = old_processwinptr;

	// remove null inputhandler
	
	if (inputdeviceok)
	{
		inputreq->io_Command = IND_REMHANDLER;
		inputreq->io_Data = &inputhandler;
		DoIO((struct IORequest *)inputreq);
		
		CloseDevice((struct IORequest *)inputreq);
	}

	if (inputreq) DeleteIORequest(inputreq);
	if (inputmp) DeleteMsgPort(inputmp);

	// reset old view

	LoadView(oldview);
	WaitTOF();
	WaitTOF();	
}

void WaitVBL(void)
{
	while (((*custom_vposr) & 0x1ff00) != (262<<8)) ;
}

void WaitVBeam(ULONG line)
{
	while (1) {
		volatile ULONG vpos=*(volatile ULONG*)0xDFF004;
		if(((vpos >> 8) & 511) == line)
			break;
	}

}

void HardWaitBlit(void)
{
	if (custom->dmaconr & DMAF_BLTDONE)
	{
	}

	while (custom->dmaconr & DMAF_BLTDONE)
	{
	}
}

void HardWaitLMB(void)
{
	while (((*(UBYTE *)0xbfe001) & 64) != 0)
	{
	}

	while (((*(UBYTE *)0xbfe001) & 64) == 0)
	{
	}

}
 
void Joy_ReadAll(void)
{
    joy1dat_cache = custom->joy1dat;
    cia_cache = *(volatile UBYTE*)0xbfe001;
}

__attribute__((always_inline)) inline BOOL JoyLeft(void)
{
    return (joy1dat_cache >> 9) & 1;
}

__attribute__((always_inline)) inline BOOL JoyRight(void)
{
    return (joy1dat_cache >> 1) & 1;
}

__attribute__((always_inline)) inline BOOL JoyUp(void)
{
    return ((joy1dat_cache << 1) ^ joy1dat_cache) >> 9 & 1;
}

__attribute__((always_inline)) inline BOOL JoyDown(void)
{
    return ((joy1dat_cache << 1) ^ joy1dat_cache) >> 1 & 1;
}

__attribute__((always_inline)) inline BOOL JoyFireHeld(void)
{
    return !(cia_cache & 128);
}

__attribute__((always_inline)) inline BOOL JoyButton2(void)
{
    return !((*(volatile UWORD*)0xdff016) & (1 << 10));
}

BOOL JoyFirePressed(void)
{
    BOOL current = JoyFireHeld();
    BOOL pressed = current && !prev_fire_state;
    prev_fire_state = current;
    return pressed;
}

BOOL LMBDown(void)
{
	return ((*(UBYTE *)0xbfe001) & 64) ? FALSE : TRUE;
}

 
static UBYTE current_keycode =  0xFF;

void KeyRead(void)
{
 
    UBYTE icr = *(volatile UBYTE*)0xBFED01;
    
    if (!(icr & 0x08))
    {
        current_keycode = 0xFF;  // No new key available
        return;
    }
    
    // Read raw keycode from CIA-A SDR
    UBYTE raw = *(volatile UBYTE*)0xBFEC01;
    
    // Acknowledge: set CIA-A CRA bit 6 (SPMODE=output) to pull SP high
    *(volatile UBYTE*)0xBFEE01 |= 0x40;
    
    // Must hold for ~85μs — 3 raster lines is safe
    for (volatile int i = 0; i < 100; i++);
    
    // Release: clear SPMODE back to input
    *(volatile UBYTE*)0xBFEE01 &= ~0x40;
    
    // Decode: shift out up/down bit, invert
    current_keycode = (raw >> 1) ^ 0x7F;
    
    // Bit 0 of raw = 0 means pressed, 1 means released
    // If key-up event, ignore it
    if (raw & 0x01)
        current_keycode = 0xFF;
}


__attribute__((always_inline)) inline BOOL KeyPressed(UBYTE keycode)
{
    return (current_keycode == keycode);
}

void System_DisableOS(void)
{
    if (os_disabled == FALSE)
        return;

    /* Disable interrupts — back to game control */
    Disable();
    
    /* Take blitter back */
    OwnBlitter();
    WaitBlit();
    
    /* Kill everything, then re-enable game hardware */
    custom->intena = 0x7FFF;
    custom->dmacon = 0x7FFF;
    
    custom->dmacon = DMAF_SETCLR | DMAF_BLITTER | DMAF_RASTER | DMAF_COPPER | DMAF_SPRITE | DMAF_AUD0 | DMAF_AUD1 | DMAF_AUD2 | DMAF_AUD3 | DMAF_MASTER;
    custom->intena = INTF_SETCLR | INTF_INTEN | INTF_VERTB | INTF_EXTER;
}

void System_EnableOS(void)
{
    if (os_disabled == FALSE)
        return;
    /* Wait for any in-progress blit before releasing */
    WaitBlit();
    
    /* Release blitter to OS */
    DisownBlitter();
    
    /* Restore OS DMA + interrupts — DOS needs disk DMA */
    custom->intena = 0x7FFF;
    custom->dmacon = 0x7FFF;
    custom->dmacon = old_dmacon;
    custom->intena = old_intena;
    /* Re-enable interrupts (undoes KillSystem's Disable) */
    /* DOS needs interrupts for disk controller */
    Enable();
}


void Transition_ToBlack(void)
{
    /* Kill sprites immediately */
    Sprites_ClearLower();
    Sprites_ClearHigher();
    
    WaitVBL();
    
    /* Set all 32 colors to black */
    for (int i = 0; i < 32; i++)
        custom->color[i] = 0x000;
    
    /* Wait another frame to ensure fully black */
    WaitVBL();
}

void Transition_FromBlack(UWORD *palette, UBYTE num_colors)
{
  
    
    WaitVBL();
    
    /* Set all colors in one burst at top of frame */
    for (int i = 0; i < num_colors; i++)
        custom->color[i] = palette[i];
    
   
}