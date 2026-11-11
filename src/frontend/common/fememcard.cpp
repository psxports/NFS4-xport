#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "xport.h"
#include "psx.h"
#include "eaclib/psx/pad.h"
#include "frontend/common/fecars.h"
#include "frontend/common/fecheats.h"
#include "frontend/common/fememcard.h"
#include "frontend/common/fetourn.h"
#include "frontend/common/fetracks.h"
#include "frontend/common/front.h"
#include "game/common/audiocmn.h"
#include "game/common/nfs3.h"
#include "game/common/stats.h"
#include "lib/psx/crc.h"

struct tSaveTrackInfo
{
    uint8 fTrackActivated[16];
};

struct tSaveTournament
{
    sint32 fSaveMoney;
    uint8 fSaveTier;
    uint8 fSaveTournament;
    uint8 fSaveTrack;
    uint8 fSaveNumRacers;
    tCompetitor fSaveCompetitors[6];
    uint8 fSaveBestPlacement[64];
    sint16 fSaveTierFinishPrize[4];
};

struct tRecordBuffer
{
    char sName[8];
    sint32 nCar;
    sint32 nTime;
    sint32 nBestLap;
};

typedef tRecordBuffer tSaveRecords[187];

struct tMemCardData
{
    uint32 fSize;
    uint32 bonus;
    tfrontEnd frontEnd;
    tSaveCarInfo carInfo;
    tSaveTrackInfo trackInfo;
    tSaveTournament tournament;
    tRecordBuffer records[187];
    uint32 pinkSlipsNoCheat;
    uint32 frontEndCRC;
    uint32 carInfoCRC;
    uint32 trackInfoCRC;
    uint32 tournamentCRC;
    uint32 recordsCRC;
};

sint32 CURRENTLYUSINGMEMCARD;

enum { NATIVE_SAVE_MAGIC = 0x3453464E, NATIVE_SAVE_VERSION = 1 };

struct NativeSaveFile
{
    uint32 magic;
    uint32 version;
    uint32 profileSize;
    uint32 present;
    tMemCardData profile[2];
};

sint32 CURRENTPLAYER;

static void ChecksumData(tMemCardData *memCardData)
{
    FUNCTION_MARKER(0x800342A0, "FRONT.BIN");
    memCardData->fSize = 0x14AC;
    memCardData->frontEndCRC = crc16(reinterpret_cast<const uint8 *>(&memCardData->frontEnd), 0x450);
    memCardData->carInfoCRC = crc16(reinterpret_cast<const uint8 *>(&memCardData->carInfo), 0xE0);
    memCardData->trackInfoCRC = crc16(reinterpret_cast<const uint8 *>(&memCardData->trackInfo), 0x10);
    memCardData->tournamentCRC = crc16(reinterpret_cast<const uint8 *>(&memCardData->tournament), 0xB0);
    memCardData->recordsCRC = crc16(reinterpret_cast<const uint8 *>(memCardData->records), 0xE9C);
}

static bool VerifySuccessfulRead(tMemCardData *memCardData)
{
    FUNCTION_MARKER(0x80034318, "FRONT.BIN");
    uint32 checksum = crc16(reinterpret_cast<const uint8 *>(&memCardData->frontEnd), 0x450);
    bool result = memCardData->frontEndCRC == checksum;
    checksum = crc16(reinterpret_cast<const uint8 *>(&memCardData->carInfo), 0xE0);
    if (memCardData->carInfoCRC != checksum)
        result = false;
    checksum = crc16(reinterpret_cast<const uint8 *>(&memCardData->trackInfo), 0x10);
    if (memCardData->trackInfoCRC != checksum)
        result = false;
    checksum = crc16(reinterpret_cast<const uint8 *>(&memCardData->tournament), 0xB0);
    if (memCardData->tournamentCRC != checksum)
        result = false;
    checksum = crc16(reinterpret_cast<const uint8 *>(memCardData->records), 0xE9C);
    if (memCardData->recordsCRC != checksum)
        result = false;
    if (memCardData->fSize != 0x14AC)
        result = false;
    return result;
}

static void InitializeNativeSave(NativeSaveFile *file)
{
    memset(file, 0, sizeof(*file));
    file->magic = NATIVE_SAVE_MAGIC;
    file->version = NATIVE_SAVE_VERSION;
    file->profileSize = sizeof(tMemCardData);
}

static sint32 ReadNativeSave(NativeSaveFile *file)
{
    sint32 bytes = xport_load(file, sizeof(*file));
    if (bytes == 0)
        return 0;
    if (bytes == (sint32)sizeof(tMemCardData))
    {
        tMemCardData legacy;
        memcpy(&legacy, file, sizeof(legacy));
        if (!VerifySuccessfulRead(&legacy))
            return -1;
        InitializeNativeSave(file);
        memcpy(&file->profile[0], &legacy, sizeof(legacy));
        memcpy(&file->profile[1], &legacy, sizeof(legacy));
        memcpy(file->profile[1].frontEnd.playerNameList[0], legacy.frontEnd.playerNameList[1], 8);
        ChecksumData(&file->profile[1]);
        file->present = 3;
        return 1;
    }
    if (bytes != (sint32)sizeof(*file) || file->magic != NATIVE_SAVE_MAGIC ||
        file->version != NATIVE_SAVE_VERSION || file->profileSize != sizeof(tMemCardData) ||
        file->present == 0 || (file->present & ~uint32(3)) != 0)
        return -1;
    for (sint32 player = 0; player < 2; ++player)
        if ((file->present & (1 << player)) != 0 && !VerifySuccessfulRead(&file->profile[player]))
            return -1;
    return 1;
}

bool SaveGame(sint16 player)
{
    FUNCTION_MARKER(0x80034988, "FRONT.BIN");
    if ((uint32)player >= 2)
        return false;
    CURRENTPLAYER = player;
    CURRENTLYUSINGMEMCARD = 1;
    NativeSaveFile file;
    sint32 readResult = ReadNativeSave(&file);
    if (readResult < 0)
    {
        CURRENTLYUSINGMEMCARD = 0;
        return false;
    }
    if (readResult == 0)
        InitializeNativeSave(&file);
    for (sint32 i = 0; i < 2; ++i)
        frontEnd.AnalogOn[i] = gPadinfo.buf[i * 4].ID == 0x73;
    tMemCardData memCardData = { 0 };
    memcpy(&memCardData.frontEnd, &frontEnd, 0x450);
    if (player != 0)
        memcpy(memCardData.frontEnd.playerNameList[0], frontEnd.playerNameList[player], 8);
    tournamentManager.SaveTournament(memCardData.tournament);
    trackManager.SaveTracks(memCardData.trackInfo);
    carManager.SaveCars(memCardData.carInfo);
    memcpy(memCardData.records, Stats::gTrackRecords, 0xE9C);
    memCardData.pinkSlipsNoCheat = randPSX();
    FECheat::SaveBonus(memCardData.bonus);
    ChecksumData(&memCardData);
    memcpy(&file.profile[player], &memCardData, sizeof(memCardData));
    file.present |= 1 << player;
    sint32 other = player ^ 1;
    if ((file.present & (1 << other)) == 0)
    {
        memcpy(&file.profile[other], &memCardData, sizeof(memCardData));
        memcpy(file.profile[other].frontEnd.playerNameList[0], memCardData.frontEnd.playerNameList[other], 8);
        ChecksumData(&file.profile[other]);
        file.present |= 1 << other;
    }
    bool result = xport_save(&file, sizeof(file)) == 1;
    CURRENTLYUSINGMEMCARD = 0;
    return result;
}

sint16 LoadGame(sint16 player, bool PinkSlips, bool)
{
    FUNCTION_MARKER(0x80034E18, "FRONT.BIN");
    if ((uint32)player >= 2)
        return 1;
    CURRENTPLAYER = player;
    CURRENTLYUSINGMEMCARD = 1;
    NativeSaveFile file;
    sint32 readResult = ReadNativeSave(&file);
    if (readResult <= 0)
    {
        CURRENTLYUSINGMEMCARD = 0;
        return readResult == 0 ? 3 : 1;
    }
    if ((file.present & (1 << player)) == 0)
    {
        CURRENTLYUSINGMEMCARD = 0;
        return 3;
    }
    tMemCardData &memCardData = file.profile[player];
    if (!PinkSlips)
    {
        if (player == 0)
        {
            memcpy(&frontEnd, &memCardData.frontEnd, 0x450);
            tournamentManager.LoadTournament(memCardData.tournament);
            trackManager.LoadTracks(memCardData.trackInfo);
            memcpy(Stats::gTrackRecords, memCardData.records, 0xE9C);
            FECheat::LoadBonus(memCardData.bonus);
        }
        else
            sprintf(frontEnd.playerNameList[player], memCardData.frontEnd.playerNameList[0]);
        carManager.LoadCars(memCardData.carInfo, player);
    }
    else
    {
        frontEnd.gPinkSlipsNoCheat[player] = memCardData.pinkSlipsNoCheat;
        carManager.LoadPinkSlipsCars(memCardData.carInfo, player);
        sprintf(frontEnd.playerNameList[player], memCardData.frontEnd.playerNameList[0]);
        frontEnd.pinkSlipsCash[player] = memCardData.tournament.fSaveMoney;
    }
    SetPads();
    Front::ResetPSXController(player, (uint8)frontEnd.controlConfig[player]);
    if (!PinkSlips)
    {
        if (frontEnd.raceType == 6)
            frontEnd.raceType = 0;
        gMasterSFXLevel = (uint8)frontEnd.sfxVolume;
    }
    CURRENTLYUSINGMEMCARD = 0;
    return 0;
}

PinkSlipsErrorCode SavePinkSlipsCars(sint16 player, sint16 withoutCarInGarageNumber)
{
    FUNCTION_MARKER(0x800353F0, "FRONT.BIN");
    if ((uint32)player >= 2)
        return PinkSlipsError_LoadFailed;
    CURRENTPLAYER = player;
    CURRENTLYUSINGMEMCARD = 1;
    NativeSaveFile file;
    if (ReadNativeSave(&file) != 1 || (file.present & (1 << player)) == 0)
    {
        CURRENTLYUSINGMEMCARD = 0;
        return PinkSlipsError_LoadFailed;
    }
    tMemCardData &memCardData = file.profile[player];
    if (frontEnd.gPinkSlipsNoCheat[player] != memCardData.pinkSlipsNoCheat)
    {
        CURRENTLYUSINGMEMCARD = 0;
        return PinkSlipsError_NotOriginalCard;
    }
    carManager.SavePinkSlipsCars(memCardData.carInfo, player, withoutCarInGarageNumber);
    ChecksumData(&memCardData);
    bool written = xport_save(&file, sizeof(file)) == 1;
    CURRENTLYUSINGMEMCARD = 0;
    return written ? PinkSlipsNoError : PinkSlipsError_SaveFailed;
}

PinkSlipsErrorCode SavePinkSlipsCarsWithErrorDialogs(sint16 player, sint16, sint16 withoutCarInGarageNumber)
{
    FUNCTION_MARKER(0x80035778, "FRONT.BIN");
    return SavePinkSlipsCars(player, withoutCarInGarageNumber);
}
