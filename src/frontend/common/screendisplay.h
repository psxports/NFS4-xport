#ifndef SCREENDISPLAY_H
#define SCREENDISPLAY_H

#include "xport.h"
#include "frontend/common/fescreen.h"

class tScreenDisplay : public tScreen
{
public:
    virtual void GetShapeInfo(sint16 &numPermShapes, sint16 &numSwapShapes, char **permFileName, char **swapFileName);
    virtual void DrawBackground(void);

};

extern tScreenDisplay *screenDisplay;

#endif
