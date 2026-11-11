#ifndef FECREDITS_H
#define FECREDITS_H

#include "xport.h"

struct tCredit;

class tCreditManager

{
public:
    void Setup(void);

    void Init(sint32);

    void DeInit(void);

    void RealDeInit(void);

    void Draw(bool selected);

    void SetupCurrCredit(void);

    void DrawCurrCredit(void);

    tCredit *CreditBuffer;

    sint32 fTVFade, fTextFade, fTextFadeDir;

    sint32 fCreditsInitialized, fRequestDeInit;

    sint32 fNumCredits, fShowCreditNum, fCurrCredit;

    sint32 StartedTransition, StartedLines, StartedTextFade;

    sint32 fLineTicks, fStartTicks;
};

extern tCreditManager CreditManager;

#endif
