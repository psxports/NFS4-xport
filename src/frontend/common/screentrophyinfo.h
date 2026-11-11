#ifndef SCREENTROPHYINFO_H
#define SCREENTROPHYINFO_H

#include "xport.h"
#include "frontend/common/fescreen.h"

class tScreenTrophyInfo : public tScreen
{
public:
    sint32 BannerCol;
    virtual void GetShapeInfo(sint16 &numPermShapes, sint16 &numSwapShapes, char **permFileName, char **swapFileName);
    virtual void DrawBackground(void);

};

extern tScreenTrophyInfo *screenTrophyInfo;

#endif
