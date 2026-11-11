#ifndef SCREENTRACKS_H
#define SCREENTRACKS_H

#include "xport.h"
#include "frontend/common/screencarselect.h"

struct tTrackInformation;

class tScreenTrackSelect : public tScreen
{
public:
    virtual void DrawBackground(void);
    virtual void GetShapeInfo(sint16 &numPermShapes, sint16 &numSwapShapes, char **permFileName, char **swapFileName);
    virtual void Initialize(void);
    virtual void Cleanup(void);
    void SetBrightness(sint16 bright);
    void UpdateBrightness(tTrackInformation &trackInfo);
    void UpdateVideoWall(tTrackInformation &trackInfo);
    void DrawVideoWall(void);
    virtual void ProcessInput(tPlayer, tInputKeyType &keyval, tMenuCommand &);
    sint32 hVideo;
    sint32 fFrame;
    sint16 fPreviousTrack;
    sint16 fMovieTrack;
    sint16 fBrightness;
    sint16 fDestBrightness;
    sint16 fStartBrightness;
    uint32 fStartTicks;
    bool fTicksSet;
    tTVConfig tvConfigs[10];
    tVideoWall fVideoWall;
    bool fTVsInitialized;
    uint32 fVideoTicks;
};

extern tScreenTrackSelect *screenTrackSelect;

#endif
