#ifndef SCREENPOST_H
#define SCREENPOST_H

#include "xport.h"
#include "frontend/common/fescreen.h"

void DrawMoney(sint32 x, sint32 y, sint32 numplaces, sint32 number, sint32 colfore, sint32 colback);

class tScreenTournamentStandings : public tScreen
{
public:
    tScreenTournamentStandings(void);

    virtual void Initialize(void);
    virtual void Cleanup(void);
    virtual void GetShapeInfo(sint16 &numPermShapes, sint16 &numSwapShapes, char **permFileName, char **swapFileName);
    virtual void ProcessInput(tPlayer, tInputKeyType &keyval, tMenuCommand &);
    virtual void DrawBackground(void);
    sint32 moneyFinal, moneyAwarded, moneyDamage, moneyBonus;
    sint32 starttick;
    sint32 gotmoney, gotbonus, gotbilled;
    sint32 fDrawMoney, fCountedDown, fStartCountdownNOW;
    sint32 fCountSpeed;
};

class tScreenTournamentStandings3item : public tScreenTournamentStandings
{
public:
    virtual void GetShapeInfo(sint16 &numPermShapes, sint16 &numSwapShapes, char **permFileName, char **swapFileName);
};

class tScreenPinkSlipStandings : public tScreenTournamentStandings3item
{
public:
    virtual void ProcessInput(tPlayer fromPlayer, tInputKeyType &keyval, tMenuCommand &command)
    {
        FUNCTION_MARKER(0x8003A86C, "FRONT.BIN");
        tScreen::ProcessInput(fromPlayer, keyval, command);
    }

    virtual void DrawBackground(void);
};

extern tScreenTournamentStandings *screenTournamentStandings;
extern tScreenTournamentStandings3item *screenTournamentStandings3item;
extern tScreenPinkSlipStandings *screenPinkSlipStandings;

#endif
