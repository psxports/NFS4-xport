#ifndef SCREENPINKSLIPS_H
#define SCREENPINKSLIPS_H

#include "xport.h"
#include "frontend/common/fescreen.h"

struct tTrackInformation;

enum tTVState
{
    tv_StateOff = 0,
    tv_StateOn = 1,
    tv_TransitionOn = 2,
    tv_TransitionOff = 3,
};

struct tTVConfig
{
    tTVState state;
    sint16 transition;
    uint16 destBrightness;
    uint16 flags;
    sint16 fxWide;
    sint16 fxThin;
    sint32 tint;
    sint16 x;
    sint16 y;
    sint16 w;
    sint16 h;
    uint8 u;
    uint8 v;
    uint8 uw;
    uint8 vh;
    uint8 shapeType;
    uint16 clut;
    uint16 tpage;
    uint16 shapex;
    uint16 shapey;
    uint16 clutID;
    uint16 flip_axis;
};

class tScreenPinkSlips : public tScreen
{
public:
    tMenu *fMenu;
    sint32 hVideo;
    sint32 fFrame;
    sint16 fPreviousTrack;
    sint16 fBrightness;
    sint16 fDestBrightness;
    sint16 fStartBrightness;
    uint32 fStartTicks;
    uint32 fTVTicks;
    sint8 fTransitionDirection;
    tTVConfig fTrackTVs[8];
    tTVConfig fImageTVs[4];
    bool fTVsInitialized;
    virtual void GetShapeInfo(sint16 &numPermShapes, sint16 &numSwapShapes, char **permFileName, char **swapFileName);
    virtual void DrawBackground(void);

    virtual void Initialize(void);
    virtual void Cleanup(void);
    virtual void ProcessInput(tPlayer fromPlayer, tInputKeyType &keyval, tMenuCommand &command);
    void UpdateVideoWall(tTrackInformation &trackInfo);
    void DrawVideoWall(void);
};

extern tScreenPinkSlips *screenPinkSlips;

#endif
