#ifndef FORCE_H
#define FORCE_H

#include "xport.h"
#include "game/common/car.h"

class Force
{
public:
    struct tGlobal
    {
        uint8 active;
        uint8 high;
        uint8 low;
        uint8 jolt;
        uint8 time;
        uint8 fade;
        uint8 actuator[2];
    };

    static tGlobal g[2];
    static sint32 gVblHandle;
    static uint16 gTick;
    static uint8 rand_256[256];
    static uint8 gActAlign[6];
    static uint8 gOffAlign[6];

    static void Vbl(void);
    static void Update(Car::tObj *car);
    static void StartUp(void);
    static void Disable(void);
    static sint32 IsForceOn(Car::tObj *car);
    static void Pause(void);
    static void UnPause(void);
    static void HitSign(Car::tObj *car);
    static void HitWall(sint32 impulse);
};

#endif
