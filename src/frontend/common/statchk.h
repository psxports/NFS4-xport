#ifndef STATCHK_H
#define STATCHK_H
#include "xport.h"
#include "game/common/car.h"

class StatChk
{
public:
    static bool IsRecordLapTime(Car::tStats *dummyCars, sint16 nNumCars, sint16 *nBestCarIndex);
    static void SaveRecordLapTime(Car::tStats *dummyCars, sint16 nNumCars, sint16 nBestCarIndex);
    static sint16 IsTopTime(Car::tStats *dummyCars, sint16 nNumCars);
    static void SaveTopTime(Car::tStats *dummyCars, sint16 nNumCars);
    static void ClearNewRecords(void);
};

extern sint32 NewBestLap;
extern sint32 NewRecords[8];

#endif
