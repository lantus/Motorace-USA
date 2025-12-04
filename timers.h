#ifndef TIMERS_H
#define TIMERS_H

#include <exec/types.h>

extern BOOL  g_is_pal;

// Timer structure - you can create as many as you need
typedef struct {
    ULONG start_tick;
    ULONG duration_ticks;
    BOOL active;
    BOOL paused;
    ULONG pause_tick;
} GameTimer;

// Initialize timer system (call once at startup)
void Timer_Init(void);

// Call this in your VBlank interrupt
void Timer_VBlankUpdate(void);

// Get current tick count
ULONG Timer_GetTicks(void);

// Get detected refresh rate (50 or 60)
UWORD Timer_GetRefreshRate(void);

// Timer control functions
void Timer_Start(GameTimer *timer, UWORD seconds);
void Timer_StartMs(GameTimer *timer, UWORD milliseconds);
void Timer_StartFrames(GameTimer *timer, UWORD frames);
void Timer_Stop(GameTimer *timer);
void Timer_Reset(GameTimer *timer);
void Timer_Pause(GameTimer *timer);
void Timer_Resume(GameTimer *timer);

// Timer query functions
BOOL Timer_HasElapsed(GameTimer *timer);
ULONG Timer_GetElapsedMs(GameTimer *timer);
ULONG Timer_GetElapsedSeconds(GameTimer *timer);
ULONG Timer_GetElapsedFrames(GameTimer *timer);
ULONG Timer_GetRemainingMs(GameTimer *timer);
ULONG Timer_GetRemainingSeconds(GameTimer *timer);
BOOL Timer_IsActive(GameTimer *timer);
BOOL Timer_IsPaused(GameTimer *timer);

// Simple delay functions (blocking - use sparingly!)
void Timer_DelayFrames(UWORD frames);
void Timer_DelayMs(UWORD milliseconds);

#endif // TIMERS_H