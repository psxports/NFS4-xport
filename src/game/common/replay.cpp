#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include "xport.h"
#include "eaclib/psx/eaclib.h"
#include "game/common/audiocmn.h"
#include "game/common/bworldsm.h"
#include "game/common/camera.h"
#include "game/common/cameraobj.h"
#include "game/common/cameraslot.h"
#include "game/common/car.h"
#include "game/common/cars.h"
#include "game/common/gmesetup.h"
#include "game/common/input.h"
#include "game/common/paths.h"
#include "game/common/replay.h"
#include "game/common/sim.h"
#include "game/psx/hud.h"

struct ReplayCompressedStorage
{
    char bytes[33];
    uint8 padding[3];
};

struct tControllerData
{
    char steering[32];
    uint8 gas[32];
    uint8 brake[32];
    uint8 states[32];
};

struct tReplayBuffer
{
    GameSetup::tData gameSetup;
    sint32 size;
    char buffer[24576];
};

static ReplayCompressedStorage replayCompressedStorage;
sint32 numValidCams;
char (&compressed_data)[33] = replayCompressedStorage.bytes;
char uncompressed_data[32];
sint32 ReplayCameraList[9] = { 19, 10, 11, 8, 9, 6, 4, 15, 0 };
tControllerData controlData[2];
Camera::tCamSlot gReplayCameraSlots[32];
sint32 Replay::ReplayMode;
sint32 Replay::ReplayStorePtr;
sint32 Replay::ReplayGetPtr;
sint32 Replay::Size;
tReplayInterface Replay::ReplayInterface;
tReplayBuffer Replay::ReplayBuffer;
sint32 Replay::ReplayCounter[2];
tReplayCameraModes Replay::ReplayCamera[2];

char * Replay::Compress(char *uncompressed_data)
{
    FUNCTION_MARKER(0x800B3C4C, "NFS4.EXE");
    char *end = uncompressed_data + 32;
    char *output = reinterpret_cast<char *>(&replayCompressedStorage);
    sint32 c_pointer = 1;
    sint32 done = 0;
    do
    {
        uint8 begin_byte = (uint8)*uncompressed_data;
        sint32 count = 0;
        do
        {
            ++uncompressed_data;
            ++count;
        } while (uncompressed_data < end && (uint8)*uncompressed_data == begin_byte);
        if (count >= 3)
        {
            output[c_pointer] = (char)0xFF;
            output[c_pointer + 1] = (char)count;
            output[c_pointer + 2] = (char)begin_byte;
            c_pointer += 3;
        }
        else
        {
            for (sint32 i = 0; i < count; ++i)
                output[c_pointer++] = (char)begin_byte;
        }
        if (uncompressed_data >= end)
            done = 1;
    } while (done == 0);
    output[0] = (char)c_pointer;
    output[c_pointer] = 0;
    return compressed_data;
}

char * Replay::Decompress(char *compressed_data)
{
    FUNCTION_MARKER(0x800B3D10, "NFS4.EXE");
    sint32 data_size = (uint8)compressed_data[0];
    char *output = uncompressed_data;
    for (sint32 c_pointer = 1; c_pointer < data_size;) {
        uint8 current_byte = (uint8)compressed_data[c_pointer];
        if (current_byte == 0xFF) {
            sint32 count = (uint8)compressed_data[c_pointer + 1];
            for (sint32 i = 0; i < count; ++i) *output++ = compressed_data[c_pointer + 2];
            c_pointer += 3;
        } else {
            *output++ = (char)current_byte;
            ++c_pointer;
        }
    }
    return uncompressed_data;
}

void Replay::InitReplay(void)
{
    FUNCTION_MARKER(0x800B3DA0, "NFS4.EXE");
    ReplayMode = GameSetup::gData.replayMode;
    LoadCameraFile();
    if (ReplayMode == 2)
    {
        GameSetup::tUserSetting temp;
        memcpy(&temp, &GameSetup::gData.userSetting, sizeof(temp));
        LoadReplay();
        memcpy(&GameSetup::gData, &ReplayBuffer.gameSetup, sizeof(GameSetup::gData));
        memcpy(&GameSetup::gData.userSetting, &temp, sizeof(temp));
        sint32 size = ReplayBuffer.size;
        ReplayGetPtr = 0;
        Size = size;
    }
    else
        memcpy(&ReplayBuffer.gameSetup, &GameSetup::gData, sizeof(GameSetup::gData));
}

void Replay::ResetReplay(void)
{
    FUNCTION_MARKER(0x800B3F44, "NFS4.EXE");
    sint32 mode = ReplayMode;
    if ((uint32)mode < 2U)
    {
        for (sint32 i = 24575; i >= 0; --i)
            ReplayBuffer.buffer[i] = 0;
        ReplayStorePtr = 0;
        ReplayGetPtr = 0;
    }
    else if (mode >= 2)
    {
        ReplayInterface.pause = 0;
        ReplayInterface.speed = 2;
        ReplayGetPtr = 0;
        ReplayInterface.end = 0;
        ReplayInterface.camera = 0;
        ReplayInterface.selection = 4;
        ReplayInterface.depressed = 0;
        ReplayInterface.changeCamera = 0;
        for (sint32 i = 0; i < 2; ++i)
        {
            sint32 cameraMode = 4;
            if (numValidCams != 0)
            {
                if (GameSetup::gData.commMode == 1 || i == 0)
                    ReplayFindClosestCamera(i, Cars::gHumanRaceCarList[i]->N.simRoadInfo.slice);
                cameraMode = 19;
            }
            ReplayCamera[i].cameraMode = cameraMode;
            ReplayCamera[i].cutToNextCamera = 0;
        }
    }
    ReplayCounter[1] = 0;
    ReplayCounter[0] = 0;
    StatsTimer[0] = 0;
    StatsTimer[1] = 0;
}

void Replay::StoringReplay(void)
{
    FUNCTION_MARKER(0x800B409C, "NFS4.EXE");
    SaveReplay();
    numValidCams = 0;
}

void Replay::StoringControllerData(tControllerData controllerdata)
{
    FUNCTION_MARKER(0x800B40BC, "NFS4.EXE");
    if ((ReplayStorePtr + 128) >= 24576)
    {
        sint32 size = simGlobal.gameTicks;
        ReplayMode = 1;
        Size = size;
        return;
    }
    char packeddata[33];
    char *channels[4] =
    {
        controllerdata.steering,
        reinterpret_cast<char *>(controllerdata.gas),
        reinterpret_cast<char *>(controllerdata.brake),
        reinterpret_cast<char *>(controllerdata.states)
    };
    for (sint32 i = 0; i < 4; ++i)
    {
        char *packed = Compress(channels[i]);
        memcpy(packeddata, packed, sizeof(packeddata));
        sint32 length = (uint8)packeddata[0];
        sint32 tail = 24576 - ReplayStorePtr;
        if (length <= tail)
            memcpy(&ReplayBuffer.buffer[ReplayStorePtr], packeddata, length);
        else
        {
            memcpy(&ReplayBuffer.buffer[ReplayStorePtr], packeddata, tail);
            memcpy(compressed_data, packeddata + tail, length - tail);
        }
        ReplayStorePtr = (ReplayStorePtr + ((uint8)packeddata[0]));
    }
}

tControllerData Replay::RetreivingControllerData(void)
{
    FUNCTION_MARKER(0x800B4488, "NFS4.EXE");
    tControllerData controllerdata;
    char packeddata[33];
    char *channels[4] =
    {
        controllerdata.steering,
        reinterpret_cast<char *>(controllerdata.gas),
        reinterpret_cast<char *>(controllerdata.brake),
        reinterpret_cast<char *>(controllerdata.states)
    };
    for (sint32 i = 0; i < 4; ++i) {
        sint32 tail = 24576 - ReplayGetPtr;
        const char *packet = &ReplayBuffer.buffer[ReplayGetPtr];
        sint32 length = (uint8)packet[0];
        if (length <= tail) {
            memcpy(packeddata, packet, length);
        } else {
            memcpy(packeddata, packet, tail);
            memcpy(packeddata + tail, compressed_data, length - tail);
        }
        char *decoded = Decompress(packeddata);
        memcpy(channels[i], decoded, 32);
        ReplayGetPtr = (ReplayGetPtr + ((uint8)packeddata[0]));
    }
    return controllerdata;
}

void Replay::SaveInput(sint32 car)
{
    FUNCTION_MARKER(0x800B4824, "NFS4.EXE");
    Input::Fetch(car);
    controlData[car].steering[ReplayCounter[car]] = (char)(((sint8)Input::gSim.steering) / 4 + 64);
    controlData[car].gas[ReplayCounter[car]] = Input::gSim.gas >> 3;
    controlData[car].brake[ReplayCounter[car]] = Input::gSim.brake >> 3;
    controlData[car].states[ReplayCounter[car]] = Input::gSim.flags;
    if (Cars::gHumanRaceCarList[car]->carInfo->RampSteering != 0)
        controlData[car].steering[ReplayCounter[car]] = (char)((uint8)controlData[car].steering[ReplayCounter[car]] | 0x80U);
    if (Cars::gHumanRaceCarList[car]->carInfo->RampGas != 0)
        controlData[car].gas[ReplayCounter[car]] |= 0x80;
    if (Cars::gHumanRaceCarList[car]->carInfo->RampBrake != 0)
        controlData[car].brake[ReplayCounter[car]] |= 0x80;
    sint32 count = (ReplayCounter[car] + 1);
    ReplayCounter[car] = count;
    if (count == 32)
    {
        ReplayCounter[car] = 0;
        StoringControllerData(controlData[car]);
    }
}

void Replay::GetInput(sint32 car)
{
    FUNCTION_MARKER(0x800B4B38, "NFS4.EXE");
    Input::Fetch(car);
    if (ReplayCounter[car] == 0)
    {
        if (ReplayGetPtr >= ReplayStorePtr || (uint8)(ReplayGetPtr < 24576 ? ReplayBuffer.buffer[ReplayGetPtr] : compressed_data[ReplayGetPtr - 24576]) == 0)
            ReplayMode = 3;
        else
        {
            tControllerData temp = RetreivingControllerData();
            memcpy(&controlData[car], &temp, sizeof(temp));
        }
    }
    if (ReplayMode == 2)
    {
        Cars::gHumanRaceCarList[car]->carInfo->RampSteering = ((uint8)controlData[car].steering[ReplayCounter[car]] & 0x80U) != 0;
        Cars::gHumanRaceCarList[car]->carInfo->RampGas = (controlData[car].gas[ReplayCounter[car]] & 0x80U) != 0;
        Cars::gHumanRaceCarList[car]->carInfo->RampBrake = (controlData[car].brake[ReplayCounter[car]] & 0x80U) != 0;
        controlData[car].steering[ReplayCounter[car]] = (char)((uint8)controlData[car].steering[ReplayCounter[car]] & 0x7FU);
        controlData[car].gas[ReplayCounter[car]] &= 0x7F;
        controlData[car].brake[ReplayCounter[car]] &= 0x7F;
        Input::gSim.steering = (char)((((uint8) controlData[car].steering[ReplayCounter[car]]) - 64) * 4);
        Input::gSim.gas = (uint8)(controlData[car].gas[ReplayCounter[car]] << 3);
        Input::gSim.brake = (uint8)(controlData[car].brake[ReplayCounter[car]] << 3);
        Input::gSim.flags = controlData[car].states[ReplayCounter[car]];
    }
    else if (ReplayMode == 3)
    {
        Input::gSim.steering = 0;
        Input::gSim.gas = 0;
        Input::gSim.brake = 0;
        Input::gSim.flags = 0;
    }
    sint32 count = (ReplayCounter[car] + 1);
    sint32 validCams = numValidCams;
    ReplayCounter[car] = count;
    if (validCams != 0 && ReplayCamera[car].cameraMode == 19 && simGlobal.gameStarted != 0)
        ReplayChooseCamera(car, Cars::gHumanRaceCarList[car]->N.simRoadInfo.slice);
    if (ReplayCounter[car] == 32)
        ReplayCounter[car] = 0;
}

void Replay::SaveReplay(void)
{
    FUNCTION_MARKER(0x800B4F98, "NFS4.EXE");
    if (ReplayMode == 0)
        Size = simGlobal.gameTicks;
    memcpy(&ReplayBuffer.gameSetup, &GameSetup::gData, sizeof(GameSetup::gData));
    sint32 size = Size;
    ReplayBuffer.gameSetup.replayMode = 2;
    ReplayBuffer.size = size;
}

void Replay::LoadReplay(void)
{
    FUNCTION_MARKER(0x800B5024, "NFS4.EXE");
}

void Replay::DoReplay(Car::tObj *carObj)
{
    FUNCTION_MARKER(0x800B502C, "NFS4.EXE");
    if ((uint32)ReplayMode < 2U)
        SaveInput(carObj->humanIndex);
    else
        GetInput(carObj->humanIndex);
}

void Replay::GetInterfaceKey(void)
{
    FUNCTION_MARKER(0x800B5070, "NFS4.EXE");
    sint32 Cameras = GameSetup::gData.commMode == 1 ? 2 : 1;
    ReplayInterface.changeCamera = 0;
    if (ReplayInterface.depressed > 0)
        ReplayInterface.depressed = (ReplayInterface.depressed - 1);
    else if (ReplayInterface.statsScreen != 0)
    {
        if (Input::Interface(5, 1) != 0 || Input::Interface(6, 1) != 0)
        {
            ReplayInterface.depressed = 24;
            ReplayInterface.end = 1;
        }
        else if (Input::Interface(25, 1) != 0)
        {
            AudioCmn::PlayPauseSound(4);
            ReplayInterface.statsScreen = 0;
        }
    }
    else if (Input::Interface(3, 1) != 0)
    {
        AudioCmn::PlayPauseSound(3);
        sint32 selection = ReplayInterface.selection;
        ReplayInterface.selection = selection == 0 ? 4 : (selection - 1);
    }
    else if (Input::Interface(4, 1) != 0)
    {
        AudioCmn::PlayPauseSound(3);
        ReplayInterface.selection = (ReplayInterface.selection + 1) % 5;
    }
    else if (Input::Interface(5, 1) != 0)
    {
        AudioCmn::PlayPauseSound(4);
        ReplayInterface.depressed = 8;
        switch (ReplayInterface.selection)
        {
        case 0:
            ReplayInterface.end = 1;
            GameSetup::gData.instantReplay = 1;
            break;
        case 1:
            ReplayInterface.end = 1;
            break;
        case 2:
            ReplayInterface.pause ^= 1;
            break;
        case 3:
            if (GameSetup::gData.commMode == 1)
                ReplayInterface.speed = (ReplayInterface.speed + 1) % 3;
            else
                ReplayInterface.speed = (ReplayInterface.speed + 1) % 4;
            break;
        case 4:
            ReplayInterface.changeCamera = 1;
            ReplayInterface.camera = (ReplayInterface.camera + 1) % 9;
            if (ReplayCameraList[ReplayInterface.camera] == 19)
            {
                for (sint32 i = 0; i < Cameras; ++i)
                {
                    if (numValidCams == 0)
                        Camera::SetMode(i, 10);
                    else
                    {
                        Camera::gInfo[i].anchor = &Cars::gHumanRaceCarList[i]->N;
                        Camera::gInfo[i].target = &Cars::gHumanRaceCarList[i]->N;
                        ReplayFindClosestCamera(i, Cars::gHumanRaceCarList[i]->N.simRoadInfo.slice);
                        ReplayCamera[i].cameraMode = 19;
                    }
                }
            }
            else
            {
                for (sint32 i = 0; i < Cameras; ++i)
                {
                    ReplayCamera[i].defaultCamera = 0;
                    Camera::gInfo[i].twist = 0;
                    Camera::gInfo[i].anchor = &Cars::gHumanRaceCarList[i]->N;
                    Camera::gInfo[i].target = &Cars::gHumanRaceCarList[i]->N;
                    Camera::SetMode(i, ReplayCameraList[ReplayInterface.camera]);
                    ReplayCamera[i].cameraMode = ReplayCameraList[ReplayInterface.camera];
                }
            }
            break;
        }
    }
    else if (Input::Interface(6, 1) != 0)
    {
        ReplayInterface.depressed = 8;
        ReplayInterface.end = 1;
    }
    else if (Input::Interface(25, 1) != 0)
    {
        StatsTimer[0] = 0;
        StatsTimer[1] = 0;
        ReplayInterface.statsScreen = 1;
        AudioCmn::PlayPauseSound(4);
    }
    if (simVar.quickPauseSim != 0)
        Camera::Update();
}

void Replay::LoadCameraFile(void)
{
    FUNCTION_MARKER(0x800B5500, "NFS4.EXE");
    if (numValidCams != 0)
        return;
    char fname[80];
    NativeFileSlot *bigFile = 0;
    sprintf(fname, "%scamera.viv", Paths_Paths[8]);
    XPORT_UNPLACED::FILE::addbigsync(fname, 16, 100, &bigFile);
    if (GameSetup::gData.reverseTrack != 0)
        sprintf(fname, "tr%02dr.rho", GameSetup::gData.track);
    else
        sprintf(fname, "tr%02d.rho", GameSetup::gData.track);
    Camera::tCamSlot *cameraFile = static_cast<Camera::tCamSlot *>(loadfileadrz(fname, 0));
    numValidCams = 0;
    XPORT_UNPLACED::FILE::delbigsync(bigFile, 100);
    for (sint32 i = 0; i < 32; ++i)
    {
        gReplayCameraSlots[i].mode = 2;
        gReplayCameraSlots[i].slice = -1;
        gReplayCameraSlots[i].zoom = 0;
    }
    if (cameraFile == 0)
        return;
    memcpy(gReplayCameraSlots, cameraFile, sizeof(gReplayCameraSlots));
    for (sint32 i = 0; i < 32; ++i)
    {
        if (gReplayCameraSlots[i].fov == 0)
            gReplayCameraSlots[i].slice = -1;
        else
            ++numValidCams;
    }
    purgememadr(cameraFile);
    for (sint32 j = 0; j < 31; ++j)
    {
        for (sint32 i = 0; i < 31; ++i)
        {
            Camera::tCamSlot *current = &gReplayCameraSlots[i];
            Camera::tCamSlot *next = &gReplayCameraSlots[i + 1];
            if ((next->slice < current->slice || current->slice < 0) && next->slice >= 0)
            {
                Camera::tCamSlot temp;
                memcpy(&temp, current, sizeof(temp));
                memcpy(current, next, sizeof(temp));
                memcpy(next, &temp, sizeof(temp));
            }
        }
    }
    if (GameSetup::gData.reverseTrack != 0)
    {
        sint32 i = numValidCams - 1;
        sint16 temp;
        if (i < 0)
            memcpy(&temp, &uncompressed_data[30], sizeof(temp));
        else
            temp = gReplayCameraSlots[i].slice;
        for (; i >= 0; --i)
        {
            if (i == 0)
                gReplayCameraSlots[0].slice = temp;
            else
                gReplayCameraSlots[i].slice = gReplayCameraSlots[i - 1].slice;
        }
    }
    sint32 count = numValidCams;
    for (sint32 i = 0; i < count; ++i)
    {
        gReplayCameraSlots[i].pos.x = ((uint32)gReplayCameraSlots[i].pos.x << 6);
        gReplayCameraSlots[i].pos.z = ((uint32)gReplayCameraSlots[i].pos.z << 6);
        gReplayCameraSlots[i].pos.y = ((uint32)gReplayCameraSlots[i].pos.y << 6);
    }
}

void Replay::ReplayChooseCamera(sint32 player, sint32 slice)
{
    FUNCTION_MARKER(0x800B586C, "NFS4.EXE");
    sint32 currIndex = ReplayCamera[player].cutToNextCamera;
    sint32 prevIndex = (currIndex - 1);
    if (prevIndex < 0)
        prevIndex = (numValidCams - 1);
    sint32 nextIndex = (currIndex + 1);
    if (nextIndex >= numValidCams)
        nextIndex = 0;
    sint32 currDist = gReplayCameraSlots[currIndex].slice;
    sint32 nextDist = gReplayCameraSlots[nextIndex].slice;
    bool movedBack = false;
    if (slice < currDist)
    {
        sint32 distance = (slice - currDist);
        if (distance <= 0)
            distance = (currDist - slice);
        if (distance < 200)
        {
            ReplayCamera[player].cutToNextCamera = prevIndex;
            movedBack = true;
        }
    }
    if (!movedBack && nextDist <= slice)
    {
        sint32 distance = (slice - nextDist);
        if (distance <= 0)
            distance = (nextDist - slice);
        if (distance < 200)
            ReplayCamera[player].cutToNextCamera = nextIndex;
    }
    if (ReplayCamera[player].cutToNextCamera == currIndex)
        return;
    if ((((GameSetup::gData.raceType == 1 || GameSetup::gData.raceType == 5) && ((Cars::gHumanRaceCarList[0]->carFlags & 0x200) != 0 || (Cars::gNumHumanRaceCars == 2 && (Cars::gHumanRaceCarList[1]->carFlags & 0x200) != 0))) || Cars::gHumanRaceCarList[player]->wrongway != 0) && (uint8)gReplayCameraSlots[ReplayCamera[player].cutToNextCamera].mode == 11)
    {
        sint32 index = ReplayCamera[player].cutToNextCamera;
        ReplayCamera[player].defaultCamera = 0;
        Camera::gInfo[player].zooming = gReplayCameraSlots[index].zoom;
        Camera::gInfo[player].splineMode = gReplayCameraSlots[ReplayCamera[player].cutToNextCamera].splineMode;
        Camera::SetMode(player, 11);
    }
    else
    {
        sint32 index = ReplayCamera[player].cutToNextCamera;
        ReplayCamera[player].defaultCamera = 1;
        Camera::ReplayUpdate(player, &gReplayCameraSlots[index]);
    }
}

void Replay::ReplayFindClosestCamera(sint32 player, sint32 slice)
{
    FUNCTION_MARKER(0x800B5B14, "NFS4.EXE");
    sint32 cameraIndex = 0;
    sint32 numSlices = gNumSlices;
    sint32 count = numValidCams;
    sint32 half = numSlices / 2;
    for (sint32 i = 0; i < count; ++i)
    {
        sint32 next = i + 1 < count ? i + 1 : 0;
        sint32 currSlice = gReplayCameraSlots[i].slice;
        sint32 nextSlice = gReplayCameraSlots[next].slice;
        sint32 currDist = (slice - currSlice);
        if (currDist > half)
            currDist = (numSlices - currDist);
        if (currDist <= 0)
            currDist = (-currDist);
        sint32 nextDist = (slice - nextSlice);
        if (nextDist > half)
            nextDist = (numSlices - nextDist);
        if (nextDist <= 0)
            nextDist = (-nextDist);
        sint32 actualDist = (currSlice - nextSlice);
        if (actualDist > half)
            actualDist = (numSlices - actualDist);
        if (actualDist <= 0)
            actualDist = (-actualDist);
        if ((currDist + nextDist) <= actualDist)
        {
            cameraIndex = i;
            break;
        }
    }
    ReplayCamera[player].cutToNextCamera = cameraIndex;
    if ((uint8)gReplayCameraSlots[cameraIndex].mode == 11)
    {
        ReplayCamera[player].defaultCamera = 0;
        Camera::gInfo[player].zooming = gReplayCameraSlots[cameraIndex].zoom;
        Camera::gInfo[player].splineMode = gReplayCameraSlots[ReplayCamera[player].cutToNextCamera].splineMode;
        Camera::SetMode(player, 11);
    }
    else
    {
        sint32 index = ReplayCamera[player].cutToNextCamera;
        ReplayCamera[player].defaultCamera = 1;
        Camera::ReplayUpdate(player, &gReplayCameraSlots[index]);
    }
}
