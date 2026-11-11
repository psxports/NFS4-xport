#ifndef FETEXTRENDER_H
#define FETEXTRENDER_H

#include "xport.h"

enum tMenuTextState
{
    textState_Unselected = 0,
    textState_Selected = 1,
    textState_Hilighted = 2,
    textState_NumStates = 3,
};

enum tMenuTextType
{
    textType_Title = 0,
    textType_FlybyHelp = 1,
    textType_BorderInfo = 2,
    textType_FramedInfo = 3,
    textType_ScreenInfo = 4,
    textType_ScreenInfoHeader = 5,
    textType_Options = 6,
    textType_FramedMoney = 7,
    textType_PopUpText = 8,
    textType_PopUpTitle = 9,
    textType_PostGame = 10,
    textType_TrackRecords = 11,
    textType_UserNameMenu = 12,
    textType_VideoWall = 13,
    textType_NumTypes = 14,
    textType_Default = 14,
};

class FETextRender
{
public:
    static void SetFont(sint32 size);
    static void FullTextRGB(char *sMenuText, sint16 x, sint16 y, sint32 col, char size, sint16 justify);
    static void FullText(char *sMenuText, sint16 x, sint16 y, tMenuTextType textType, tMenuTextState textState, sint16 justify);
    static void FullTextFade(sint32 fade, char *sMenuText, sint16 x, sint16 y, tMenuTextType textType, tMenuTextState textState, sint16 justify);
    static void MenuTextPositioned(sint16 index, sint16 x, sint16 y, tMenuTextState textState, tMenuTextType textType);
    static void MenuTextFade(sint32 fade, sint16 index, tMenuTextState textState, tMenuTextType textType);
    static void MenuTextPositionedJustify(sint16 index, sint16 x, sint16 y, sint16 justify, tMenuTextState state, tMenuTextType type);
    static void MenuTextPositionedJustifyFade(sint32 fade, sint16 index, sint16 x, sint16 y, sint16 justify, tMenuTextState textState, tMenuTextType textType);
    static sint32 WordWrapTextRGBJustify(char *str, PSX_RECT &r, sint32 col, sint32 justify, sint32 size, bool JustGrabHeight);
    static void WordWrapTextRGB(char *str, PSX_RECT &r, sint32 Col);
    static void WordWrapTextFade(sint32 fade, char *str, PSX_RECT &r, tMenuTextState textState, tMenuTextType textType);
    static void WordWrapText(char *str, PSX_RECT &r, tMenuTextState textState, tMenuTextType textType);
    static void WordWrapFade(sint32 fade, sint16 index, PSX_RECT &r, tMenuTextState state, tMenuTextType type);
    static void WordWrap(sint16 index, PSX_RECT &r, tMenuTextState textState, tMenuTextType textType);
    static sint32 WordWrapHeight(sint16 width, char *str);
    static void Title(sint16 index);
    static void SetABR(sint32 abr, bool trans);
};

extern sint32 gSemiTransText;
extern char textDefinitions[14][6];

#endif
