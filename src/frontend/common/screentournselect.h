#ifndef SCREENTOURNSELECT_H
#define SCREENTOURNSELECT_H

#include "xport.h"
#include "frontend/common/fetourn.h"
#include "frontend/common/screenpinkslips.h"

class tScreenTournSelect : public tScreen
{
public:
    tScreenTournSelect(void);
    virtual ~tScreenTournSelect(void);
    virtual void GetShapeInfo(sint16 &numPermShapes, sint16 &numSwapShapes, char **permFileName, char **swapFileName);
    virtual void Initialize(void);
    virtual void Cleanup(void);
    void UpdateVideoWall(tTourneyInfo *tourn);
    void DrawVideoWall(void);
    virtual void DrawBackground(void);
    virtual void DrawForeground(void);
    sint32 hVideo;
    sint32 fFrame;
    tTVConfig tvConfigs[8];
    tTVConfig trophyTV[4];
    sint16 fPreviousMovie;
    sint16 fCurrentMovie;
    uint32 fStartTicks;
    uint32 fTVTicks;
    sint16 fTransitionDirection;
    char fPreviousTrophy;
    sint32 fTVsInitialized;
    sint32 PreCalculatedTournamentY;
    sint32 fPrevi;
};

extern tScreenTournSelect *screenTournSelect;

#endif
