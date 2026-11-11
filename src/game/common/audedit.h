#ifndef AUDEDIT_H
#define AUDEDIT_H

#include "xport.h"

struct CAudioList
{
    sint32 id_;
    sint32 numElements_;
    sint32 slice_;
    sint32 versionNumber_;
};

class AudList
{
public:
    static void PurgeAudio(void);
    static void LoadAudioFile(sint32 AudioFileIndex);
};

extern CAudioList *gGameAudioList;

#endif
