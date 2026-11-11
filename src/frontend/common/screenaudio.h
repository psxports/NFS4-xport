#ifndef SCREENAUDIO_H
#define SCREENAUDIO_H

#include "xport.h"
#include "frontend/common/fescreen.h"
#include "game/common/audiomus.h"

class tScreenAudio;

extern tScreenAudio *screenAudio;

class tScreenAudio : public tScreen
{
public:
    sint16 fShapeCount;
    char prevAudioMode;
    sint16 audioTest;
    sint32 audioTestHandle;
    sint16 fPrevSelectedSong;
    char fCurrentAudioMode;
    sint16 fSelectedSong;
    AudioMus::tSongList *songlist;
    virtual void GetShapeInfo(sint16 &numPermShapes, sint16 &numSwapShapes, char **permFileName, char **swapFileName);
    virtual void DrawBackground(void);
    virtual void DrawForeground(void);

    virtual void Initialize(void);
    virtual void Cleanup(void);
    void PlaySound(void);
    tScreenAudio(void);
};

#endif
