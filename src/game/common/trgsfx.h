#ifndef TRGSFX_H
#define TRGSFX_H

#include "xport.h"
#include "game/common/car.h"
#include "game/common/coorddef.h"

class TrgSfx
{
public:
    static void AddEnviroEffect(sint32 obj, sint32 type, coorddef *emitterpt, coorddef *vec);
    static void AddCarSfx(sint32 car, coorddef *skidpt, sint32 type, coorddef *vec);
    static void AddCarWheelSfx(sint32 car, sint32 wheel, coorddef *skidpt, sint32 type, coorddef *vec, sint32 delay);
    static bool AddCarExtraCheck(sint32 car, sint32 wheel);
    static void AddCarExtraSfx(sint32 car, sint32 wheel, coorddef *skidpt, sint32 type, coorddef *vec, sint32 velY, sint32 ground, sint32 colour);
    static void AddCarSplash(sint32 car, sint32 wheel, coorddef *skidpt, sint32 type, coorddef *vec, sint32 delay, sint32 velXZ);
    static void CrashCar(coorddef *location);
    static void AddSkidmark(sint32 car, sint32 wheel, coorddef *skidpt, sint32 end, sint32 intensity, Car::tObj *carObj, sint32 type);
    static void InitTrgSfx(void);
    static void RestartTrgSfx(void);
    static void KillTrgSfx(void);
};

extern sint32 gTAddCarWheelDelay;

#endif
