#ifndef REPLAY_H
#define REPLAY_H

#include "xport.h"
#include "game/common/car.h"

struct tControllerData;

struct tReplayInterface
{
    sint32 pause;
    sint32 speed;
    sint32 end;
    sint32 camera;
    sint32 selection;
    sint32 depressed;
    sint32 changeCamera;
    sint32 statsScreen;
};

struct tReplayBuffer;

struct tReplayCameraModes
{
    sint32 defaultCamera;
    sint32 timeCounter;
    sint32 cameraMode;
    sint32 cutToNextCamera;
};

class Replay
{
public:
    static tReplayInterface ReplayInterface;
    static tReplayBuffer ReplayBuffer;
    static tReplayCameraModes ReplayCamera[2];

    static sint32 ReplayMode;
    static sint32 ReplayStorePtr;
    static sint32 ReplayGetPtr;
    static sint32 Size;
    static sint32 ReplayCounter[2];

    static char * Compress(char *uncompressed_data);
    static char * Decompress(char *compressed_data);
    static void InitReplay(void);
    static void ResetReplay(void);
    static void StoringReplay(void);
    static void StoringControllerData(tControllerData controllerdata);
    static tControllerData RetreivingControllerData(void);
    static void SaveInput(sint32 car);
    static void GetInput(sint32 car);
    static void SaveReplay(void);
    static void LoadReplay(void);
    static void DoReplay(Car::tObj *carObj);
    static void GetInterfaceKey(void);
    static void LoadCameraFile(void);
    static void ReplayChooseCamera(sint32 player, sint32 slice);
    static void ReplayFindClosestCamera(sint32 player, sint32 slice);
};

extern sint32 numValidCams;

#endif
