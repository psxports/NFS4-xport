#include <stddef.h>
#include <string.h>
#include "xport.h"
#include "eaclib/psx/eaclib.h"
#include "frontend/common/fecars.h"
#include "frontend/common/front.h"
#include "frontend/common/statchk.h"
#include "frontend/common/stattool.h"
#include "game/common/car.h"
#include "game/common/gmesetup.h"
#include "game/common/nfs3.h"
#include "game/common/stats.h"
#include "lib/psx/blkmov.h"

struct tRecordBuffer
{
    char sName[8];
    sint32 nCar;
    sint32 nTime;
    sint32 nBestLap;
};

sint32 NewBestLap;
sint32 NewRecords[8];

bool StatChk::IsRecordLapTime(Car::tStats *dummyCars, sint16 nNumCars, sint16 *nBestCarIndex)
{
    FUNCTION_MARKER(0x80049840, "FRONT.BIN");
    sint16 bBestLapFlag = 0;
    sint16 bCheckLapRecords = Stattool::CheckForHumanCar(dummyCars);
    if (bCheckLapRecords != 1)
        return false;
    sint32 *nBestLapTimes = static_cast<sint32 *>(reservememadr("ranklap", nNumCars * 4, 16));
    sint16 *nRankBestLapTimes = static_cast<sint16 *>(reservememadr("rankbst", nNumCars * 2, 16));
    tRecordBuffer *TrackRecords = static_cast<tRecordBuffer *>(reservememadr("trkrcrds", 360, 16));
    for (sint16 i = 0; i < nNumCars; ++i) {
        tCarInfo *carInfo = carManager.GetCarFromSimID((sint16)dummyCars[i].carType);
        if ((dummyCars[i].carFlags & 0x200) != 0) {
            purgememadr(nBestLapTimes);
            purgememadr(nRankBestLapTimes);
            purgememadr(TrackRecords);
            return false;
        }
        if ((uint32)carInfo->fCarClass - 7U < 2)
            nBestLapTimes[i] = (sint32)((uint32)(i + 1) * 144000U);
        else
            nBestLapTimes[i] = dummyCars[i].finalBestLap;
    }
    if (nNumCars < 2)
        nRankBestLapTimes[0] = 0;
    else
        Stattool::nCreateIndex(nNumCars, nBestLapTimes, nRankBestLapTimes);
    sint16 i = 0;
    for (; i < nNumCars; ++i) {
        sint16 nBestCarIndexTemp = nRankBestLapTimes[i];

        if ((dummyCars[nBestCarIndexTemp].carFlags & 4) != 0 && dummyCars[nRankBestLapTimes[nBestCarIndexTemp]].finalBestLap > 0) {
            bCheckLapRecords = 1;
            break;
        }
        bCheckLapRecords = 0;
    }
    if (bCheckLapRecords != 0) {
        sint16 nNewRecordHolder = nRankBestLapTimes[i];
        sint16 track = Front::GetTrackRaced();
        Stattool::GetRecords(track, TrackRecords);
        tRecordBuffer RecordHolder;
        memcpy(&RecordHolder, TrackRecords, 20);
        sint32 lap = dummyCars[nNewRecordHolder].finalBestLap;
        if ((lap < RecordHolder.nBestLap || RecordHolder.nBestLap == 0) && lap > 0)
            bBestLapFlag = 1;
        *nBestCarIndex = nNewRecordHolder;
    }
    purgememadr(nBestLapTimes);
    purgememadr(nRankBestLapTimes);
    purgememadr(TrackRecords);
    return bBestLapFlag != 0;
}

void StatChk::SaveRecordLapTime(Car::tStats *dummyCars, sint16 nNumCars, sint16 nBestCarIndex)
{
    FUNCTION_MARKER(0x80049B7C, "FRONT.BIN");
    tCarInfo *carInfo = carManager.GetCarFromSimID((sint16)dummyCars[nBestCarIndex].carType);
    if ((uint32)carInfo->fCarClass - 7U < 2)
        return;
    tRecordBuffer *TrackRecords = static_cast<tRecordBuffer *>(reservememadr("trkrcrds", 360, 16));
    sint16 track = Front::GetTrackRaced();
    Stattool::GetRecords(track, TrackRecords);
    tRecordBuffer RecordHolder;
    memcpy(&RecordHolder, TrackRecords + 1, 20);
    tRecordBuffer DummyRaceResult;
    if (dummyCars[nBestCarIndex].finalNumArrests == 0 && dummyCars[nBestCarIndex].finalFinishType == 2)
        DummyRaceResult.nTime = dummyCars[nBestCarIndex].finalTotalTime;
    else
        DummyRaceResult.nTime = 0;
    DummyRaceResult.nBestLap = dummyCars[nBestCarIndex].finalBestLap;
    DummyRaceResult.nCar = (sint8)carInfo->fCarID;
    char *name = PlayerName(nBestCarIndex);
    strcpy(DummyRaceResult.sName, name);
    memcpy(TrackRecords, &DummyRaceResult, 20);
    track = Front::GetTrackRaced();
    blockmove(TrackRecords, Stats::gTrackRecords + track * 17, 340);
    NewBestLap = 1;
    purgememadr(TrackRecords);
}

sint16 StatChk::IsTopTime(Car::tStats *dummyCars, sint16 nNumCars)
{
    FUNCTION_MARKER(0x80049D0C, "FRONT.BIN");
    sint16 nLaps = (sint16)Front::GetLapsForType();
    sint32 LASTPLACE[2];
    sint32 TOPLIST[2];
    sint32 NUMBERONE[2];
    memset(LASTPLACE, 0, 8);
    memset(TOPLIST, 0, 8);
    memset(NUMBERONE, 0, 8);
    tRecordBuffer *RecordHolders = static_cast<tRecordBuffer *>(reservememadr("toprcrds", 360, 16));
    sint32 *nCarTotalTimes = static_cast<sint32 *>(reservememadr("carttime", nNumCars * 4, 16));
    sint16 *nRankCarTotalTimes = static_cast<sint16 *>(reservememadr("carttrnk", nNumCars * 2, 16));
    for (sint16 k = 0; k < nNumCars; ++k)
        nRankCarTotalTimes[dummyCars[k].position - 1] = k;
    sint16 track = Front::GetTrackRaced();
    Stattool::GetRecords(track, RecordHolders);
    sint16 nLapIndicator = nLaps == 2 ? 1 : 9;
    for (sint16 k = 0; k < nNumCars; ++k) {
        tCarInfo *carInfo = carManager.GetCarFromSimID((sint16)dummyCars[k].carType);
        sint16 nCar = nRankCarTotalTimes[k];
        sint32 flags = dummyCars[nCar].carFlags;
        if ((flags & 0x200) != 0) {
            purgememadr(RecordHolders);
            purgememadr(nCarTotalTimes);
            purgememadr(nRankCarTotalTimes);
            return 0;
        }
        if ((uint32)carInfo->fCarClass - 7U < 2)
            continue;
        if ((uint8)frontEnd.gameMode < 3 && (flags & 4) != 0 && dummyCars[nCar].finalFinishType == 2) {
            sint32 nCheckTotalTime = dummyCars[nCar].finalTotalTime;
            if (nCheckTotalTime < RecordHolders[nLapIndicator + 6].nTime || (RecordHolders[nLapIndicator + 7].nTime == 0 && nCheckTotalTime > 0)) {
                TOPLIST[nCar] = 1;
            } else if (nCheckTotalTime < RecordHolders[nLapIndicator + 7].nTime || (RecordHolders[nLapIndicator + 7].nTime == 0 && nCheckTotalTime > 0)) {
                TOPLIST[nCar] = 1;
                LASTPLACE[nCar] = 1;
            }
            if (nCheckTotalTime < RecordHolders[nLapIndicator].nTime || (RecordHolders[nLapIndicator + 7].nTime == 0 && nCheckTotalTime > 0)) {
                TOPLIST[nCar] = 1;
                NUMBERONE[nCar] = 1;
            }
        }
    }
    if (LASTPLACE[0] != 0) {
        if (LASTPLACE[1] != 0) {
            if (dummyCars[1].finalTotalTime < dummyCars[0].finalTotalTime)
                TOPLIST[0] = 0;
            else
                TOPLIST[1] = 0;
        } else if (TOPLIST[1] != 0) {
            TOPLIST[0] = 0;
        }
    } else if (LASTPLACE[1] != 0 && TOPLIST[0] != 0) {
        TOPLIST[1] = 0;
    }
    if (NUMBERONE[0] != 0 && NUMBERONE[1] != 0) {
        if (dummyCars[1].finalTotalTime < dummyCars[0].finalTotalTime)
            NUMBERONE[0] = 0;
        else
            NUMBERONE[1] = 0;
    }
    sint16 retvalue = TOPLIST[0] != 0;
    if (NUMBERONE[0] != 0)
        retvalue |= 2;
    if (TOPLIST[1] != 0)
        retvalue |= 4;
    if (NUMBERONE[1] != 0)
        retvalue |= 8;
    purgememadr(RecordHolders);
    purgememadr(nCarTotalTimes);
    purgememadr(nRankCarTotalTimes);
    return retvalue;
}

void StatChk::SaveTopTime(Car::tStats *dummyCars, sint16 nNumCars)
{
    FUNCTION_MARKER(0x8004A1B8, "FRONT.BIN");
    bool bTopTenFlag = false;
    sint16 nPlace = 0;
    sint32 nTopTenSort[8];
    sint16 nTopTenIndex[8];
    tRecordBuffer DummyRaceResult;
    sint32 topPlacements[2] = { 0, 0 };
    tRecordBuffer *RecordHolders = static_cast<tRecordBuffer *>(reservememadr("toprcrds", 360, 16));
    sint32 *nCarTotalTimes = static_cast<sint32 *>(reservememadr("carttime", nNumCars * 4, 16));
    sint16 *nRankCarTotalTimes = static_cast<sint16 *>(reservememadr("carttrnk", nNumCars * 2, 16));
    char *buffer = static_cast<char *>(reservememadr("records", 160, 16));
    for (sint16 k = 0; k < nNumCars; ++k)
        nRankCarTotalTimes[dummyCars[k].position - 1] = k;
    sint16 track = Front::GetTrackRaced();
    Stattool::GetRecords(track, RecordHolders);
    sint16 nLapIndicator = Front::GetLapsForType() == 2 ? 1 : 9;
    for (sint16 k = 0; k < nNumCars; ++k) {
        sint16 nCar = nRankCarTotalTimes[k];
        tCarInfo *carInfo = carManager.GetCarFromSimID((sint16)dummyCars[nCar].carType);
        if ((uint32)carInfo->fCarClass - 7U < 2)
            continue;
        bool bDoRecordCheck = ((uint8)frontEnd.gameMode < 3 || nCar == GameSetup::gData.localCar)
            && (dummyCars[nCar].carFlags & 4) != 0 && dummyCars[nCar].finalFinishType == 2;
        if (!bDoRecordCheck)
            continue;
        sint32 nCheckTotalTime = dummyCars[nCar].finalTotalTime;
        sint32 lastTime = RecordHolders[nLapIndicator + 7].nTime;
        if (!(nCheckTotalTime < lastTime || (lastTime == 0 && nCheckTotalTime > 0)))
            continue;
        DummyRaceResult.nTime = nCheckTotalTime;
        DummyRaceResult.nCar = (sint8)carInfo->fCarID;
        DummyRaceResult.nBestLap = dummyCars[nCar].finalBestLap;
        bTopTenFlag = true;

        memcpy(RecordHolders + nLapIndicator + 7, &DummyRaceResult, 20);
        for (sint16 j = nLapIndicator; j < nLapIndicator + 8; ++j) {
            nTopTenSort[j - nLapIndicator] = RecordHolders[j].nTime;
            if (nTopTenSort[j - nLapIndicator] == 0)
                nTopTenSort[j - nLapIndicator] = (j + 1) * 144000;
        }
        Stattool::nCreateIndex(8, nTopTenSort, nTopTenIndex);
        for (sint16 j = 0; j < 8; ++j) {
            if (nTopTenIndex[j] == 7)
                nPlace = j + 1;
        }
        for (sint16 j = 0; j < 8; ++j) {
            if (nTopTenIndex[j] == nRankCarTotalTimes[k]) {
                topPlacements[nTopTenIndex[j]] = nPlace;
                break;
            }
        }
        strcpy(DummyRaceResult.sName, PlayerName(nRankCarTotalTimes[k]));
        memcpy(RecordHolders + nLapIndicator + 7, &DummyRaceResult, 20);
        memcpy(buffer, RecordHolders + nLapIndicator, 160);
        for (sint16 j = 0; j < 8; ++j)
            memcpy(RecordHolders + nLapIndicator + j, buffer + nTopTenIndex[j] * 20, 20);
    }
    if (bTopTenFlag) {
        track = Front::GetTrackRaced();
        blockmove(RecordHolders, Stats::gTrackRecords + track * 17, 340);
    }
    if (topPlacements[0] == topPlacements[1])
        topPlacements[1] = topPlacements[0] + 1;
    else if (topPlacements[1] != 0 && topPlacements[1] < topPlacements[0])
        ++topPlacements[0];
    for (sint16 j = 0; j < 2; ++j) {
        if (topPlacements[j] != 0 && topPlacements[j] < 9)
            NewRecords[topPlacements[j] - 1] = 1;
    }
    purgememadr(buffer);
    purgememadr(RecordHolders);
    purgememadr(nCarTotalTimes);
    purgememadr(nRankCarTotalTimes);
}

void StatChk::ClearNewRecords(void)
{
    FUNCTION_MARKER(0x8004A838, "FRONT.BIN");
    for (sint32 i = 7; i >= 0; --i)
        NewRecords[i] = 0;
    NewBestLap = 0;
}
