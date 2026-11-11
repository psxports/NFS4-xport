#ifndef MPAUSE_H
#define MPAUSE_H

#include "xport.h"
#include "game/common/car.h"

class MPause
{
public:
    static void MusicLogic(char active);
    static void ControllerLogic(void);
    static sint32 Logic(void);
    static void Render(void);
    static void InitMPause(void);
    static void StartPauseMenu(void);
    static void EndPauseMenu(void);
    static void KillMPause(void);
};

extern sint32 gMPauseUpdate;
extern sint32 gMPauseUpdateNextTime;
extern sint16 InGameSelectListAudioMode[4];
extern sint16 SelectListConfig[4];

#endif
