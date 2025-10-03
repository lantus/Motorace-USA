#include "animation.h"
#include "memory.h"
#include "disk.h"
#include <clib/exec_protos.h>

animation* Anim_Initialize(char* filename, USHORT anim_type) 
{
    animation* new_animation = Mem_AllocFast(sizeof(animation));  // Allowed to stay in fast ram
    new_animation->animdata = Disk_AllocAndLoadAsset(filename, MEMF_FAST);
    new_animation->size = new_animation->animdata[0];
    new_animation->actualFrame = 0;
    new_animation->type = anim_type;
    new_animation->status = PLAYING;

    return new_animation;
}

void Anim_Free(animation* animation) 
{
    Mem_Free(animation->animdata, animation->size);
    Mem_Free(animation, sizeof(animation));
}