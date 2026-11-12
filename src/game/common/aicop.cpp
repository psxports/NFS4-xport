#include <new>
#include <stddef.h>
#include <stdio.h>
#include "xport.h"
#include "eaclib/psx/eaclib.h"
#include "game/common/aicop.h"
#include "game/common/aitriger.h"
#include "game/common/aiworld.h"
#include "game/common/carobj.h"
#include "game/common/cars.h"
#include "game/common/gmesetup.h"
#include "game/common/paths.h"

copLevel_t twoLapCopGame[4] =
{
    {{2, 0}, 0, {0, 0}, 0, {0, 0}, 16, 19660, 512, 128, 2},
    {{0, 1}, 2, {2, 0}, 0, {0, 0}, 32, 26214, 0, 192, 0},
    {{2, 1}, 0, {0, 0}, 0, {1, 1}, 32, 19660, 0, 192, 0},
    {{0, 0}, 3, {2, 1}, 1, {1, 1}, 32, 26214, 0, 192, 0},
};

copLevel_t fourLapCopGame[5] =
{
    {{2, 0}, 0, {0, 0}, 0, {0, 0}, 16, 45875, 512, 128, 2},
    {{0, 0}, 2, {2, 0}, 0, {0, 0}, 16, 45875, 512, 192, 0},
    {{0, 1}, 0, {0, 0}, 0, {0, 0}, 32, 45875, 256, 192, 0},
    {{2, 1}, 0, {0, 0}, 0, {1, 1}, 32, 45875, 0, 224, 0},
    {{0, 0}, 3, {2, 1}, 1, {1, 1}, 32, 45875, 0, 256, 0},
};

copLevel_t twoLapCopGameSplit[4] =
{
    {{0, 1}, 0, {0, 0}, 0, {2, 2}, 16, 32768, 512, 128, 2},
    {{0, 0}, 1, {0, 1}, 0, {2, 2}, 32, 32768, 0, 192, 0},
    {{0, 1}, 0, {0, 0}, 0, {2, 2}, 32, 32768, 0, 192, 0},
    {{0, 0}, 1, {0, 1}, 1, {2, 2}, 32, 32768, 0, 192, 0},
};

copLevel_t fourLapCopGameSplit[4] =
{
    {{0, 1}, 0, {0, 0}, 0, {2, 2}, 16, 45875, 512, 128, 2},
    {{0, 0}, 1, {0, 1}, 0, {2, 2}, 16, 45875, 512, 192, 0},
    {{0, 1}, 0, {0, 0}, 0, {2, 2}, 32, 45875, 256, 192, 0},
    {{0, 0}, 1, {0, 1}, 1, {2, 2}, 32, 45875, 0, 256, 0},
};

copLevel_t twoLapCopGame1H1AI[4] =
{
    {{2, 0}, 0, {0, 0}, 0, {0, 0}, 16, 19660, 512, 128, 2},
    {{0, 0}, 2, {2, 0}, 0, {0, 0}, 32, 19660, 0, 192, 0},
    {{2, 0}, 0, {0, 0}, 0, {1, 1}, 32, 19660, 0, 192, 0},
    {{0, 0}, 2, {2, 0}, 1, {1, 1}, 32, 19660, 0, 192, 0},
};

copLevel_t fourLapCopGame1H1AI[5] =
{
    {{1, 0}, 0, {0, 0}, 0, {0, 0}, 16, 45875, 512, 128, 2},
    {{0, 0}, 2, {2, 0}, 0, {0, 0}, 16, 45875, 512, 192, 0},
    {{2, 0}, 0, {0, 0}, 0, {0, 0}, 32, 45875, 256, 192, 0},
    {{2, 0}, 0, {0, 0}, 0, {1, 1}, 32, 45875, 0, 224, 0},
    {{0, 0}, 2, {2, 0}, 1, {1, 1}, 32, 45875, 0, 256, 0},
};

copGame_t copGame[6] =
{
    {4, twoLapCopGame},
    {5, fourLapCopGame},
    {4, twoLapCopGameSplit},
    {4, fourLapCopGameSplit},
    {4, twoLapCopGame1H1AI},
    {5, fourLapCopGame1H1AI},
};

sint32 AICop::numArrestedHumans;
AICop::spikeBelt_t AICop::spikeBelt;
AICop::RoadBlockState AICop::gRoadBlockState = AICop::kAICop_RoadBlockState_None;
void *AICop::rawTriggers;

void AICop::StartUp(void)
{
    FUNCTION_MARKER(0x800669AC, "NFS4.EXE");
    if (GameSetup::gData.cops) {
        void *memory = reservememadr(0, sizeof(AITrigger_TriggerManager), 0);
        triggerManagerCops = ::new (memory) AITrigger_TriggerManager;
        char filename[100];
        sprintf(filename, "%sTr%02d.cop", Paths_Paths[22], GameSetup::gData.track);
        rawTriggers = loadfileadrz(filename, 0);
        triggerManagerCops->Init(static_cast<char *>(rawTriggers));
    }
    spikeBelt.active_ = 0;
    numArrestedHumans = 0;
    gRoadBlockState = kAICop_RoadBlockState_None;
}

void AICop::Restart(void)
{
    FUNCTION_MARKER(0x80066A58, "NFS4.EXE");
    void *triggers = rawTriggers;
    if (triggers && GameSetup::gData.cops)
        triggerManagerCops->Init(static_cast<char *>(triggers));
    spikeBelt.active_ = 0;
    numArrestedHumans = 0;
    gRoadBlockState = kAICop_RoadBlockState_None;
}

void AICop::CleanUp(void)
{
    FUNCTION_MARKER(0x80066AA8, "NFS4.EXE");
    AITrigger_TriggerManager *manager = triggerManagerCops;
    if (manager) {
        purgememadr(manager);
        triggerManagerCops = 0;
    }
    void *triggers = rawTriggers;
    if (triggers && GameSetup::gData.cops) {
        purgememadr(triggers);
        rawTriggers = 0;
    }
}

sint32 AICop::NoCopsInArea(sint32 slice, sint32 sliceDistance)
{
    FUNCTION_MARKER(0x80066B08, "NFS4.EXE");
    for (sint32 copLoop = 0; copLoop < Cars::gNumCopCars; ++copLoop) {
        Car::tObj *cop = Cars::gCopCarList[copLoop];
        if ((cop->AIFlags & 4) == 0) {
            sint32 absDistance = AIWorld::ApxSplineDistance(cop, slice);
            if (absDistance < 0)
                absDistance = (-absDistance);
            if (absDistance < sliceDistance)
                return 0;
        }
    }
    return 1;
}
