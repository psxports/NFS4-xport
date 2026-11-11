#ifndef FEDIALOG_H
#define FEDIALOG_H

#include "xport.h"
#include "frontend/common/fescreen.h"

class tDialogBase : public tScreen
{
public:
    sint16 specificPlayer;
    sint16 left;
    sint16 top;
    sint16 width;
    sint16 height;
    sint16 reservedheight;
    sint32 currentlyOn;
    sint32 startTicks;
    sint32 timeOutTicks;
    sint16 OffsetX;
    sint16 OffsetY;
    sint16 MaxW;
    sint16 MaxH;
    sint32 fFullyOpen;
    sint16 fDefault;
    sint16 ReturnValue;
    sint32 fFadeText;

    virtual void ProcessInput(tPlayer, tInputKeyType &keyval, tMenuCommand &);

    virtual void CalculateDimensions(void) = 0;
    virtual void Draw(void);
    sint16 ShouldTimeOut(void);
    static void InitializeClass(void);
    static void DrawAllDialogs(void);
    static void HideAllDialogs(void);
    static tDialogBase *GetTopMostDialog(void);
    void Display(void);
    void Hide(void);
};

class tDialogHelp : public tDialogBase
{
public:
    sint16 variant;
    char *text[7];
    sint32 cont[7];
    sint16 numItems;
    sint16 helpcontrollers;
    sint16 lefttext;

    virtual void CalculateDimensions(void);
    virtual void Draw(void);
    void AddItem(sint16 textID, sint16 controllerID);
};

class tDialogMessageString : public tDialogBase
{
public:
    char *string;
    sint32 Centerit;

    virtual void CalculateDimensions(void);
    virtual void Draw(void);
};

class tDialogMessageStringWithTimeout : public tDialogMessageString
{
public:
};

class tDialogBackUpOnly : public tDialogMessageString
{
public:
    virtual void ProcessInput(tPlayer fromPlayer, tInputKeyType &keyval, tMenuCommand &);
};

class tDialogNoInputMessage : public tDialogMessageString
{
public:
    virtual void ProcessInput(tPlayer, tInputKeyType &, tMenuCommand &);
};

class tDialogInteractive : public tDialogMessageString
{
public:
    sint32 ReadyToReturnValue;
    sint32 fCurrentlyRunning;

    sint16 Run(void);
};

class tDialogYesNo : public tDialogInteractive
{
public:
    sint32 yesnowords[2];

    virtual void ProcessInput(tPlayer fromPlayer, tInputKeyType &keyVal, tMenuCommand &command);
    virtual void CalculateDimensions(void);
    virtual void Draw(void);
    tDialogYesNo(void);
};

class tDialogYesNoTri : public tDialogYesNo
{
public:
    virtual void ProcessInput(tPlayer fromPlayer, tInputKeyType &keyVal, tMenuCommand &command);
};

#endif
