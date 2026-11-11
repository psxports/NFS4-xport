#ifndef FEAPP_H
#define FEAPP_H

#include "xport.h"
#include "psx.h"
#include "frontend/common/fedialog.h"
#include "frontend/common/femenu.h"

enum tAppCommand

{
    kApp_Command_StartRace = 0,

    kApp_Command_ReStartRace = 1,

    kApp_Command_StartReplay = 2
};

class tFEApplication

{
public:
    tFEApplication(void);

    ~tFEApplication(void);

    void PerformMenuInitialization(void);

    void PerformMenuDestruction(void);

    void DrawHelpIcons(void);

    void Redraw(void);

    void UpdateMusic(void);

    void SetMenu(sint16 i, tMenu *menu);

    void SetScreen(sint16 i, tScreen *screen);

    void DisplayHelp(sint16 variant);

    void RunDemoVideo(void);

    tAppCommand MainLoop(tMenu *newMenu);

    tAppCommand RunPostGame(void);

    tAppCommand RunFrontEnd(void);

    uint32 fCurrentMusic;

    tMenu *fCurrentMenu[2];

    tScreen *fCurrentScreen[2];

    tMenu *fTransitionToMenu[2];

    tScreen *fTransitionToScreen[2];

    tMenu *fParentMenu[2];

    tDialogMessageString messagePopup;

    tMenu *backList[2][16];

    sint32 backDepth[2];

    tInputKeyType fLastKeyPressed[2];

    sint16 fYOffset;

    tDialogHelp helpPopup;

    char fPlayer;

    char fInputPlayer;

    sint32 waitingForOtherPlayer[2];

    tDialogMessageStringWithTimeout MemCardDialog;

    sint32 gotName[2];

    sint32 needName[2];

    sint32 speechToPlay[2];
};

extern tFEApplication *FEApp;
extern uint32 gLargestUnused;

#endif
