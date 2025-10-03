#ifndef _ANIMATION_
#define _ANIMATION_

#include <clib/exec_protos.h>


enum ANIM_TYPE {
    CYCLIC,
    BOUNCE,
    ONESHOT
};

enum ANIM_STATUS {
    PLAYING,
    PAUSED,
    END
};

typedef struct animation {
    UWORD*          animdata;
    UWORD           size;
    UWORD           actualFrame;
    USHORT          type;
    USHORT          status;
} animation;

animation* Anim_Initialize(char* filename, USHORT anim_type);
void Anim_Free(animation* animation);
#endif