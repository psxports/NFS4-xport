#ifndef PAUSEMENU_H
#define PAUSEMENU_H

#include <stddef.h>
#include "xport.h"
#include "frontend/common/femenu.h"

class tPMenuItem;

typedef tPMenuItem *tPItemList[16];

class tPMenu;

enum tPMenuCommandType
{
    kMPause_NoEvent = 0,
    kMPause_Continue = 1,
    kMPause_Restart = 2,
    kMPause_QuitToGameSetup = 3,
    kMPause_QuitToRaceSummary = 4,
    kMPause_ForfeitToRaceSummary = 5,
    kMPause_GoToMenu = 6,
    kMPause_BackupMenu = 7,
    kMPause_CommandConfirmationFlag = 256,
};

struct tPMenuCommand
{
    tPMenuCommandType type;
    tPMenu *nextMenu;
};

class tPMenuItem
{
public:
    static void *operator new(size_t size) throw();
    static void operator delete(void *memory) throw();
    uint32 fFlags;
    uint32 fTextDescription;
    virtual ~tPMenuItem(void);
    virtual tPMenu *NextMenu(void);
    virtual bool Debounce(void);
    virtual void ProcessInput(tInputKeyType &, tPMenuCommand &);

    bool IsDisabled(void)
    {
        FUNCTION_MARKER(0x800A86BC, "NFS4.EXE");
        return (fFlags & 1) != 0;
    }

    bool IsEnabled(void)
    {
        FUNCTION_MARKER(0x800A86A8, "NFS4.EXE");
        return (fFlags & 1) == 0;
    }

    tPMenuItem(uint32 textDescription);

    virtual bool IsNavigable(void) = 0;
    virtual void Draw(bool) = 0;
};

class tPMenu
{
public:
    static void *operator new(size_t size) throw();
    static void operator delete(void *memory) throw();
    sint32 fCurrentItem;
    bool fHighlight;
    tPMenuItem *fItemList[16];
    tPMenu *fNextMenu;
    sint32 fNumItems;
    virtual ~tPMenu(void);
    virtual void Initialize(void);
    virtual void ProcessInput(tInputKeyType &keyval, tPMenuCommand &command);
    virtual void Draw(void);
    void tPMenuConstructor(tPMenuItem *firstItem, void *ap);
    tPMenu(tPMenuItem *firstItem, ...);
    bool Debounce(void);
    void CheckForDisabled(void);
    sint32 NumEnabledItems(void);
    sint32 ItemEnabledNum(sint32 num);
};

class tPListIterator
{
public:
    static void *operator new(size_t size) throw();
    static void operator delete(void *memory) throw();
    sint16 *fSelectionList;
    sint32 *fValue;
    virtual ~tPListIterator(void);
    virtual char Value(tPlayer);
    virtual sint16 TextValue(tPlayer);
    virtual void Increment(tPlayer);
    virtual void Decrement(tPlayer);
    tPListIterator(sint16 *selection, sint32 *valPtr);
};

class tPListIteratorIndexed : public tPListIterator
{
public:
    char *fIndex;
    virtual ~tPListIteratorIndexed(void);
    virtual char Value(tPlayer);
    virtual sint16 TextValue(tPlayer);
    virtual void Increment(tPlayer);
    virtual void Decrement(tPlayer);
    tPListIteratorIndexed(sint16 *selection, sint32 *valPtr, char *index);
};

class tPMenuItemNonInteractiveText : public tPMenuItem
{
public:
    virtual ~tPMenuItemNonInteractiveText(void);
    virtual bool IsNavigable(void);
    virtual void Draw(bool);
    tPMenuItemNonInteractiveText(uint32 textDescription);
};

class tPMenuItemInteractive : public tPMenuItem
{
public:
    virtual ~tPMenuItemInteractive(void);
    virtual bool IsNavigable(void);
    virtual void Draw(bool selected);
    tPMenuItemInteractive(uint32 textDescription);
};

class tPMenuItemLeftRightChoice : public tPMenuItemInteractive
{
public:
    tPListIterator *fData;
    virtual ~tPMenuItemLeftRightChoice(void);
    virtual void ProcessInput(tInputKeyType &keyval, tPMenuCommand &);
    virtual void Draw(bool selected);
    tPMenuItemLeftRightChoice(uint32 textDescription, tPListIterator *dataPtr);
};

class tPMenuItemLeftRightSlider : public tPMenuItemInteractive
{
public:
    sint32 *fData;
    char fMaxVal;
    virtual ~tPMenuItemLeftRightSlider(void);
    virtual bool Debounce(void);
    virtual void ProcessInput(tInputKeyType &keyval, tPMenuCommand &);
    virtual void Draw(bool selected);
    tPMenuItemLeftRightSlider(uint32 textDescription, sint32 *dataPtr, char maxVal);
};

class tPMenuItemLeftRightSliderIndexed : public tPMenuItemLeftRightSlider
{
public:
    char *fIndex;
    virtual ~tPMenuItemLeftRightSliderIndexed(void);
    virtual void ProcessInput(tInputKeyType &keyval, tPMenuCommand &command);
    virtual void Draw(bool selected);
    tPMenuItemLeftRightSliderIndexed(uint32 textDescription, sint32 *dataPtr, char maxVal, char *index);
};

class tPMenuItemGoToMenuButton : public tPMenuItemInteractive
{
public:
    tPMenu *fNewMenu;
    void (*fOnButtonPress)(tPMenuCommand &);
    virtual ~tPMenuItemGoToMenuButton(void);
    virtual tPMenu *NextMenu(void);
    virtual void ProcessInput(tInputKeyType &keyval, tPMenuCommand &command);
    tPMenuItemGoToMenuButton(uint32 textDescription, tPMenu *newMenu, void (*OnButtonPress)(tPMenuCommand &));
};

class tPMenuItemCommandButton : public tPMenuItemInteractive
{
public:
    tPMenuCommandType fCommand;
    virtual ~tPMenuItemCommandButton(void);
    virtual void ProcessInput(tInputKeyType &keyval, tPMenuCommand &command);
    tPMenuItemCommandButton(uint32 textDescription, tPMenuCommandType command);
};

class PauseMenu
{
public:
    static void FullText(char *sMenuText, sint16 x, sint16 flags, sint16 color);
    static void MenuTextPositioned(sint16 index, sint16 selected, sint16 disabled, sint16 x);
    static void MenuText(sint16 index, bool selected, bool disabled);
};

extern sint32 ChangedEnabling;

#endif
