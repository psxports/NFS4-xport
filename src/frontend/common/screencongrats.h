#ifndef SCREENCONGRATS_H
#define SCREENCONGRATS_H

#include "xport.h"
#include "frontend/common/fescreen.h"
#include "frontend/common/tcarinfo.h"

enum tScreenCongratsMessage
{
    kScreenCongrats_Congrats = 0,
    kScreenCongrats_Eliminated = 1
};

enum tTrophyClass
{
    kTrophyGold = 0,
    kTrophySilver = 1,
    kTrophyBronze = 2,
    kTrophyCar = 3,
    kTrophyNone = 4
};

enum tSmallSpinningThing
{
    kSpinningNone = 0,
    kSpinningGold = 1,
    kSpinningMemCard = 2
};

class tScreenCongrats : public tScreen
{
public:
    virtual void Cleanup(void);
    virtual void GetShapeInfo(sint16 &numPermShapes, sint16 &numSwapShapes, char **permFileName, char **swapFileName);
    virtual void CalculatePrizes(void);
    virtual void DrawCongratsMessage(void) = 0;
    virtual bool GetCar(tCarInfo &carInfo);
    virtual void DrawBackground(void);
    virtual void DrawForeground(void);
    virtual void Initialize(void);
    virtual void ProcessInput(tPlayer p, tInputKeyType &keyval, tMenuCommand &c);
    tScreenCongratsMessage congratsMessage;
    tTrophyClass trophy;
    tSmallSpinningThing smallSpinningThing;
    sint32 fNumSpinShapes, fNumSmallSpinShapes, fCarPlayer;
    sint32 TotalCash, CashAwarded;
    sint32 framenum, starttick;
    bool InExtraSpin;
    sint32 InExtraSpinTick;
    tCarInfo fCarInfo;
    bool fGotCar;
    sint32 fEnterTick;
    sint32 fCountedDown;
    bool fStartCountdownNOW;
    sint32 fCountSpeed, fSpeechToPlay;
    sint16 fCarX, fCarY;
    float fCarCX, fCarCY;
};

class tScreenTournamentTrophy : public tScreenCongrats
{
public:
    virtual void ProcessInput(tPlayer p, tInputKeyType &keyval, tMenuCommand &c);
    virtual bool GetCar(tCarInfo &carInfo);
    virtual void DrawCongratsMessage(void);
    virtual void CalculatePrizes(void);
    sint16 fShapeCount;
    char fDoUpdate;
};

class tScreenPinkSlipCongrats : public tScreenCongrats
{
public:
    virtual void DrawCongratsMessage(void);
    virtual bool GetCar(tCarInfo &carInfo);
    virtual void CalculatePrizes(void);
    virtual void Initialize(void);
    virtual void Cleanup(void);
    sint16 fWinner;
};

class tScreenBeTheCopCongrats : public tScreenCongrats
{
public:
    virtual bool GetCar(tCarInfo &carInfo);
    virtual void CalculatePrizes(void);
    virtual void DrawCongratsMessage(void);
};

class tScreenTournamentCongrats : public tScreenCongrats
{
public:
    virtual bool GetCar(tCarInfo &carInfo);
    virtual void CalculatePrizes(void);
    virtual void DrawCongratsMessage(void);
};

extern tScreenPinkSlipCongrats *screenPinkSlipCongrats;
extern tScreenTournamentTrophy *screenTournamentTrophy;
extern tScreenBeTheCopCongrats *screenBeTheCopCongrats;
extern tScreenTournamentCongrats *screenTournamentCongrats;

#endif
