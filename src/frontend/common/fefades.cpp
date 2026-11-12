#include "xport.h"
#include "frontend/common/fefades.h"
#include "frontend/common/fetextrender.h"

extern sint32 kRGBVals[28];

sint32 CalcFadeVal(sint32 col1, sint32 col2, sint32 amount)
{
    FUNCTION_MARKER(0x8004B7F8, "FRONT.BIN");
    uint32 complement = 128 - (uint32)amount;
    uint32 first = col1;
    uint32 second = col2;
    sint32 r = (sint32)(complement * (first & 255U) + amount * (second & 255U)) >> 7;
    sint32 g = (sint32)(complement * ((first >> 8) & 255U) + amount * ((second >> 8) & 255U)) >> 7;
    sint32 b = (sint32)(complement * ((first >> 16) & 255U) + amount * ((second >> 16) & 255U)) >> 7;
    return (((uint32)b << 16) | ((uint32)g << 8) | r);
}

sint32 CalcFadeVal(sint32 col1, sint32 amount)
{
    FUNCTION_MARKER(0x8004B888, "FRONT.BIN");
    return CalcFadeVal(col1, 0, amount);
}

sint32 CalcFadeVal(sint32 col1, sint32 col2, sint32 amount, sint32 fFade)
{
    FUNCTION_MARKER(0x8004B8AC, "FRONT.BIN");
    sint32 col = CalcFadeVal(col1, col2, amount);
    return CalcFadeVal(col, 0, fFade);
}

sint32 CalcTextFadeUnselToSel(tMenuTextType type, sint16 fSelFade, sint16 fFade)
{
    FUNCTION_MARKER(0x8004B8E0, "FRONT.BIN");
    sint32 first = kRGBVals[(uint8)textDefinitions[type][3]];
    sint32 second = kRGBVals[(uint8)textDefinitions[type][4]];
    return CalcFadeVal(first, second, fSelFade, fFade);
}

sint32 CalcTextFadeSelToHi(tMenuTextType type, sint16 fSelFade, sint16 fFade)
{
    FUNCTION_MARKER(0x8004B950, "FRONT.BIN");
    sint32 first = kRGBVals[(uint8)textDefinitions[type][4]];
    sint32 second = kRGBVals[(uint8)textDefinitions[type][5]];
    sint32 col = CalcFadeVal(first, second, fSelFade);
    return CalcFadeVal(col, 0, fFade);
}

void CalcOnOffFade(tMenuTextType type, sint16 fOnOffFade, sint16 fSelFade, sint16 fFade, sint32 &OnColor, sint32 &OffColor)
{
    FUNCTION_MARKER(0x8004B9D4, "FRONT.BIN");
    sint32 selected = kRGBVals[(uint8)textDefinitions[type][4]];
    sint32 highlighted = kRGBVals[(uint8)textDefinitions[type][5]];
    sint32 unselected = kRGBVals[(uint8)textDefinitions[type][3]];
    sint32 ColSelOn = CalcFadeVal(selected, highlighted, fOnOffFade);
    sint32 ColSelOff = CalcFadeVal(highlighted, selected, fOnOffFade);
    sint32 ColUnSelOn = CalcFadeVal(unselected, selected, fOnOffFade);
    sint32 ColUnSelOff = CalcFadeVal(selected, unselected, fOnOffFade);
    OnColor = CalcFadeVal(ColUnSelOn, ColSelOn, fSelFade, fFade);
    OffColor = CalcFadeVal(ColUnSelOff, ColSelOff, fSelFade, fFade);
}
