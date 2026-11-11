#ifndef FEMEMCARD_H
#define FEMEMCARD_H

#include "xport.h"

enum PinkSlipsErrorCode
{
    PinkSlipsNoError = 0,
    PinkSlipsError_NotOriginalCard = 1,
    PinkSlipsError_LoadFailed = 2,
    PinkSlipsError_SaveFailed = 3,
    PinkSlipsError_CardNotFound = 4,
    PinkSlipsError_NotFormatted = 5,
    PinkSlipsError_CardFull = 6,
    numPinkSlipsErrors = 7
};

extern sint32 CURRENTLYUSINGMEMCARD;

bool SaveGame(sint16 player);
sint16 LoadGame(sint16 player, bool PinkSlips, bool WithDialogs);
PinkSlipsErrorCode SavePinkSlipsCars(sint16 player, sint16 withoutCarInGarageNumber);
PinkSlipsErrorCode SavePinkSlipsCarsWithErrorDialogs(sint16 player, sint16 WillLoseCar, sint16 withoutCarInGarageNumber);

#endif
