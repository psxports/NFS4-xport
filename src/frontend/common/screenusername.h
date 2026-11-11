#ifndef SCREENUSERNAME_H
#define SCREENUSERNAME_H

#include "xport.h"
#include "frontend/common/femenuoptions.h"
#include "frontend/common/fescreen.h"

extern sint16 menu_kUserNameRows;
extern sint16 MENUUSERNAME_STARTY;

class tScreenUserName : public tScreen
{
public:
    virtual void Initialize(void);
    virtual void GetShapeInfo(sint16 &numPermShapes, sint16 &numSwapShapes, char **permFileName, char **swapFileName);
    void DrawVerticalLine(sint16 x, sint16 y, sint16 gridpos);
    void DrawHorizontalLine(sint16 x, sint16 y, sint16 gridpos);
    virtual void DrawBackground(void);
    tOptionsMenu *callingMenu;
    sint16 fTextFade;
    sint32 fInTransition;
    char fRowList[10][9];
};

extern tScreenUserName *screenUserName;

#endif
