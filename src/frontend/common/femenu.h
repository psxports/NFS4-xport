#ifndef FEMENU_H
#define FEMENU_H

#include <stddef.h>
#include "xport.h"
#include "frontend/common/fecars.h"
#include "frontend/common/fetourn.h"
#include "frontend/common/fetracks.h"

class tMenuItem;

typedef tMenuItem *tItemList[16];

class tScreen;

class tMenu;

enum tMenuCommandType
{
    kMenu_Command_None = 0,
    kMenu_Command_GoToMenu = 1,
    kMenu_Command_GoToMenuOneWay = 2,
    kMenu_Command_GoToMenuTwoPlayer = 3,
    kMenu_Command_BackupMenu = 4,
    kMenu_Command_StartRace = 5,
    kMenu_Command_Start2PlayerRace = 6,
    kMenu_Command_ReStartRace = 7,
    kMenu_Command_StartReplay = 8,
    kMenu_Command_ClearRecords = 9,
};

struct tMenuCommand
{
    tMenuCommandType type;
    tMenu *nextMenu;
};

enum tPlayer
{
    kPlayerBoth = -1,
    kPlayerOne = 0,
    kPlayerTwo = 1,
};

enum tInputKeyType
{
    kInput_KeyType_NoKey = 0,
    kInput_KeyType_AlreadyProcessed = 1,
    kInput_KeyType_Cross = 2,
    kInput_KeyType_Circle = 4,
    kInput_KeyType_Square = 8,
    kInput_KeyType_Triangle = 16,
    kInput_KeyType_L1 = 32,
    kInput_KeyType_L2 = 64,
    kInput_KeyType_R1 = 128,
    kInput_KeyType_R2 = 256,
    kInput_KeyType_Up = 512,
    kInput_KeyType_Down = 1024,
    kInput_KeyType_Left = 2048,
    kInput_KeyType_Right = 4096,
    kInput_KeyType_Start = 8192,
    kInput_KeyType_Select = 16384,
};

extern tPlayer gMenu_SubMenuPlayer;

class tListIterator
{
public:
    static void *operator new(size_t size) throw();
    static void operator delete(void *memory) throw();
    sint16 *fSelectionList;
    char *fValue;
    char fMinValue;
    char fMaxValue;
    virtual ~tListIterator(void);
    virtual char Value(tPlayer);
    virtual sint16 TextValue(tPlayer);
    virtual void Increment(tPlayer);
    virtual void Decrement(tPlayer);
    tListIterator(sint16 *selection, char *valPtr);
};

class tListIteratorIndexed : public tListIterator
{
public:
    char *fIndex;
    virtual ~tListIteratorIndexed(void);
    virtual char Value(tPlayer);
    virtual sint16 TextValue(tPlayer);
    virtual void Increment(tPlayer);
    virtual void Decrement(tPlayer);
    tListIteratorIndexed(sint16 *selection, char *valPtr, char *index);
};

class tListIteratorDoubleIndexed : public tListIterator
{
public:
    char *fIndex1;
    sint32 index1multiplier;
    char *fIndex2;
    virtual ~tListIteratorDoubleIndexed(void);
    virtual char Value(tPlayer);
    virtual sint16 TextValue(tPlayer);
    virtual void Increment(tPlayer);
    virtual void Decrement(tPlayer);
};

class tListIteratorMultiPlayer : public tListIterator
{
public:
    virtual ~tListIteratorMultiPlayer(void);
    virtual char Value(tPlayer atIndex);
    virtual sint16 TextValue(tPlayer atIndex);
    virtual void Increment(tPlayer atIndex);
    virtual void Decrement(tPlayer atIndex);
};

class tListIteratorRange : public tListIterator
{
public:
    virtual ~tListIteratorRange(void);
    virtual char Value(tPlayer);
    virtual sint16 TextValue(tPlayer);
    virtual void Increment(tPlayer);
    virtual void Decrement(tPlayer);
    tListIteratorRange(char minValue, char maxValue, char *valPtr);
};

class tListIteratorRangeIndexed : public tListIteratorRange
{
public:
    char *fIndex;
    virtual ~tListIteratorRangeIndexed(void);
    virtual char Value(tPlayer);
    virtual void Increment(tPlayer);
    virtual void Decrement(tPlayer);
    tListIteratorRangeIndexed(char minValue, char maxValue, char *valPtr, char *index);
};

class tMenu
{
public:
    static void *operator new(size_t size) throw();
    static void operator delete(void *memory) throw();
    tMenu(uint32 flags, tScreen *screenHandler, tMenu *nextMenu, tMenu *optionsMenu, void (*OnButtonPress)(tMenuCommand &), sint16 title);

    uint32 fFlags;
    sint16 fTitle;
    sint32 fCurrentItem;
    bool fNeverAnyEnabled;
    tMenuItem *fItemList[16];
    tScreen *fScreen;
    tMenu *fNextMenu;
    tMenu *fChildMenu;
    tMenu *fOptionsMenu;
    void (*fOnButtonPress)(tMenuCommand &);
    sint16 VertHelp;

    virtual ~tMenu(void);
    virtual void Initialize(void);
    virtual void ProcessInput(tPlayer fromPlayer, tInputKeyType &keyval, tMenuCommand &command);
    sint16 GetNumberEnabledItems(void);
    virtual sint32 DebounceKeys(void);
    virtual void TransitionOff(void);
    virtual void TransitionOn(void);
    virtual bool TransitionIsFinished(void);
    virtual bool IsSubMenu(void);
    virtual void Draw(void);
    virtual void UpdateTransition(void);
    void tMenuConstructor(tMenuItem *firstItem, void *ap);
};

class tMenuItem
{
public:
    static void *operator new(size_t size) throw();
    static void operator delete(void *memory) throw();
    uint32 fFlags;
    uint32 fTextDescription;
    sint16 fSelFade;
    sint32 fButtonImage;
    sint32 fNumFrames;
    tMenu *fNewMenu;

    tMenuItem(uint32 textDescription);
    virtual ~tMenuItem(void);
    virtual sint32 DebounceKeys(void);
    virtual void ProcessInput(tPlayer, tInputKeyType &, tMenuCommand &);

    virtual void Draw(bool) = 0;
    virtual void Draw(sint32, sint32, bool);
    virtual void Draw(sint32, sint32, sint32, bool);

    virtual void TransitionOff(void);

    virtual void TransitionOn(void);
    virtual bool TransitionIsFinished(void);
    virtual void UpdateTransition(bool selected);
    void UpdateSelFade(bool selected);
};

class tMenuItemInteractive : public tMenuItem
{
public:
    tMenuItemInteractive(uint32 textDescription);
    virtual ~tMenuItemInteractive(void);
};

class tMenuItemGoToMenuButton : public tMenuItemInteractive
{
public:
    void (*fOnButtonPress)(tMenuCommand &);

    tMenuItemGoToMenuButton(uint32 textDescription, tMenu *newMenu, void (*OnButtonPress)(tMenuCommand &));
    virtual ~tMenuItemGoToMenuButton(void);
    virtual void ProcessInput(tPlayer, tInputKeyType &, tMenuCommand &);
};

class tMenuItemLeftRightChoice : public tMenuItemInteractive
{
public:
    tListIterator *fData;

    tMenuItemLeftRightChoice(uint32 textDescription, tListIterator *dataPtr);
    virtual ~tMenuItemLeftRightChoice(void);
    virtual void ProcessInput(tPlayer, tInputKeyType &, tMenuCommand &);
    virtual void Draw(bool selected);
};

class tMenuItemLeftRightSlider : public tMenuItemInteractive
{
public:
    tMenuItemLeftRightSlider(uint32 textDescription, tListIterator *dataPtr);
    virtual ~tMenuItemLeftRightSlider(void);
    virtual sint32 DebounceKeys(void);
    virtual void ProcessInput(tPlayer fromPlayer, tInputKeyType &keyval, tMenuCommand &);
    virtual void Draw(bool selected);
    void SetDimensions(sint16 x, sint16 y, sint16 width, sint16 height);
    tListIterator *fData;
    sint16 fX, fY, fWidth, fHeight;
};

enum tCarNameLength
{
    cnl_Medium = 0,
    cnl_Short = 1,
    cnl_Long = 2,
};

class tListIteratorCar : public tListIterator
{
public:
    sint32 fCarListFilter;
    tCarManager *fCarManager;
    tCarNameLength fNameLength;
    virtual ~tListIteratorCar(void);
    virtual char Value(tPlayer atIndex);
    virtual sint16 TextValue(tPlayer atIndex);
    virtual void Increment(tPlayer atIndex);
    virtual void Decrement(tPlayer atIndex);
    tListIteratorCar(char *valPtr, tCarManager *carManager);
    void AdjustPosition(tPlayer atIndex, sint16 direction);
    bool ValidCar(tPlayer atIndex, char carNumber);
};

class tListIteratorCarColor : public tListIterator
{
public:
    char *fPlayer;
    char *fPlayerCar;
    sint32 fIndexSize;
    tCarManager *fCarManager;
    virtual ~tListIteratorCarColor(void);
    virtual char Value(tPlayer);
    virtual sint16 TextValue(tPlayer);
    virtual void Increment(tPlayer);
    virtual void Decrement(tPlayer);
    tListIteratorCarColor(char *value, char *player, char *playerCar, sint32 indexSize, tCarManager *carManager);
};

class tListIteratorTrack : public tListIteratorIndexed
{
public:
    tTrackManager *fTrackManager;
    virtual ~tListIteratorTrack(void);
    virtual sint16 TextValue(tPlayer atIndex);
    virtual void Increment(tPlayer atIndex);
    virtual void Decrement(tPlayer atIndex);
    tListIteratorTrack(char *valPtr, char *index, tTrackManager *trackManager);
    sint32 ValidTrack(char track);
};

class tListIteratorTournament : public tListIterator
{
public:
    tTournamentManager *fTournamentManager;
    virtual ~tListIteratorTournament(void);
    virtual char Value(tPlayer);
    virtual sint16 TextValue(tPlayer);
    virtual void Increment(tPlayer);
    virtual void Decrement(tPlayer);
    tListIteratorTournament(char *valPtr, tTournamentManager *tournManager);
    bool ValidTournament(char tourn);
};

void DrawSlider(sint16 value, sint16 min, sint16 max, sint16 fX, sint16 fY, sint16 fWidth, sint16 fHeight, sint16 rectwidth, sint16 rectspace, bool reverse, bool shadow, sint16 fSelFade, sint16 fFadeVal);

#endif
