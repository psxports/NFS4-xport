#ifndef SCREENTRACKINFO_H
#define SCREENTRACKINFO_H

#include "xport.h"
#include "frontend/common/screencarselect.h"
#include "frontend/common/trackinfo.h"

class tScreenTrackInfo : public tScreen
{
public:
    virtual void GetShapeInfo(sint16 &numPermShapes, sint16 &numSwapShapes, char **permFileName, char **swapFileName);
    virtual void DrawBackground(void);
    virtual void Initialize(void);
    virtual void ProcessInput(tPlayer fromPlayer, tInputKeyType &keyval, tMenuCommand &command);
    tTrackInfo fTrack;
    tTVConfig tvConfigs[10];
    tVideoWall fVideoWall;
};

extern tScreenTrackInfo *screenTrackInfo;

#endif
