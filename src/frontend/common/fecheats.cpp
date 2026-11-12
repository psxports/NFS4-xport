#include <stddef.h>
#include "xport.h"
#include "frontend/common/feapp.h"
#include "frontend/common/fecars.h"
#include "frontend/common/fecheats.h"
#include "frontend/common/fetourn.h"
#include "frontend/common/fetracks.h"
#include "frontend/common/stattool.h"
#include "game/common/audiocmn.h"
#include "game/common/nfs3.h"
#include "game/common/stats.h"
#include "game/common/textsys.h"

struct tCheat
{
    uint8 name[8];
    tCheatCode cheat;
};

static tCheat cheatList[10] =
{
    { { 0x59, 0x68, 0x06, 0x06, 0x60, 0x7E, 0x7F, 0x00 }, cheat_AllCars },
    { { 0x59, 0x58, 0x16, 0x16, 0x60, 0x7E, 0x7F, 0x00 }, cheat_AllCops },
    { { 0x61, 0x70, 0x0E, 0x26, 0x58, 0x7E, 0x7F, 0x00 }, cheat_AllTracks },
    { { 0x1B, 0x06, 0x0F, 0x17, 0x10, 0x1E, 0x1F, 0x00 }, cheat_MoreMoney },
    { { 0x2D, 0x41, 0x34, 0x02, 0x50, 0x7E, 0x7F, 0x00 }, cheat_AllCheats },
    { { 0x0E, 0x05, 0x01, 0x02, 0x05, 0x0E, 0x0F, 0x00 }, cheat_LotsaMoney },
    { { 0x24, 0x1F, 0x2C, 0x1C, 0x12, 0x3E, 0x3F, 0x00 }, cheat_AllBronze },
    { { 0x13, 0x29, 0x1C, 0x06, 0x29, 0x3E, 0x3F, 0x00 }, cheat_AllSilver },
    { { 0x03, 0x03, 0x0F, 0x06, 0x00, 0x0E, 0x0F, 0x00 }, cheat_AllGold },
    { { 0x1E, 0x1F, 0x3F, 0x1B, 0x00, 0x3E, 0x3F, 0x00 }, cheat_AllNotRaced },
};

static tCheat bonusList[3] =
{
    { { 0x9D, 0x99, 0x88, 0x86, 0x93, 0xAF, 0x92, 0xFF }, cheat_NFSTeamRecords },
    { { 0x87, 0xDF, 0xDF, 0xAB, 0x9D, 0x8F, 0xD8, 0xFF }, cheat_MyMomSaysImCool },
    { { 0xF6, 0x9C, 0x8C, 0x9F, 0xFF, 0xFF, 0xBF, 0xFF }, cheat_BonusMoney },
};

void FECheat::HandleActivation(tCheatCode cheat)
{
    FUNCTION_MARKER(0x80023240, "FRONT.BIN");
    if ((uint32)cheat >= cheat_NumCheats)
        return;
    switch (cheat)
    {
    case cheat_Roadster:
    case cheat_Pony:
    case cheat_Saloon:
    case cheat_Sports:
    case cheat_SuperCar:
    case cheat_GTRCar:
    case cheat_BonusCar:
    case cheat_CopCar:
        carManager.SetClassAvailable((tCarClassType)cheat, true);
        break;
    case cheat_AllCars:
        for (sint16 i = 0; i < 7; ++i)
            HandleActivation((tCheatCode)i);
        break;
    case cheat_AllTracks:
        trackManager.SetClassAvailable(tct_Beginner, true);
        trackManager.SetClassAvailable(tct_Intermediate, true);
        trackManager.SetClassAvailable(tct_Expert, true);
        trackManager.SetClassAvailable(tct_GTR, true);
        trackManager.SetClassAvailable(tct_Bonus, true);
        break;
    case cheat_MoreMoney:
    case cheat_BonusMoney:
        tournamentManager.fMoney = (tournamentManager.fMoney + 50000);
        break;
    case cheat_LotsaMoney:
        tournamentManager.fMoney = (tournamentManager.fMoney + 100000000);
        break;
    case cheat_NFSTeamRecords:
        Stattool::GetAllDefaultRecords(Stats::gTrackRecords, true);
        break;
    case cheat_AllCheats:
        gFECheats = 0xFFFFFFFF;
        for (sint16 i = 0; i < cheat_NumCheats; ++i)
        {
            if (i != cheat_AllCheats)
                HandleActivation((tCheatCode)i);
        }
        break;
    case cheat_AllNotRaced:
    case cheat_AllGold:
    case cheat_AllSilver:
    case cheat_AllBronze:
        for (sint16 num = 0; num < 64; ++num)
            tournamentManager.fBestPlacement[num] = cheat - cheat_AllNotRaced;
        break;
    default:
        break;
    }
}

void FECheat::EncodeString(char *input, char *output)
{
    FUNCTION_MARKER(0x80023410, "FRONT.BIN");
    char buffer[8];
    for (sint32 i = 0; i < 8; ++i)
    {
        buffer[i] = input[i];
        output[i] = 0;
    }
    for (sint32 i = 0; i < 8; ++i)
    {
        for (sint32 j = 0; j < 8; ++j)
        {
            output[i] |= (buffer[j] & 1) << j;
            buffer[j] >>= 1;
        }
    }
}

void FECheat::EncodeString2(char *input, char *output)
{
    FUNCTION_MARKER(0x8002349C, "FRONT.BIN");
    char buffer[8];
    for (sint32 i = 0; i < 8; ++i)
    {
        buffer[i] = (char)~input[i];
        output[i] = (char)~input[i];
    }
    for (sint32 i = 0; i < 8; ++i)
    {
        output[0] ^= ((output[0] >> i) & 1) << i;
        output[i] ^= ((output[i] >> i) & 1) << i;
        output[0] |= ((buffer[i] >> i) & 1) << i;
        output[i] |= ((buffer[0] >> i) & 1) << i;
    }
    for (sint32 i = 1; i < 8; ++i)
    {
        output[i] &= 0xFE;
        output[i] ^= ((output[i] >> (i - 1)) & 1) << (i - 1);
        output[i] |= (buffer[i] >> (i - 1)) & 1;
        output[i] |= (buffer[i] & 1) << (i - 1);
    }
}

void FECheat::ActivateBonus(tCheatCode cheat)
{
    FUNCTION_MARKER(0x800235D8, "FRONT.BIN");
    HandleActivation(cheat);
    gFEBonus |= 1U << ((uint32)cheat & 31);
}

bool FECheat::ActivateCheat(char *cheat)
{
    FUNCTION_MARKER(0x80023614, "FRONT.BIN");
    char buffer[8];
    EncodeString(cheat, buffer);
    for (sint32 i = 0; i < 10; ++i)
    {
        sint32 j = 0;
        while (j < 8 && cheatList[i].name[j] == buffer[j])
            ++j;
        if (j == 8)
        {
            AudioCmn::PlayFESFX(26);
            tFEApplication *application = FEApp;
            char *string = TextSys::Word(634);
            tFEApplication *displayApplication = FEApp;
            application->MemCardDialog.string = string;
            displayApplication->MemCardDialog.Display();
            HandleActivation(cheatList[i].cheat);
            gFECheats |= 1U << ((uint32)cheatList[i].cheat & 31);
            return true;
        }
    }
    return false;
}

bool FECheat::IsCheatEnabled(tCheatCode cheat)
{
    FUNCTION_MARKER(0x8002371C, "FRONT.BIN");
    uint32 mask = 1U << ((uint32)cheat & 31);
    return ((gFECheats | gFEBonus) & mask) != 0;
}

void FECheat::SaveBonus(uint32 &cheat)
{
    FUNCTION_MARKER(0x80023748, "FRONT.BIN");
    cheat = gFEBonus;
}

void FECheat::LoadBonus(uint32 &cheat)
{
    FUNCTION_MARKER(0x80023758, "FRONT.BIN");
    uint32 value = cheat;
    gFECheats = 0;
    gFEBonus = value;
    for (sint32 i = 0; i < cheat_NumCheats; ++i)
    {
        if (gFEBonus & (1 << i))
            HandleActivation((tCheatCode)i);
    }
}

bool FECheat::IsTheUserACryBabyCheater(void)
{
    FUNCTION_MARKER(0x800237D0, "FRONT.BIN");
    return gFECheats != 0;
}

bool FECheat::ActivateBonusByCode(char *code)
{
    FUNCTION_MARKER(0x800237E0, "FRONT.BIN");
    char buffer[8];
    EncodeString2(code, buffer);
    for (sint32 i = 0; i < 3; ++i)
    {
        sint32 j = 0;
        while (j < 8 && bonusList[i].name[j] == buffer[j])
            ++j;
        if (j == 8)
        {
            AudioCmn::PlayFESFX(26);
            ActivateBonus(bonusList[i].cheat);
            return true;
        }
    }
    return false;
}
