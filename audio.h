
extern void *current_track;
extern BOOL g_is_music_ready;

enum MusicTrack
{
    MUSIC_START = 0,
    MUSIC_ONROAD = 1,
    MUSIC_CHECKPOINT = 2,
    MUSIC_RANKING = 3,
    MUSIC_OFFROAD = 4,
    MUSIC_FRONTVIEW = 5,
    MUSIC_VIVANY = 6,
    MUSIC_GAMEOVER = 7,
};

enum SoundEffect
{
    SFX_CRASH = 0,
    SFX_SKID = 1,
    SFX_HONK = 2,
    SFX_CRASHSKID = 3
} ;

// Audio channels (Amiga has 4 hardware channels)
typedef enum {
    SFX_CHANNEL_0 = 0,
    SFX_CHANNEL_1 = 1,
    SFX_CHANNEL_2 = 2,
    SFX_CHANNEL_3 = 3
} SFXChannel;

// SFX handle for loaded sounds
typedef struct {
    UBYTE *sample_data;      // Pointer to 8-bit sample data (CHIP RAM)
    ULONG sample_length;     // Length in bytes
    UWORD sample_rate;       // Playback rate (e.g., 8000, 11025, 22050)
    UWORD volume;            // Default volume (0-64)
    BOOL loaded;             // Is this SFX loaded?
} SFXHandle;


extern UBYTE channel_play_count[4];

void Audio_Initialize(void);
void Music_LoadModule(BYTE track);
void Music_Play();
void Music_Stop();
void SFX_Play(UBYTE effect);
 
 
BOOL SFX_LoadRaw(const char *filename, SFXHandle *sfx, UWORD sample_rate);

// Free an SFX handle
void SFX_Free(SFXHandle *sfx);

// Play an SFX on specified channel
// channel: 0-3 (Amiga audio channels)
// volume: 0-64 (0=silent, 64=max), use 0 to use default from file
void SFX_PlayEffect(SFXHandle *sfx, SFXChannel channel, UWORD volume);

// Stop playback on a channel
void SFX_Stop(SFXChannel channel);

// Stop all SFX playback
void SFX_StopAll(void);

// Set master volume for all SFX (0-64)
void SFX_SetMasterVolume(UWORD volume);