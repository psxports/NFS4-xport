#ifndef SIM_H
#define SIM_H

#include "xport.h"
#include "game/common/car.h"
#include "game/common/schedule.h"

class Sim
{
public:
    struct tSimGlobalVar
{
    sint32 gameStarted;
    sint32 gameTicks;
    sint32 time32Hz;
    Sched::tSchedule *schedule64Hz;
    Sched::tSchedule *schedule32Hz;
    Sched::tSchedule *schedule32Hz2;
};

    struct tSimSystemVar
    {
        sint32 restartGame;
        sint32 endSimGame;
        sint32 pauseSim;
        sint32 keyRelease;
        sint32 quickPauseSim;
        sint32 goalClockTicks;
        sint32 currentClockTicks;
    };

    static void StartUp(void);
    static void Restart(void);
    static void CleanUp(void);
    static void FadeInSFX(void);
    static void ProcessSimSchedules(void);
    static void ProcessPause(void);
    static void CheckForPause(sint32 checkInput);
    static void MainGameLoop(void);
};

extern Sim::tSimGlobalVar simGlobal;
extern Sim::tSimSystemVar simVar;
extern sint32 InBetween;
extern sint32 quitType;

#endif
