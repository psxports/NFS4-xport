#include "xport.h"
#include "game/common/aitune.h"
#include "game/common/bworldsm.h"
#include "game/common/gmesetup.h"

copTuning_t copTuningInfo[6] =
{
    {65536, 78643, 6407964, 6407964},
    {65536, 78643, 6407964, 6407964},
    {65536, 78643, 6407964, 6407964},
    {65536, 78643, 6407964, 6407964},
    {65536, 78643, 6407964, 6407964},
    {65536, 78643, 6407964, 6407964},
};

sint32 AITune::oneWay;
sint32 AITune::driveSide;
sint32 AITune::gRoughLapTime;

sint32 AITune::MaxTraffic[2][4] =
{
    { 5, 5, 5, 5 },
    { 1, 1, 1, 1 }
};

sint32 AITune::LifeTimer[12] = { 25, 50, 75, 100, 200, 400, 500, 800, 900, 1000, 1100, 1200 };
sint32 AITune::BTCPerpAccMults[3] = { 49152, 55705, 65536 };

AITune::BTC_t AITune::BTC[3] =
{
    {98304, 55705, 65536, 5760, 64, 1920, 5760},
    {65536, 65536, 65536, 3840, 2880, 5760, 8320},
    {52428, 72089, 65536, 2880, 3840, 7680, 9600},
};

AITune::tTrackInfo AITune::trackInfo[12] =
{
    {0, 1, 170},
    {0, 1, 130},
    {0, 1, 135},
    {0, 1, 120},
    {0, 1, 118},
    {0, -1, 130},
    {0, 1, 145},
    {0, -1, 110},
    {0, 1, 121},
    {0, 1, 140},
    {0, 1, 145},
    {0, 1, 90},
};

accelscale_t AITune::accelerationScale[22] =
{
    {65536},
    {65536},
    {65536},
    {65536},
    {65536},
    {65536},
    {65536},
    {65536},
    {65536},
    {65536},
    {65536},
    {65536},
    {65536},
    {65536},
    {65536},
    {65536},
    {65536},
    {65536},
    {65536},
    {65536},
    {65536},
    {65536},
};

sint32 AITune::GetOneWay(void)
{
    FUNCTION_MARKER(0x80072E68, "NFS4.EXE");
    if (GameSetup::gData.track >= 12)
        return (BWorldSm::slices[1].laneCount >> 4) == 0;
    return trackInfo[GameSetup::gData.track].oneWay;
}

sint32 AITune::GetDriveSide(void)
{
    FUNCTION_MARKER(0x80072EC0, "NFS4.EXE");
    if (GameSetup::gData.track >= 12)
        return 1;
    return trackInfo[GameSetup::gData.track].driveSide;
}

sint32 AITune::GetRoughLapTimes(void)
{
    FUNCTION_MARKER(0x80072F00, "NFS4.EXE");
    if (GameSetup::gData.track >= 12)
        return 190;
    return trackInfo[GameSetup::gData.track].roughLapTimeSeconds;
}

void AITune::StartUp1(void)
{
    FUNCTION_MARKER(0x80072F40, "NFS4.EXE");
    oneWay = GetOneWay();
    driveSide = GetDriveSide();
    gRoughLapTime = GetRoughLapTimes();
}

void AITune::StartUp2(void)
{
    FUNCTION_MARKER(0x80072F78, "NFS4.EXE");
}

void AITune::CleanUp1(void)
{
    FUNCTION_MARKER(0x80072F80, "NFS4.EXE");
}

void AITune::CleanUp2(void)
{
    FUNCTION_MARKER(0x80072F88, "NFS4.EXE");
}
