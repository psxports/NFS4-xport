#ifndef SCREENCONTROLLER_H
#define SCREENCONTROLLER_H

#include "xport.h"
#include "frontend/common/fedialog.h"
#include "game/psx/force.h"

class tScreenControllerConfig;

extern tScreenControllerConfig *screenControllerConfig;

void Controller_SetRamp(void);

class tScreenControllerConfig : public tScreen
{
public:
    Force::tGlobal fShaker;
    char fPrevConfig;
    char fTextConfig;
    char fTextController;
    char fPrevController;
    sint16 fFade[2];
    sint16 fFadeController[2];
    sint32 fStartTick;
    sint16 fGotTick;
    sint16 fAnim;
    sint16 fAnimFrame;
    sint16 fAnimStart;
    sint16 fAnimStop;
    sint16 fAnimStep;
    sint16 fAnimController;
    sint16 fSwap;
    sint16 fAnimFade;
    sint16 fAnimFadeStart;
    sint16 fAnimFadeStop;
    sint16 fAnimFadeFrame;
    sint16 fAnimFadeController;
    sint16 CurrentlyLoadedArt;
    sint16 negconChoice;
    sint32 fTransitionedIn;
    sint32 fTransitioningIn;
    sint32 fTransitioningOut;
    sint16 fArrowFade;
    sint16 fArrowFadeDir;
    sint16 fTextTypeOn;
    sint32 fFadeTextOut;
    sint16 mult;
    tDialogYesNo negconPopUp;
    sint32 fTimeOutStartTick;
    sint32 SuperFastFadeOut;
    sint32 fPlayedInSound;
    sint16 fShakingItem;
    sint32 fResetShakeTimeOut;
    char fCurrentController;
    sint32 player;
    virtual void GetShapeInfo(sint16 &numPermShapes, sint16 &numSwapShapes, char **permFileName, char **swapFileName);
    virtual void DrawBackground(void);
    virtual void DrawForeground(void);

    virtual ~tScreenControllerConfig(void)
    {
        FUNCTION_MARKER(0x8004590C, "FRONT.BIN");
    }

    virtual void Initialize(void);
    virtual void Cleanup(void);
    void ClearActuators(void);
    void SetActuators(sint32 max);
    void TurnOffShakers(void);
    void ShakeIt(void);
    sint16 AnimKeyPoints(bool forward, bool pt);
    void CheckConfigs(void);
    void SwapInController(void);
    void SetCurrentController(bool firsttime);
    sint32 CalcAnimFrame(sint32 frame);
    void ActualDrawController(sint32 frame, sint32 fadelevelmain, sint32 fadeleveltop, sint32 shakex, sint32 shakey);
    void DrawController(void);
    void HorzVertLine(sint16 *ArrowLoc, bool type);
    void DrawArrow(sint16 *ArrowLoc);
    tScreenControllerConfig(void);
    sint32 GetHelpText(void);
};

#endif
