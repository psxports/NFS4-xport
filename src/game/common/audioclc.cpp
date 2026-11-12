#include <limits.h>
#include <stddef.h>
#include <stdlib.h>
#include "xport.h"
#include "eaclib/psx/eaclib.h"
#include "eaclib/psx/spktplay.h"
#include "game/common/audioclc.h"
#include "game/common/audiocmn.h"
#include "game/common/audiotrk.h"
#include "game/common/camera.h"
#include "game/common/cameraobj.h"
#include "game/common/car.h"
#include "game/common/carobj.h"
#include "game/common/copspeak.h"
#include "game/common/renderview.h"
#include "game/common/speech.h"
#include "game/psx/hud.h"
#include "lib/psx/fixddiv.h"
#include "lib/psx/fixdmult.h"

struct AudioClc::tCLCache
{
    Car::tObj *ptr;
    sint32 dst;
};

struct AudioClc::tSource
{
    Car::tObj *car;
    sint32 distToCamera;
    sint32 dopplerShift;
    sint32 relVelocity;
    sint32 gameTicks;
    sint32 distSq;
    sint32 frequency;
    sint32 channel;
    sint32 horn;
    sint32 hornOn;
    sint32 hornOff;
    sint32 hornCount;
    sint32 pursuit;
    sint32 yelled;
};

struct AudioClc::tPlayer
{
    sint32 cameraMode;
    sint32 gameTicks;
    sint32 warnings;
    coorddef lastview;
    AudioClc::tSource source;
};

sint32 AudioClc::gRandomPhrase;
sint32 AudioClc::gBullHornCount;
sint32 AudioClc::gBumpCopCount;
sint32 AudioClc::gLastVoice;
sint32 AudioClc::gLastphrase1;
sint32 AudioClc::gLastphrase2;
sint32 AudioClc::gLastphrase3;
DRender::tCalcView AudioClc::gRenderView;
coorddef *AudioClc::gCameraVelocity;
sint32 AudioClc::PursuitTime[9];
AudioClc::tPlayer AudioClc::gPlayer[2];
AudioClc::tSource AudioClc::gClosest[4];

static sint32 audioClcFixedDot(const coorddef &vector, const matrixtdef &matrix, sint32 row)
{
    uint32 result = (vector.x / 256) * (uint32)(matrix.m[row] / 256);
    result += (vector.y / 256) * (uint32)(matrix.m[row + 1] / 256);
    result += (vector.z / 256) * (uint32)(matrix.m[row + 2] / 256);
    return result;
}

static sint32 audioClcVelocityDot(const coorddef &left, const coorddef &right)
{
    uint32 result = (left.x / 256) * (uint32)(right.x / 256);
    result += (left.y / 256) * (uint32)(right.y / 256);
    result += (left.z / 256) * (uint32)(right.z / 256);
    return result;
}

void AudioClc::SndError(sint32 shandle)
{
    FUNCTION_MARKER(0x80074730, "NFS4.EXE");
}

void AudioClc::SetHorn(Car::tObj *car, sint32 state)
{
    FUNCTION_MARKER(0x80074738, "NFS4.EXE");
    for (sint32 i = 0; i < 4; ++i)
    {
        tSource *s = &gClosest[i];
        if (s->car == car)
        {
            if (state != 0)
            {
                s->horn = 1;
                s->hornOn = 0;
            }
            else if (s->horn != 0)
            {
                s->horn = 1;
                s->hornOn = 1;
                s->hornCount = 1;
            }
        }
    }
}

void AudioClc::HonkHorn(Car::tObj *car, sint32 numBeeps, sint32 ticksOn, sint32 ticksOff)
{
    FUNCTION_MARKER(0x800747A8, "NFS4.EXE");
    sint32 off = (-ticksOff);
    for (sint32 i = 0; i < 4; ++i)
    {
        tSource *s = &gClosest[i];
        if (s->car == car && s->horn == 0)
        {
            s->horn = ticksOn;
            s->hornOn = ticksOn;
            s->hornOff = off;
            s->hornCount = numBeeps;
        }
    }
}

void AudioClc::InitSource(AudioClc::tSource *s, Car::tObj *car)
{
    FUNCTION_MARKER(0x80074808, "NFS4.EXE");
    s->distToCamera = -1;
    s->dopplerShift = -1;
    s->car = car;
    s->gameTicks = 0;
    s->horn = 0;
    s->distSq = 0x1324;
    s->frequency = 0;
    s->pursuit = 0;
    s->yelled = 0;
}

void AudioClc::StartUp(void)
{
    FUNCTION_MARKER(0x80074838, "NFS4.EXE");
    gRandomPhrase = 0;
    gBullHornCount = 0;
    gBumpCopCount = 0;
    gLastVoice = -1;
    gLastphrase1 = -1;
    gLastphrase2 = -1;
    gLastphrase3 = -1;
    for (sint32 i = 0; i < 2; ++i)
    {
        tPlayer *p = &gPlayer[i];
        p->cameraMode = -1;
        p->gameTicks = 0;
        p->warnings = 0;
        InitSource(&p->source, 0);
        if (GameSetup::gData.commMode < 2)
        {
            if (i < Cars::gNumHumanRaceCars)
                p->source.car = Cars::gHumanRaceCarList[i];
        }
        else if (i == 0 && GameSetup::gData.localCar < Cars::gNumHumanRaceCars)
            p->source.car = Cars::gHumanRaceCarList[GameSetup::gData.localCar];
    }
    for (sint32 i = 0; i < 4; ++i)
        InitSource(&gClosest[i], 0);
    gCameraVelocity = 0;

    {
        AudioClc::tPlayer *p;
        (void)sizeof(p);
        AudioClc::tSource *c;
        (void)sizeof(c);
        sint32 i;
        (void)sizeof(i);
    }
}

sint32 AudioClc::CalcDopplerShiftRatio(coorddef *objectPos, coorddef *objectVel)
{
    FUNCTION_MARKER(0x80074994, "NFS4.EXE");
    coorddef *cameraPos = &gRenderView.translation;
    coorddef *cameraVel = gCameraVelocity;
    coorddef vectorToSound;
    vectorToSound.x = (objectPos->x - cameraPos->x) >> 8;
    vectorToSound.y = (objectPos->y - cameraPos->y) >> 8;
    vectorToSound.z = (objectPos->z - cameraPos->z) >> 8;
    Math::NormalizeVector(&vectorToSound);
    sint32 relativeVelocity = 0;
    if (objectVel != 0)
        relativeVelocity = audioClcVelocityDot(vectorToSound, *objectVel);
    if (cameraVel != 0)
        relativeVelocity = (relativeVelocity - audioClcVelocityDot(vectorToSound, *cameraVel));
    return fixeddiv(0x1540000, ((relativeVelocity + 0x1540000)));
}

sint32 AudioClc::CalcDistance(DRender::tCalcView *view, coorddef *object)
{
    FUNCTION_MARKER(0x80074B60, "NFS4.EXE");
    sint32 x = (object->x - view->translation.x);
    if (x < 0)
        x = (-x);
    sint32 y = (object->y - view->translation.y);
    sint32 z = (object->z - view->translation.z);
    if (y < 0)
        y = (-y);
    if (z < 0)
        z = (-z);
    sint32 length;
    if (z < x)
        length = (x + (z >> 2));
    else
        length = (z + (x >> 2));
    sint32 length1;
    if (length < y)
        length1 = (y + (length >> 2));
    else
        length1 = (length + (y >> 2));
    return length1;
}

sint32 AudioClc::CalcAzimuth(DRender::tCalcView *view, coorddef *object)
{
    FUNCTION_MARKER(0x80074BE8, "NFS4.EXE");
    coorddef temp;
    temp.x = (object->x - view->translation.x);
    temp.y = (object->y - view->translation.y);
    temp.z = (object->z - view->translation.z);
    sint32 x = audioClcFixedDot(temp, view->mrotation, 0);
    sint32 y = audioClcFixedDot(temp, view->mrotation, 6);
    if (GameSetup::gData.mirrorTrack != 0)
        x = (-x);
    return (((uint32)intatan(x >> 8, y >> 8) << 6) & 0xFFC0u);
}

sint32 AudioClc::CalcCarDirection(DRender::tCalcView *view, Car::tObj *car)
{
    FUNCTION_MARKER(0x80074D50, "NFS4.EXE");
    coorddef temp;
    temp.x = (car->N.position.x - view->translation.x);
    temp.y = (car->N.position.y - view->translation.y);
    temp.z = (car->N.position.z - view->translation.z);
    return audioClcFixedDot(temp, car->N.orientMat, 6);
}

sint32 AudioClc::CalcTrackAzimuth(DRender::tCalcView *view, Car::tObj *car)
{
    FUNCTION_MARKER(0x80074E24, "NFS4.EXE");
    coorddef direction;
    direction.x = car->N.roadMatrix.m[6];
    direction.y = car->N.roadMatrix.m[7];
    direction.z = car->N.roadMatrix.m[8];
    sint32 x = audioClcFixedDot(direction, view->mrotation, 0);
    sint32 y = audioClcFixedDot(direction, view->mrotation, 6);
    if (GameSetup::gData.mirrorTrack != 0)
        x = (-x);
    return (((uint32)intatan(x >> 8, y >> 8) << 6) & 0xFFC0u);
}

void AudioClc::SoundOpponentHorn(sint32 closestIndex, sint32 azimuth, sint32 dop, sint32 dsquare)
{
    FUNCTION_MARKER(0x80074F5C, "NFS4.EXE");
    AudioClc::tSource *source = &gClosest[closestIndex];
    sint32 cartype = source->car->carInfo->carType;
    sint32 carhornSFX = cartype == 0x30 ? 11 : 10;
    static const uint8 trafficFreqs[50] =
    {
        88, 80, 70, 65, 60, 50, 85, 90, 60, 45, 90, 50, 85, 75, 84, 74, 50, 70, 45, 72, 60, 60, 55, 70, 60, 45, 50, 75, 65, 82, 72, 60, 86, 34, 47, 82, 47, 80, 30, 30, 64, 74, 55, 50, 52, 52, 62, 21, 64, 21
    };
    uint32 difference = 0x1324 - (uint32)dsquare;
    sint32 scaled = ((difference << 7) - difference);
    sint64 product = (sint64)scaled * 0x6AFF5F81;
    sint32 high = (sint32)(product >> 32);
    sint32 iamp = (((high >> 11)) - (scaled >> 31));
    AudioCmn::PlaySFX((closestIndex + 0x25), carhornSFX, trafficFreqs[cartype], dop, iamp, azimuth);
}

void AudioClc::SilenceOpponentHorn(sint32 closestIndex)
{
    FUNCTION_MARKER(0x80075008, "NFS4.EXE");
    freeVoiceChannel(((closestIndex + 0x25)));
}

void AudioClc::SoundCloseCar(sint32 playerIndex, sint32 closestIndex)
{
    FUNCTION_MARKER(0x80075028, "NFS4.EXE");
    tSource *source = &gClosest[closestIndex];
    Car::tObj *car = source->car;
    if (car == 0)
        return;
    sint32 dop = CalcDopplerShiftRatio(&car->N.position, &car->N.linearVel);
    if (dop < 0)
        return;
    sint32 dst = CalcDistance(&gRenderView, &car->N.position);
    sint32 dir;
    if (fixeddiv(CalcCarDirection(&gRenderView, car), dst) <= 0x10000 && fixeddiv(CalcCarDirection(&gRenderView, car), dst) < -0x10000)
        dir = -0x10000;
    else if (fixeddiv(CalcCarDirection(&gRenderView, car), dst) > 0x10000)
        dir = 0x10000;
    else
        dir = fixeddiv(CalcCarDirection(&gRenderView, car), dst);
    sint32 azimuth;
    if (GameSetup::gData.commMode == 1)
        azimuth = playerIndex == 0 ? 0xC000 : 0x3FFF;
    else
        azimuth = CalcAzimuth(&gRenderView, &car->N.position);
    sint32 wholeDistance = dst / 0x10000;
    sint32 dsquare = (wholeDistance * wholeDistance);
    sint32 distSq = dsquare < 0x1324 ? dsquare : 0x1324;
    if (gCameraVelocity != 0)
    {
        sint32 speed = car->currentSpeed;
        sint32 playerSpeed = gPlayer[playerIndex].source.car->currentSpeed;
        sint32 relative = (speed - playerSpeed);
        source->relVelocity = relative > 0 ? relative : (playerSpeed - speed);
    }
    else
    {
        sint32 speed = car->currentSpeed;
        source->relVelocity = speed < 0 ? -speed : speed;
    }
    source->distSq = distSq;
    sint32 redline = car->specs->redline;
    sint32 rpm = ((uint32)car->flywheelRpm << 16);
    if (redline == 0 || (redline == -1 && rpm == INT_MIN))
        abort();
    sint32 relvel = car->linearVel_ch.z;
    if (relvel < 0)
        relvel = (-relvel);
    AudioCmn::TrafficSFX((closestIndex + 6), car->carInfo->carType, rpm / redline, dop, dst, azimuth, relvel, dir);
    for (sint32 c = (car->audioCount - 1); c >= 0; --c)
    {
        Cars::tAudio *audio = &car->audio[c];
        AudioCmn::TrafficSkidSFX(audio->channel < 0 ? -1 : (closestIndex + 32), (s_type)audio->surface1, (s_type)audio->surface2, audio->force, audio->channel < 0 ? distSq : dsquare, azimuth);
    }
    if ((car->carFlags & 4u) != 0)
    {
        if (car->control.horn != 0)
        {
            SoundOpponentHorn(closestIndex, azimuth, dop, distSq);
            source->horn = 1;
        }
        else if (source->horn != 0)
        {
            SilenceOpponentHorn(closestIndex);
            source->horn = 0;
        }
    }
    else if (source->horn > 0)
    {
        if (source->hornOn > 0)
            source->horn = (source->horn - 1);
        if (source->horn == 0)
        {
            car->control.horn = 0;
            SilenceOpponentHorn(closestIndex);
            source->hornCount = (source->hornCount - 1);
            if (source->hornCount != 0)
                source->horn = source->hornOff;
        }
        else
        {
            car->control.horn = 1;
            SoundOpponentHorn(closestIndex, azimuth, dop, distSq);
        }
    }
    else if (source->horn < 0)
    {
        source->horn = (source->horn + 1);
        if (source->horn == 0)
            source->horn = source->hornOn;
    }
    if (car->carInfo->carType - 22u < 6u)
    {
        if ((car->AIFlags & 2u) != 0)
        {
            uint32 difference = 0x1324 - (uint32)distSq;
            sint32 scaled = ((difference << 7) - difference);
            sint32 high = (sint32)(((sint64)scaled * 0x6AFF5F81) >> 32);
            sint32 iamp = (((high >> 11)) - (scaled >> 31));
            if (bSirenOn[closestIndex] == 0)
                SirenOn(closestIndex, car->carFlags & 0x40);
            else
                UpdateSiren(closestIndex, iamp, dop, azimuth, car->carFlags & 0x40);
        }
        else if (bSirenOn[closestIndex] != 0)
        {
            SirenOff(closestIndex);
            freeVoiceChannel(((closestIndex + 43)));
        }
    }
}

void AudioClc::SoundPlayersCar(sint32 playerIndex)
{
    FUNCTION_MARKER(0x80075508, "NFS4.EXE");
    DRender::tCalcView *view = &gRenderView;
    tSource *previous = &gPlayer[playerIndex].source;
    Car::tObj *car = previous->car;
    if (car == 0)
        return;
    sint32 facing = 0;
    if ((car->carFlags & 0x200u) != 0)
    {
        if (car->desiredDirection != car->direction)
            facing = -1;
    }
    else
    {
        sint32 x = fixedmult(car->N.orientMat.m[6], (sint8)BWorldSm::slices[car->N.simRoadInfo.slice].forward[0]);
        sint32 y = fixedmult(car->N.orientMat.m[7], (sint8)BWorldSm::slices[car->N.simRoadInfo.slice].forward[1]);
        sint32 z = fixedmult(car->N.orientMat.m[8], (sint8)BWorldSm::slices[car->N.simRoadInfo.slice].forward[2]);
        facing = ((x + y) + z);
        if (GameSetup::gData.reverseTrack != 0)
            facing = (-facing);
    }
    if (car->stats.finishType == 2 || ((GameSetup::gData.raceType == 1 || GameSetup::gData.raceType == 5) && ((Cars::gHumanRaceCarList[0]->carFlags & 0x200u) != 0 || (Cars::gNumHumanRaceCars == 2 && (Cars::gHumanRaceCarList[1]->carFlags & 0x200u) != 0)) && (car->carFlags & 0x204u) == 4u) || car->RSControl != 0 || car->pullOver != 0 || facing >= 0)
        car->wrongway = 0;
    else if (car->N.flightTime == 0 && car->collision.smoking == 0)
    {
        car->wrongway = (car->wrongway + 1);
        if (car->wrongway >= 64 && (car->wrongway & 31u) == 0 && (car->wrongway < 148 || Hud::BeTheCop != 0))
            AudioCmn::PlayWrongWaySFX();
    }
    sint32 dop = CalcDopplerShiftRatio(&car->N.position, &car->N.linearVel);
    sint32 azimuth;
    if (GameSetup::gData.commMode == 1)
        azimuth = playerIndex == 0 ? 0xC000 : 0x3FFF;
    else if (Camera::gInfo[playerIndex].mode == 0)
        azimuth = 0;
    else
        azimuth = CalcAzimuth(view, &car->N.position);
    sint32 dst = CalcDistance(view, &car->N.position);
    sint32 cardir;
    if (fixeddiv(CalcCarDirection(view, car), dst) <= 0x10000 && fixeddiv(CalcCarDirection(view, car), dst) < -0x10000)
        cardir = -0x10000;
    else if (fixeddiv(CalcCarDirection(view, car), dst) > 0x10000)
        cardir = 0x10000;
    else
        cardir = fixeddiv(CalcCarDirection(view, car), dst);
    sint32 wholeDistance = dst / 0x10000;
    sint32 dsquare = (wholeDistance * wholeDistance);
    sint32 revLimit = (car->specs->redline + (car->carInfo->Transmission == 1 ? 1000u : 2000u));
    if (car->N.flightTime >= 6)
        revLimit = (revLimit - 500);
    uint32 rpm = car->flywheelRpm;
    sint32 scaledRpm = ((rpm << 7) - rpm);
    if (revLimit == 0 || (revLimit == -1 && scaledRpm == INT_MIN))
        abort();
    sint32 frequency = scaledRpm / revLimit;
    if (gCameraVelocity != 0)
        previous->relVelocity = 0;
    else
    {
        sint32 speed = car->currentSpeed;
        previous->relVelocity = speed < 0 ? -speed : speed;
    }
    sint32 trkazi = CalcTrackAzimuth(view, car);
    AudioTrk::SoundTrack(car, trkazi);
    AudioCmn::SoundCar(car, dst, frequency, dop, azimuth, trkazi, previous->relVelocity, cardir);
    if (gMasterSFXLevel == 0)
        return;
    for (sint32 c = (car->audioCount - 1); c >= 0; --c)
    {
        Cars::tAudio *audio = &car->audio[c];
        sint32 channel = audio->channel;
        if (channel == 18 && playerIndex != 0)
            channel = 19;
        else if (channel == 20 && playerIndex != 0)
            channel = 21;
        if ((audio->type == 5 || audio->type == 3) && channel >= 0)
        {
            freeVoiceChannel(channel);
            if (channel - 18u < 2u)
                freeVoiceChannel(((channel + 4)));
        }
        else
        {
            sint32 soundAzimuth = azimuth;
            if (channel < 0 && GameSetup::gData.commMode != 1 && audio->surface1 != 10 && audio->surface1 != 8)
                soundAzimuth = CalcAzimuth(view, &car->N.collision.collisionPoint);
            AudioCmn::SFX(channel, (s_type)audio->surface1, (s_type)audio->surface2, audio->force, dsquare, soundAzimuth);
        }
    }
    if (car->control.horn != 0)
    {
        AudioCmn::PlayerHornOn(car->carIndex, dsquare, 64, azimuth, dop);
        previous->horn = 1;
    }
    else if (previous->horn != 0)
        previous->horn = AudioCmn::PlayerHornOff(car->carIndex);
    if (car->carInfo->carType - 22u < 6u)
    {
        if ((car->AIFlags & 2u) != 0)
        {
            sint32 iamp = 0;
            if (dsquare < 0x1324)
            {
                uint32 difference = 0x1324 - (uint32)dsquare;
                sint32 scaled = ((difference << 7) - difference);
                sint32 high = (sint32)(((sint64)scaled * 0x6AFF5F81) >> 32);
                iamp = (((high >> 11)) - (scaled >> 31));
            }
            sint32 siren = (car->carIndex + 4);
            if (bSirenOn[siren] == 0)
                SirenOn(siren, car->carFlags & 0x40);
            else
                UpdateSiren(siren, iamp, dop, azimuth, car->carFlags & 0x40);
        }
        else
        {
            sint32 siren = (car->carIndex + 4);
            if (bSirenOn[siren] != 0)
            {
                SirenOff(siren);
                freeVoiceChannel(((car->carIndex + 47)));
            }
        }
    }
}

void AudioClc::ResetClosest(sint32 closestIndex, Car::tObj *car, sint32 playerIndex)
{
    FUNCTION_MARKER(0x80075C3C, "NFS4.EXE");
    if (gClosest[closestIndex].car != 0)
    {
        if (car == 0)
        {
            freeVoiceChannel(((closestIndex + 6)));
            freeVoiceChannel(((closestIndex + 10)));
            freeVoiceChannel(((closestIndex + 14)));
        }
        if (bSirenOn[closestIndex] != 0)
            SirenOff(closestIndex);
        freeVoiceChannel(((closestIndex + 32)));
        freeVoiceChannel(((closestIndex + 37)));
    }
    InitSource(&gClosest[closestIndex], car);
}

void AudioClc::GetClosestCars(sint32 playerIndex, sint32 closestIndex, sint32 numclosest)
{
    FUNCTION_MARKER(0x80075D04, "NFS4.EXE");
    tCLCache cl[sizeof(gClosest) / sizeof(gClosest[0])];
    tSource *closest = &gClosest[closestIndex];
    for (sint32 i = 0; i < numclosest; ++i)
    {
        cl[i].ptr = 0;
        cl[i].dst = 0x12C0000;
    }
    Car::tObj **car = Cars::gList;
    for (sint32 i = 0; i < Cars::gNumCars; ++i, ++car)
    {
        if (GameSetup::gData.commMode == 1)
        {
            if (((*car)->carFlags & 4u) != 0)
                continue;
        }
        else if (*car == gPlayer[playerIndex].source.car)
            continue;
        if ((*car)->N.active == 0)
            continue;
        sint32 searchdist = ((*car)->carFlags & 0x10u) != 0 ? 0x320000 : 0x12C0000;
        sint32 x = ((*car)->N.position.x - gRenderView.translation.x);
        sint32 y = ((*car)->N.position.y - gRenderView.translation.y);
        sint32 z = ((*car)->N.position.z - gRenderView.translation.z);
        if (x < 0)
            x = (-x);
        if (y < 0)
            y = (-y);
        if (z < 0)
            z = (-z);
        sint32 distance1 = z < x ? (x + (z >> 2)) : (z + (x >> 2));
        if (distance1 < 0x1900000)
        {
            sint32 patch = CopSpeak::GetEnginePatch((*car)->carInfo->carType, 0);
            if (patch >= 0)
                AudioCmn::GetAsyncSfx(1, patch, false);
        }
        if (distance1 >= searchdist)
            continue;
        sint32 distance = distance1 < y ? (y + (distance1 >> 2)) : (distance1 + (y >> 2));
        if (distance >= searchdist)
            continue;
        for (sint32 j = 0; j < numclosest; ++j)
        {
            if (distance < cl[j].dst)
            {
                for (sint32 k = numclosest - 1; j < k; --k)
                    cl[k] = cl[k - 1];
                cl[j].dst = distance;
                cl[j].ptr = *car;
                break;
            }
        }
    }
    Car::tObj *removed = reinterpret_cast<Car::tObj *>(static_cast<size_t>(-1));
    for (sint32 i = 0; i < numclosest; ++i)
    {
        if (closest[i].car != 0)
        {
            sint32 j;
            for (j = 0; j < numclosest; ++j)
            {
                if (cl[j].ptr == closest[i].car)
                {
                    cl[j].ptr = 0;
                    break;
                }
            }
            if (j == numclosest)
                closest[i].car = removed;
        }
    }
    for (sint32 i = 0; i < numclosest; ++i)
    {
        if (cl[i].ptr != 0)
        {
            for (sint32 j = 0; j < numclosest; ++j)
            {
                if (closest[j].car == 0 || closest[j].car == removed)
                {
                    ResetClosest((closestIndex + j), cl[i].ptr, playerIndex);
                    break;
                }
            }
        }
    }
    for (sint32 i = 0; i < numclosest; ++i)
    {
        if (closest[i].car == removed)
            ResetClosest((closestIndex + i), 0, playerIndex);
    }
}

void AudioClc::SoundSpeech(void)
{
    FUNCTION_MARKER(0x80076130, "NFS4.EXE");
    AudioCmn::SetLevels();
    Speech::Server();
    CopSpeak::Server();
}

void AudioClc::SoundCars(void)
{
    FUNCTION_MARKER(0x80076160, "NFS4.EXE");
    SoundSpeech();
    AudioCmn::UpdateThunder();
    if ((GameSetup::gData.raceType == 1 || GameSetup::gData.raceType == 5) && ((Cars::gHumanRaceCarList[0]->carFlags & 0x200u) != 0 || (Cars::gNumHumanRaceCars == 2 && (Cars::gHumanRaceCarList[1]->carFlags & 0x200u) != 0)) && HudBustedOverlay != 0)
    {
        for (sint32 i = 0; i < GameSetup::gData.numPerps; ++i)
        {
            sint32 patch = CopSpeak::GetEnginePatch(GameSetup::gData.perpInfo[i].CarType, 0);
            AudioCmn::GetAsyncSfx(1, patch, false);
            patch = CopSpeak::GetEnginePatch(GameSetup::gData.perpInfo[i].CarType, 1);
            AudioCmn::GetAsyncSfx(1, patch, false);
        }
    }
    if (GameSetup::gData.commMode == 1)
    {
        Camera::GetAudioViewInfo(0, &gRenderView, &gCameraVelocity);
        GetClosestCars(0, 0, 2);
        SoundPlayersCar(0);
        if (gMasterAmbientLevel > 0)
        {
            SoundCloseCar(0, 0);
            SoundCloseCar(0, 1);
        }
        gPlayer[0].lastview = gRenderView.translation;
        Camera::GetAudioViewInfo(1, &gRenderView, &gCameraVelocity);
        GetClosestCars(1, 2, 2);
        SoundPlayersCar(1);
        if (gMasterAmbientLevel > 0)
        {
            SoundCloseCar(1, 2);
            SoundCloseCar(1, 3);
        }
        gPlayer[1].lastview = gRenderView.translation;
    }
    else
    {
        Camera::GetAudioViewInfo(0, &gRenderView, &gCameraVelocity);
        GetClosestCars(0, 0, 4);
        SoundPlayersCar(0);
        if (gMasterAmbientLevel > 0)
        {
            SoundCloseCar(0, 0);
            SoundCloseCar(0, 1);
            SoundCloseCar(0, 2);
            SoundCloseCar(0, 3);
        }
        gPlayer[0].lastview = gRenderView.translation;
    }
}
