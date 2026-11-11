#ifndef SCREENMAIN_H
#define SCREENMAIN_H

#include "xport.h"
#include "frontend/common/screenpinkslips.h"

typedef sint32 STREAMHANDLE;

typedef sint32 STREAMREQUESTID;

enum tScreenMainState
{
    kScreenMain_Off = 0,
    kScreenMain_StaticImage = 1,
    kScreenMain_DynamicImage = 2,
    kScreenMain_WarningImage = 3,
    kScreenMain_Credits = 4
};

struct tVideoTransition
{
    tScreenMainState state;
    uint16 flags;
    uint8 u;
    uint8 v;
    uint8 uw;
    uint8 vh;
    uint16 clut;
    uint16 tpage;
    uint32 tint;
    sint16 bright;
};

class tScreenMain : public tScreen
{
public:
    void SwapBackground(sint32 num);
    bool DoneLoadingBackground(void);
    void SetState(tScreenMainState state);
    void InitDynamicImages(void);
    virtual void ProcessInput(tPlayer, tInputKeyType &keyval, tMenuCommand &);
    void DrawDropShadow(void);
    void DrawVideoLines(void);
    virtual void DrawBackground(void);
    virtual void GetShapeInfo(sint16 &numPermShapes, sint16 &numSwapShapes, char **permFileName, char **swapFileName);
    virtual void PreLoad(void);
    virtual void Initialize(void);
    virtual void Cleanup(void);
    sint32 hVideo;
    sint32 fFrame;
    uint32 fStartTicks;
    uint32 fAnimTicks;
    sint16 fAnimLocation;
    tScreenMainState fState;
    tTVConfig tvConfigs[16];
    tScreenMainState tvStates[16];
    tVideoTransition tvTransitions[16];
    sint32 fTVsInitialized;
    char fTransitionDirection;
    sint32 fAnimationUploaded;
    sint16 fPreviousAnim;
    sint16 fWarningFade;
    sint16 fPreviousMovie;
    sint16 fCurrentMovie;
    bool bVideoAborted;
    uint32 fMovieTicks;
    tShapeInformation fVideoShapes[2];
    sint32 fCurrentSlot;
    sint32 fCurrentBG[2];
    sint32 fNumTVsInTransition;
};

extern tScreenMain *screenMain;

#endif
