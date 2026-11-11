#ifndef FEMENUEXTENDED_H
#define FEMENUEXTENDED_H

#include "xport.h"
#include "psx.h"
#include "frontend/common/femenu.h"

class tMenuItemGoToMenuNFS4Button : public tMenuItemGoToMenuButton
{
public:
    tMenuItemGoToMenuNFS4Button(uint32 textDescription, tMenu *newMenu, void (*OnButtonPress)(tMenuCommand &), sint32 firstFrame, sint32 numFrames);
    virtual ~tMenuItemGoToMenuNFS4Button(void);

    virtual void Draw(bool)
    {
        FUNCTION_MARKER(0x8001BF40, "FRONT.BIN");
    }

    virtual void Draw(sint32 x, sint32 y, bool selected);
    virtual void TransitionOn(void);
    virtual void TransitionOff(void);
    virtual bool TransitionIsFinished(void);
    virtual void UpdateTransition(bool selected);

    sint32 fOrdinalPos;
    sint16 fOffset;
    sint16 fTransitionVal;
    sint16 fTransitionSpeed;
    sint16 fEnabledTransitionVal;
};

class tMenuItemNFS4LeftRightChoice : public tMenuItemLeftRightChoice
{
public:
    tMenuItemNFS4LeftRightChoice(uint32 textDescription, tListIterator *dataPtr, sint32 firstFrame, sint32 numFrames);
    virtual ~tMenuItemNFS4LeftRightChoice(void);
    virtual void Draw(sint32 x, sint32 y, bool selected);
    virtual void TransitionOn(void);
    virtual void TransitionOff(void);
    virtual bool TransitionIsFinished(void);
    virtual void UpdateTransition(bool selected);

    sint16 fOffset;
    sint16 fTransitionVal;
    sint16 fTransitionSpeed;
    sint16 fEnabledTransitionVal;
};

class tBlankMenuItemNFS4LeftRightChoice : public tMenuItemNFS4LeftRightChoice
{
public:
    tBlankMenuItemNFS4LeftRightChoice(uint32 textDescription, tListIterator *dataPtr, sint32 firstFrame, sint32 numFrames);
    virtual void Draw(sint32, sint32, bool)
    {
        FUNCTION_MARKER(0x80032200, "FRONT.BIN");
    }

    virtual bool TransitionIsFinished(void)
    {
        FUNCTION_MARKER(0x800321F8, "FRONT.BIN");
        return true;
    }

};

class tMenuItemOptionsLeftRightChoice : public tMenuItemLeftRightChoice
{
public:
    tMenuItemOptionsLeftRightChoice(uint32 textDescription, tListIterator *dataPtr);
    virtual void Draw(sint32 x, sint32 y, bool selected);
};

class tMenuItemOptionsTwoItemChoice : public tMenuItemLeftRightChoice
{
public:
    tMenuItemOptionsTwoItemChoice(uint32 textDescription, tListIterator *dataPtr);
    virtual void TransitionOn(void);
    virtual void Draw(sint32 x, sint32 y, bool selected);
    sint16 fOnOffFade;
};

class tMenuNFS4 : public tMenu
{
public:
    tMenuNFS4(uint32 flags, tScreen *screenHandler, tMenu *nextMenu, tMenu *optionsMenu, void (*OnButtonPress)(tMenuCommand &), sint16 title, tMenuItem *firstItem, ...);
    tMenuNFS4(uint32 flags, tScreen *screenHandler, tMenu *nextMenu, tMenu *optionsMenu, void (*OnButtonPress)(tMenuCommand &), sint16 title);

    sint32 fInItemTransition;
    sint32 fInMenuTransition;
    sint16 fTransitionVal;
    sint8 fTransitionDirection;
    char fLastItem;
    char fNumItems;

    virtual ~tMenuNFS4(void);
    virtual void Initialize(void);
    virtual void TransitionOff(void);
    virtual void TransitionOn(void);
    virtual bool TransitionIsFinished(void);
    virtual void UpdateTransition(void);
    virtual void DrawItem(sint32 item);
    virtual void Draw(void);
};

class tMenuNFS4TwoPlayer : public tMenuNFS4
{
public:
    tMenuNFS4TwoPlayer(uint32 flags, tScreen *screenHandler, tMenu *nextMenu, tMenu *optionsMenu, void (*OnButtonPress)(tMenuCommand &), sint16 title, tMenuItem *firstItem, ...);

    virtual ~tMenuNFS4TwoPlayer(void);
    virtual void DrawItem(sint32 item);
};

class tMenuNFS4Bottom : public tMenuNFS4
{
public:
    tMenuNFS4Bottom(uint32 flags, tScreen *screenHandler, tMenu *nextMenu, tMenu *optionsMenu, void (*OnButtonPress)(tMenuCommand &), sint16 title, tMenuItem *firstItem, ...);
    virtual ~tMenuNFS4Bottom(void);
    virtual void Draw(void);
};

class tMenuBlank : public tMenuNFS4
{
public:
    tMenuBlank(uint32 flags, tScreen *screenHandler, tMenu *nextMenu, tMenu *optionsMenu, void (*OnButtonPress)(tMenuCommand &), sint16 title);
    virtual ~tMenuBlank(void);
    virtual void Draw(void);
    virtual void ProcessInput(tPlayer fromPlayer, tInputKeyType &keyval, tMenuCommand &command);
    virtual void Initialize(void);
    virtual sint32 DebounceKeys(void);
    virtual void TransitionOff(void);
    virtual void TransitionOn(void);
    virtual bool TransitionIsFinished(void);

    virtual void UpdateTransition(void)
    {
        FUNCTION_MARKER(0x8001BEF8, "FRONT.BIN");
    }
};

class tMenuOptions : public tMenuNFS4
{
public:
    tMenuOptions(uint32 flags, tScreen *screenHandler, tMenu *nextMenu, tMenu *optionsMenu, void (*OnButtonPress)(tMenuCommand &), sint16 title, sint16 player, tMenuItem *firstItem, ...);

    uint32 fMenuEnterTicks;
    sint16 fPlayer;

    virtual ~tMenuOptions(void);
    virtual void Draw(void);
    virtual void TransitionOff(void);
    virtual void TransitionOn(void);
    virtual bool TransitionIsFinished(void);
    virtual void ProcessInput(tPlayer fromPlayer, tInputKeyType &keyval, tMenuCommand &command);
    virtual bool IsSubMenu(void);
};

class MenuNFS4
{
public:
    static void SetHelpPos(PSX_RECT &r);
    static void DrawTextBox(sint32 helpText, PSX_RECT &r, sint32 initialWidth, sint16 drawOffset, sint16 fSelFade, bool drawArrows, bool reflected);
};

#endif
