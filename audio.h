
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

void Music_LoadModule(BYTE track);
void Music_Play();
void Music_Stop();
void Sfx_Play();
void Sfx_Stop();
 