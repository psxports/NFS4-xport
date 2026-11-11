#ifndef STATTOOL_H
#define STATTOOL_H
#include "xport.h"
#include "frontend/common/personalities.h"
#include "game/common/car.h"

struct tRecordBuffer;

class Stattool
{
public:
    static void UpperCaseItKeepingInMindThoseBloodySpecialCharacters(char *string);
    static void nCreateIndex(sint32 nNumber, sint32 *nInput, sint16 *nIndex);
    static void ParseTime(sint32 nTime, char *sLapTime);
    static void SamNelsonsUpperLowerStringConverterForRecords(char *string);
    static void GetAllDefaultRecords(tRecordBuffer *TrackRecords, bool cheatones);
    static void ReadDefaultRecords(tRecordBuffer *Records, bool cheatones);
    static void GetRecords(sint16 nShowTrack, tRecordBuffer *TrackRecords);
    static sint32 ReturnRecordLapTime(sint16 nTrack);
    static sint16 CheckForHumanCar(Car::tStats *dummyCars);
    static char * GetAINameFromPersonality(tPersonalities personality);
};

#endif
