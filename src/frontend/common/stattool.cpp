#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include "xport.h"
#include "eaclib/psx/eaclib.h"
#include "frontend/common/fetools.h"
#include "frontend/common/stattool.h"
#include "game/common/car.h"
#include "game/common/gmesetup.h"
#include "game/common/nfs3.h"
#include "game/common/paths.h"
#include "game/common/stats.h"
#include "game/common/textsys.h"
#include "lib/psx/blkfill.h"
#include "lib/psx/blkmov.h"

struct tRecordBuffer
{
    char sName[8];
    sint32 nCar;
    sint32 nTime;
    sint32 nBestLap;
};

char minChar[6] = { 58, 58, 58, 58, 58, 58 };
char secChar[6] = { 46, 58, 46, 46, 46, 46 };

void UserNameUpperCaseOneLetter(char *c)
{
    FUNCTION_MARKER(0x8004AAB4, "FRONT.BIN");
    uint8 value = (uint8)*c;
    if (value >= 97 && value <= 122) {
        *c = value - 32;
        return;
    }
    switch ((uint8)*c) {
    case 0xE0: *c = 0xC0; return;
    case 0xE1: *c = 'A'; return;
    case 0xE4: *c = 0xC4; return;
    case 0xE5: *c = 0xC5; return;
    case 0xE8:
        *c = (uint8)frontEnd.language == 4 ? 0xC8 : 'E';

    case 0xEC: *c = 0xCC; return;
    case 0xE9: *c = 'E'; return;
    case 0xED: *c = 'I'; return;
    case 0xF1: *c = 0xD1; return;
    case 0xF2: *c = 0xD2; return;
    case 0xF3: *c = 'O'; return;
    case 0xF6: *c = 0xD6; return;
    case 0xF9: *c = 0xD9; return;
    case 0xFA: *c = 'U'; return;
    case 0xFC: *c = 0xDC; return;
    default: return;
    }
}

void Stattool::nCreateIndex(sint32 nNumber, sint32 *nInput, sint16 *nIndex)
{
    FUNCTION_MARKER(0x8004A868, "FRONT.BIN");
    sint32 *nTemp = static_cast<sint32 *>(reservememadr("TempSort", (sint32)(((uint32)nNumber + 1U) * 4U), 16));
    for (sint32 j = 0; j < nNumber; ++j) {
        nIndex[j] = (sint16)j;
        nTemp[j] = nInput[j];
    }
    for (sint32 j = 1; j < nNumber; ++j) {
        sint32 i = j - 1;
        sint32 nADummy = nTemp[j];
        sint32 nBDummy = nIndex[j];
        while (i >= 0 && nTemp[i] > nADummy) {
            nTemp[i + 1] = nTemp[i];
            nIndex[i + 1] = nIndex[i];
            --i;
        }
        nTemp[i + 1] = nADummy;
        nIndex[i + 1] = (sint16)nBDummy;
    }
    purgememadr(nTemp);
}

void Stattool::ParseTime(sint32 nTime, char *sLapTime)
{
    FUNCTION_MARKER(0x8004A99C, "FRONT.BIN");
    nTime = __fixsfsi(__divsf3(__floatsisf(nTime), 0.64f));
    sint16 minutes = (sint16)(nTime / 6000);
    nTime -= minutes * 6000;
    sint16 seconds = (sint16)(nTime / 100);
    sint16 hundredths = (sint16)(nTime - seconds * 100);
    sprintf(sLapTime, "%02d%c%02d%c%02d", minutes, minChar[(uint8)frontEnd.language], seconds, secChar[(uint8)frontEnd.language], hundredths);
}

void Stattool::SamNelsonsUpperLowerStringConverterForRecords(char *string)
{
    FUNCTION_MARKER(0x8004ABD0, "FRONT.BIN");
    s_lower(string);
    uint8 previousLetter = (uint8)*string;
    UserNameUpperCaseOneLetter(string);
    ++string;
    while ((uint8)*string != 0) {
        if (previousLetter == ' ')
            UserNameUpperCaseOneLetter(string);
        previousLetter = (uint8)*string;
        ++string;
    }
}

void Stattool::UpperCaseItKeepingInMindThoseBloodySpecialCharacters(char *string)
{
    FUNCTION_MARKER(0x8004AC4C, "FRONT.BIN");
    while ((uint8)*string != 0) {
        UserNameUpperCaseOneLetter(string);
        ++string;
    }
}

void Stattool::GetAllDefaultRecords(tRecordBuffer *TrackRecords, bool cheatones)
{
    FUNCTION_MARKER(0x8004AC98, "FRONT.BIN");
    tRecordBuffer *AllRecords = static_cast<tRecordBuffer *>(reservememadr("records", 3740, 16));
    ReadDefaultRecords(AllRecords, cheatones);
    for (sint32 n = 0; n < 11; ++n) {
        for (sint32 i = 0; i < 17; ++i) {
            sint32 s = n * 17 + i;
            strcpy(TrackRecords[s].sName, AllRecords[s].sName);
            SamNelsonsUpperLowerStringConverterForRecords(TrackRecords[s].sName);
            TrackRecords[s].nCar = AllRecords[s].nCar;
            TrackRecords[s].nTime = AllRecords[s].nTime;
            TrackRecords[s].nBestLap = AllRecords[s].nBestLap;
        }
    }
    purgememadr(AllRecords);
}

void Stattool::ReadDefaultRecords(tRecordBuffer *Records, bool cheatones)
{
    FUNCTION_MARKER(0x8004AD90, "FRONT.BIN");
    char filename[80];
    sprintf(filename, cheatones ? "%sznfsrec.dat" : "%szrecord.dat", Paths_Paths[36]);
    if (filesize(filename) == 3740)
        loadfileatadrz(filename, Records);
    else
        blockclear(Records, 3740);
}

void Stattool::GetRecords(sint16 nShowTrack, tRecordBuffer *TrackRecords)
{
    FUNCTION_MARKER(0x8004AE18, "FRONT.BIN");
    blockmove(Stats::gTrackRecords + nShowTrack * 17, TrackRecords, 340);
}

sint32 Stattool::ReturnRecordLapTime(sint16 nTrack)
{
    FUNCTION_MARKER(0x8004AE60, "FRONT.BIN");
    tRecordBuffer *TrackRecords = static_cast<tRecordBuffer *>(reservememadr("trkrcrds", 360, 16));
    GetRecords(nTrack, TrackRecords);
    tRecordBuffer RecordLapHolder;
    memcpy(&RecordLapHolder, TrackRecords, sizeof(RecordLapHolder));
    purgememadr(TrackRecords);
    return RecordLapHolder.nBestLap;
}

sint16 Stattool::CheckForHumanCar(Car::tStats *dummyCars)
{
    FUNCTION_MARKER(0x8004AEC4, "FRONT.BIN");
    sint16 count = (sint16)((uint16)GameSetup::gData.numPlayerRaceCars + (uint16)GameSetup::gData.numOpponentRaceCars);
    for (sint16 k = 0; k < count; ++k) {
        if ((dummyCars[k].carFlags & 4) != 0)
            return 1;
    }
    return 0;
}

char * Stattool::GetAINameFromPersonality(tPersonalities personality)
{
    FUNCTION_MARKER(0x8004AF4C, "FRONT.BIN");
    if ((uint32)personality < 15)
        return GameSetup::gPersonalityNames[personality];
    return TextSys::Word(750);
}
