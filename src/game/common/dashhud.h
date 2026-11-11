#ifndef DASHHUD_H
#define DASHHUD_H

#include "xport.h"
#include "psx.h"
#include "game/common/car.h"

struct dashhud_info
{
    sint32 splitscreen;
    char name[9];
    sint32 conversion;
    sint32 flashtime;
    sint32 flashlap;
    sint32 showhud[2];
    sint32 showmap[2];
    sint32 wrongway[2];
    sint32 laptime;
    sint32 lap;
    sint32 maxlaps;
    sint32 rpm;
    sint32 redline;
    sint32 gear;
    sint32 speed;
    sint32 topspeed;
    sint32 position;
    sint32 opponents;
    sint32 record;
    sint32 tutor;
    sint32 warning[2];
};

class DashHUD
{
public:

    typedef dashhud_info tInfo;
    static tInfo gInfo;

    static void InitHUD(void);
    static void ResetHUD(void);
    static void ToggleHud(sint32 player);
    static void CheckWrongWay(sint32 player);
    static void HUDCalc(sint32 player);
};

#endif
