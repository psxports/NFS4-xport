#ifndef TCARINFO_H
#define TCARINFO_H

#include "xport.h"

struct tCarInfo
{
    char fCarID;
    uint8 fSimNumber, fCarClass, fABSAvailable, fDefaultColor, fCopClass, fDefaultTires, fAvailable;
    char fShapeName[8], fSmallName[8], fQTVRName[8];
    sint32 fPrices[4];
    uint8 fStats[4][5];
    uint32 fColorList[16];
    uint8 fSpeechColors[16];
    uint32 fHudColor[2];
    char fDayTachFile[8], fNightTachFile[8];
    char fNumLightColors, fNumDarkColors;
    uint8 fCountries;
    char fColorOrder[16];
    uint8 fPursuitAvailable, fEnginePatch;
    char fSpeechCarID;
    uint8 fTractionAvailable, fExoticCar, fUpgrades, fColor, fViewable, fCountry, fCarIndex;
    uint8 fReserved[3];
};

#endif
