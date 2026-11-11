#ifndef FEFADES_H
#define FEFADES_H

#include "xport.h"
#include "psx.h"
#include "frontend/common/fetextrender.h"

extern sint32 kRGBVals[28];

sint32 CalcFadeVal(sint32 col1, sint32 col2, sint32 amount);
sint32 CalcFadeVal(sint32 col1, sint32 amount);
sint32 CalcFadeVal(sint32 col1, sint32 col2, sint32 amount, sint32 fFade);

sint32 CalcTextFadeUnselToSel(tMenuTextType type, sint16 fSelFade, sint16 fFade);
sint32 CalcTextFadeSelToHi(tMenuTextType type, sint16 fSelFade, sint16 fFade);
void CalcOnOffFade(tMenuTextType type, sint16 fOnOffFade, sint16 fSelFade, sint16 fFade, sint32 &OnColor, sint32 &OffColor);

#endif
