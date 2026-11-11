#ifndef AUDIOCLC_H
#define AUDIOCLC_H

#include "xport.h"
#include "game/common/car.h"
#include "game/common/coorddef.h"
#include "game/common/renderview.h"

class AudioClc
{
public:
    static DRender::tCalcView gRenderView;
    static coorddef *gCameraVelocity;

    static sint32 gRandomPhrase;
    static sint32 gBullHornCount;
    static sint32 gBumpCopCount;
    static sint32 gLastVoice;
    static sint32 gLastphrase1;
    static sint32 gLastphrase2;
    static sint32 gLastphrase3;
    static sint32 PursuitTime[9];

    struct tCLCache;

    struct tSource;

    struct tPlayer;
    static tPlayer gPlayer[2];
    static tSource gClosest[4];

    static void SndError(sint32 shandle);
    static void SetHorn(Car::tObj *car, sint32 state);
    static void HonkHorn(Car::tObj *car, sint32 numBeeps, sint32 ticksOn, sint32 ticksOff);
    static void InitSource(tSource *s, Car::tObj *car);
    static void StartUp(void);
    static sint32 CalcDopplerShiftRatio(coorddef *objectPos, coorddef *objectVel);
    static sint32 CalcDistance(DRender::tCalcView *view, coorddef *object);
    static sint32 CalcAzimuth(DRender::tCalcView *view, coorddef *object);
    static sint32 CalcCarDirection(DRender::tCalcView *view, Car::tObj *car);
    static sint32 CalcTrackAzimuth(DRender::tCalcView *view, Car::tObj *car);
    static void SoundOpponentHorn(sint32 closestIndex, sint32 azimuth, sint32 dop, sint32 dsquare);
    static void SilenceOpponentHorn(sint32 closestIndex);
    static void SoundCloseCar(sint32 playerIndex, sint32 closestIndex);
    static void SoundPlayersCar(sint32 playerIndex);
    static void ResetClosest(sint32 closestIndex, Car::tObj *car, sint32 playerIndex);
    static void GetClosestCars(sint32 playerIndex, sint32 closestIndex, sint32 numclosest);
    static void SoundSpeech(void);
    static void SoundCars(void);
};

#endif
