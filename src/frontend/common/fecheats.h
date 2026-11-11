#ifndef FECHEATS_H
#define FECHEATS_H

#include "xport.h"

enum tCheatCode
{
    cheat_Roadster = 0,
    cheat_Pony = 1,
    cheat_Saloon = 2,
    cheat_Sports = 3,
    cheat_SuperCar = 4,
    cheat_GTRCar = 5,
    cheat_BonusCar = 6,
    cheat_CopCar = 7,
    cheat_FiveFingerDiscount = 8,
    cheat_FreeRepairs = 9,
    cheat_NoDamage = 10,
    cheat_AllCars = 11,
    cheat_AllCops = 12,
    cheat_AllTracks = 13,
    cheat_MoreMoney = 14,
    cheat_BonusTrack1 = 15,
    cheat_BonusTrack2 = 16,
    cheat_BonusTrack3 = 17,
    cheat_BonusTrack4 = 18,
    cheat_AllCheats = 19,
    cheat_NFSTeamRecords = 20,
    cheat_MyMomSaysImCool = 21,
    cheat_BonusMoney = 22,
    cheat_LotsaMoney = 23,
    cheat_FullGridTraffic = 24,
    cheat_FinishedTournament = 25,
    cheat_FinishedSpecialEvent = 26,
    cheat_AllNotRaced = 27,
    cheat_AllGold = 28,
    cheat_AllSilver = 29,
    cheat_AllBronze = 30,
    cheat_NumCheats = 31,
};

class FECheat
{
public:
    static void HandleActivation(tCheatCode cheat);
    static void EncodeString(char *input, char *output);
    static void EncodeString2(char *input, char *output);
    static void ActivateBonus(tCheatCode cheat);
    static bool ActivateCheat(char *cheat);
    static bool IsCheatEnabled(tCheatCode cheat);
    static void SaveBonus(uint32 &cheat);
    static void LoadBonus(uint32 &cheat);
    static bool IsTheUserACryBabyCheater(void);
    static bool ActivateBonusByCode(char *code);
};

#endif
