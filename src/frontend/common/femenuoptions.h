#ifndef FEMENUOPTIONS_H
#define FEMENUOPTIONS_H

#include "xport.h"
#include "frontend/common/femenu.h"
#include "frontend/common/femenuextended.h"

extern sint32 PulsateYellow;
extern sint32 fHelpText;

void CalcPulsateYellow(void);
void DrawLeftFlare(sint32 y, sint32 fSelFade, sint32 fFadeVal, sint32 &flareextra);
void SubtractiveBox(sint32 x, sint32 y, sint32 w, sint32 h, sint32 col1, sint32 col2, sint32 col3, sint32 col4);

typedef sint16 tArrowLocation[6];

typedef sint16 tTextLocation[5];

class tInsideBoxMenu : public tMenu
{
public:
    tInsideBoxMenu(uint32 flags, tScreen *screenHandler, tMenu *nextMenu, tMenu *optionsMenu, void (*OnButtonPress)(tMenuCommand &), sint16 title, tMenuItem *firstItem, ...);
    tInsideBoxMenu(uint32 flags, tScreen *screenHandler, tMenu *nextMenu, tMenu *optionsMenu, void (*OnButtonPress)(tMenuCommand &), sint16 title);
    virtual ~tInsideBoxMenu(void);
    virtual void ProcessInput(tPlayer fromPlayer, tInputKeyType &keyval, tMenuCommand &command);
    virtual void Draw(sint16 x, sint16 y, sint16 w, sint16 slideOffset, sint16 maxheight);
    sint16 fPrevItem, fMoving, fMovingDir;
};

class tInsideBoxSongMenu : public tInsideBoxMenu
{
public:
    tInsideBoxSongMenu(uint32 flags, tScreen *screenHandler, tMenu *nextMenu, tMenu *optionsMenu, void (*OnButtonPress)(tMenuCommand &), sint16 title, tMenuItem *firstItem, ...);
    virtual ~tInsideBoxSongMenu(void);
    virtual void Draw(sint16 x, sint16 y, sint16 w, sint16 slideOffset, sint16 maxheight);
    virtual void DrawOneSong(sint16 songnum, sint16 x, sint16 y, sint16 w, sint16 fOnOffFade, sint16 fSelFade);
    virtual sint32 DebounceKeys(void);
    virtual void ProcessInput(tPlayer fromPlayer, tInputKeyType &keyval, tMenuCommand &command);
    sint16 fOnOffFade[5], fSelFade[5];
};

class tInsideBoxLeftRightSlider : public tMenuItemLeftRightSlider
{
public:
    tInsideBoxLeftRightSlider(uint32 textDescription, tListIterator *dataPtr);
    virtual ~tInsideBoxLeftRightSlider(void);
    virtual void Draw(sint32 x, sint32 y, sint32 w, bool selected);
};

class tInsideBoxTwoWaySlider : public tMenuItemLeftRightSlider
{
public:
    tInsideBoxTwoWaySlider(uint32 textDescription, tListIterator *dataPtr, sint32 type);
    virtual ~tInsideBoxTwoWaySlider(void);
    virtual void ProcessInput(tPlayer fromPlayer, tInputKeyType &keyval, tMenuCommand &command);
    virtual void Draw(sint32 x, sint32 y, sint32 w, bool selected);
    void Calibrate(void);
    sint16 fType;
    sint32 fActive;
};

class tInsideBoxControllerLeftRightSlider : public tInsideBoxLeftRightSlider
{
public:
    tInsideBoxControllerLeftRightSlider(uint32 textDescription, tListIterator *dataPtr);
    virtual void ProcessInput(tPlayer fromPlayer, tInputKeyType &keyval, tMenuCommand &command);
};

class tMenuItemGoToMenuButtonFade : public tMenuItemGoToMenuButton
{
public:
    tMenuItemGoToMenuButtonFade(uint32 textDescription, tMenu *newMenu, void (*OnButtonPress)(tMenuCommand &));
    virtual void TransitionOff(void);
    virtual void TransitionOn(void);
    virtual bool TransitionIsFinished(void);
    virtual void UpdateTransition(bool selected);
    sint16 fFadeVal, fFadeDir;
    sint32 fInTransition;
    sint16 fEnableVal;
};

class tBlankMenuItemGoToMenuNFS4Button : public tMenuItemGoToMenuNFS4Button
{
public:
    tBlankMenuItemGoToMenuNFS4Button(uint32 textDescription, tMenu *newMenu, void (*OnButtonPress)(tMenuCommand &), sint32 firstFrame, sint32 numFrames);
    virtual void Draw(bool selected);
    virtual void Draw(sint32 x, sint32 y, bool selected);
    virtual bool TransitionIsFinished(void);
};

class tOptionsMenu : public tMenu
{
public:
    sint32 fInMenuTransition;
    sint8 fTransitionDirection;
    sint16 fPrevItem;
    sint32 fScreenFade;
    sint32 fFirstFrame;
    sint32 fNumFrames;
    virtual ~tOptionsMenu(void);
    virtual void ProcessInput(tPlayer fromPlayer, tInputKeyType &keyval, tMenuCommand &command);
    virtual sint32 DebounceKeys(void);
    virtual void TransitionOff(void);
    virtual void TransitionOn(void);
    virtual bool TransitionIsFinished(void);
    virtual void Draw(void);
    virtual void UpdateTransition(void);
    tOptionsMenu(uint32 flags, tScreen *screenHandler, tMenu *nextMenu, tMenu *optionsMenu, void (*OnButtonPress)(tMenuCommand &), sint16 title, sint32 firstframe, sint32 numframes, tMenuItem *firstItem, ...);
};

class tMenuItemLeftRightFade : public tMenuItemLeftRightChoice
{
public:
    tMenuItemLeftRightFade(uint32 textDescription, tListIterator *dataPtr);

    virtual void TransitionOff(void);
    virtual void TransitionOn(void);
    virtual bool TransitionIsFinished(void);
    virtual void UpdateTransition(bool selected);
    void MyLeftRightDraw(sint32 x, sint32 y);
    sint16 fFadeVal, fFadeDir;
    sint32 fInTransition;
    sint32 flareextra;
};

class tMenuItemSlidingMenu : public tMenuItem
{
public:
    tMenuItemSlidingMenu(uint32 textDescription, sint16 width, sint16 height, sint16 diffx, sint16 diffy, bool fillback);
    virtual ~tMenuItemSlidingMenu(void);
    virtual void TransitionOff(void);
    virtual void TransitionOn(void);
    virtual bool TransitionIsFinished(void);
    virtual void UpdateTransition(bool selected);
    virtual sint32 DebounceKeys(void);
    virtual void Draw(bool selected);
    virtual void UpdatefOpenHeight(bool selected);
    virtual void Draw(sint32 offx, sint32 offy, bool selected);
    virtual void ProcessInput(tPlayer fromPlayer, tInputKeyType &keyval, tMenuCommand &command);
    void SetMenu(bool bothmenus, tInsideBoxMenu *menu);
    tInsideBoxMenu *currMenu, *nextMenu;
    sint16 fWidth, fHeight, fOpenHeight, fSlideOffset, fFadeVal, fFadeDir;
    sint32 fInTransition;
    sint32 fTransitioningOut;
    sint32 fClosing;
    sint16 fDiffX, fDiffY;
    sint32 fFillback;
};

class tMenuItemSlidingActivated : public tMenuItemSlidingMenu
{
public:
    tMenuItemSlidingActivated(uint32 textDescription, sint16 width, sint16 height, sint16 diffx, sint16 diffy, bool fillback);
    virtual void UpdatefOpenHeight(bool);
    virtual void TransitionOff(void);
    virtual void TransitionOn(void);
    virtual bool TransitionIsFinished(void);
    virtual void UpdateTransition(bool selected);
    virtual void ProcessInput(tPlayer fromPlayer, tInputKeyType &keyval, tMenuCommand &command);
    sint32 fActive;
};

class tMenuItemDisplayLeftRightChoice : public tMenuItemLeftRightFade
{
public:
    tMenuItemDisplayLeftRightChoice(uint32 textDescription, tListIterator *dataPtr);
    virtual void Draw(sint32 x, sint32 y, bool selected);
};

class tMenuItemOnOffLeftRightChoice : public tMenuItemLeftRightFade
{
public:
    tMenuItemOnOffLeftRightChoice(uint32 textDescription, tListIterator *dataPtr);
    virtual void TransitionOn(void);
    virtual void Draw(sint32 x, sint32 y, bool selected);
    sint16 fOnFade;
};

class tMenuItemLeftRightAudioSlider : public tMenuItemLeftRightSlider
{
public:
    tMenuItemLeftRightAudioSlider(uint32 textDescription, tListIterator *dataPtr, sint32 AudioArt);
    virtual ~tMenuItemLeftRightAudioSlider(void);
    virtual void Draw(sint32 ox, sint32 oy, bool);
    sint32 Percentage(void);
    virtual void TransitionOff(void);
    virtual void TransitionOn(void);
    virtual bool TransitionIsFinished(void);
    virtual void UpdateTransition(bool selected);
    sint16 fFadeVal, fFadeDir;
    sint32 fInTransition;
    sint16 fAudioArt;
    sint32 flareextra;
};

class tMenuItemControllerLeftRightChoice : public tMenuItemLeftRightFade
{
public:
    tMenuItemControllerLeftRightChoice(uint32 textDescription, tListIterator *dataPtr);
    virtual void Draw(sint32 x, sint32 y, bool selected);
};

class tUserNameMenuItem : public tMenuItem
{
public:
    tUserNameMenuItem(uint32 textDescription);

    virtual void ProcessInput(tPlayer, tInputKeyType &keyval, tMenuCommand &);
    virtual void Draw(bool);
    virtual void TransitionOff(void);
    virtual void TransitionOn(void);
    virtual bool TransitionIsFinished(void);
    virtual void UpdateTransition(bool selected);
    char *fData;
    sint16 fMaxStringLength, fCurrentColumn, fCurrentRow;
    char fRowList[10][9];
    sint16 fPlayer, fFadeVal, fFadeDir;
    sint32 fInTransition;
};

#endif
