#ifndef SCREENTROPHYROOM_H
#define SCREENTROPHYROOM_H

#include "xport.h"
#include "frontend/common/fescreen.h"

class tScreenTrophyRoom : public tScreen
{
public:
    tShapeInformation fTrophyShapes;
    sint16 fNumTrophies;
    sint32 startTicks;
    sint16 fShapeCount;
    sint32 fLoadingTrophy;
    char fPreviousTrophy;
    char fDoUpdate;
    sint32 fClearScreen;
    char fBrightness;
    uint32 fStartTicks;
    sint16 fTextInfo[16];
    char thisisuseless;
    sint32 tier;
    sint16 fRealCurrentTourn[2];
    sint16 fTrophyList[64];
    virtual void GetShapeInfo(sint16 &numPermShapes, sint16 &numSwapShapes, char **permFileName, char **swapFileName);
    virtual void DrawBackground(void);
    virtual ~tScreenTrophyRoom(void);
    virtual void PreLoad(void);
    virtual void Initialize(void);
    virtual void Cleanup(void);
    virtual void ProcessInput(tPlayer fromPlayer, tInputKeyType &keyval, tMenuCommand &command);
    tScreenTrophyRoom(void);
    void LoadTrophy(void);
};

extern tScreenTrophyRoom *screenTrophyRoom;

#endif
