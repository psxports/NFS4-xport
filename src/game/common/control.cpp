#include <stddef.h>
#include "xport.h"
#include "game/common/ai.h"
#include "game/common/car.h"
#include "game/common/carobj.h"
#include "game/common/cars.h"
#include "game/common/control.h"
#include "game/common/gmesetup.h"
#include "game/common/input.h"
#include "game/common/r3dcar.h"
#include "game/common/sim.h"
#include "game/psx/hud.h"
#include "game/psx/psxcontroller.h"

typedef sint16 (*tPA32)[32];

void Control::AI(Car::tObj *carObj)
{
    FUNCTION_MARKER(0x80091FDC, "NFS4.EXE");
    (void)carObj;
}

void Control::Human(Car::tObj *carObj)
{
    FUNCTION_MARKER(0x80091FE4, "NFS4.EXE");
    if (((uint32)simGlobal.gameTicks & 3) != 0)
        return;
    InGame::SetRamp();
    carObj->funcReplay(carObj);
    if (carObj->RSControl != 0) {
        carObj->control.desiredGasLevel = carObj->RSGasLevel & 0xf8;
        carObj->control.desiredSteering = (carObj->RSSteering / 4) * 4;
        carObj->control.desiredBrakeLevel = 0;
        carObj->control.handBrake = 0;
        carObj->control.horn = 0;
        carObj->control.abort = 0;
        carObj->control.event = 0;
    } else {
        carObj->control.desiredGasLevel = Input::gSim.gas & 0xf8;
        carObj->control.desiredBrakeLevel = Input::gSim.brake & 0xf8;
        sint32 steering = ((sint8)Input::gSim.steering / 4) * 4;
        if (GameSetup::gData.mirrorTrack == 1)
            steering = -steering;
        carObj->control.desiredSteering = steering;
        carObj->control.handBrake = Input::gSim.flags & 1;
        carObj->control.horn = (Input::gSim.flags >> 1) & 1;
        carObj->control.abort = (Input::gSim.flags >> 3) == 3;
        carObj->control.event = Input::gSim.flags >> 3;
    }
    sint32 event = (uint8)carObj->control.event;
    switch (event) {
    case 4:
    case 5:
    case 6:
        if (HudBustedOverlay == 0) {
            sint32 player = carObj->carIndex;
            char queued = carObj->control.event;
            carObj->control.queuedEvent = queued;
            Hud::WingmanFlash(player, event - 4);
        }
        break;
    case 7:
        if ((uint32)carObj->carInfo->carType - 22 >= 6) {
            uint8 lights = carObj->control.lights;
            if ((lights & 8) == 0) {
                uint16 signal = carObj->render.signalLight[0];
                carObj->control.lights = (char)((lights | 8) & 0xef);
                carObj->render.signalLight[1] = 0;
                carObj->render.signalLight[0] = (sint16)(signal | 0x80);
            } else {
                carObj->control.lights = lights & 0xe7;
                carObj->render.signalLight[0] = 0;
                carObj->render.signalLight[1] = 0;
            }
        }
        break;
    case 8:
        if ((uint32)carObj->carInfo->carType - 22 >= 6) {
            uint8 lights = carObj->control.lights;
            if ((lights & 0x10) != 0) {
                carObj->control.lights = lights & 0xe7;
                carObj->render.signalLight[1] = 0;
                carObj->render.signalLight[0] = 0;
            } else {
                uint16 signal = carObj->render.signalLight[1];
                carObj->control.lights = (char)((lights | 0x10) & 0xf7);
                carObj->render.signalLight[0] = 0;
                carObj->render.signalLight[1] = (sint16)(signal | 0x80);
            }
        }
        break;
    case 11:
        if ((uint32)carObj->carInfo->carType - 22 < 6) {
            carObj->AIFlags ^= 2;
        } else {
            uint8 lights = carObj->control.lights;
            if ((lights & 0x18) == 0) {
                carObj->control.lights = lights | 0x18;
                uint16 left = carObj->render.signalLight[0];
                uint16 right = carObj->render.signalLight[1];
                carObj->render.signalLight[0] = (sint16)(left | 0x80);
                carObj->render.signalLight[1] = (sint16)(right | 0x80);
            } else {
                carObj->control.lights = lights & 0xe7;
                carObj->render.signalLight[0] = 0;
                carObj->render.signalLight[1] = 0;
            }
        }
        break;
    case 12:
        if ((carObj->AIFlags & 2) == 0) {
            uint8 lights = carObj->control.lights ^ 3;
            carObj->control.lights = lights;
            if ((lights & 2) != 0)
                R3DCar::TurnHeadLightOn(carObj, 1);
            else
                R3DCar::TurnHeadLightOff(carObj, (lights & 4) == 0);
        }
        break;
    case 14:
        {
            uint8 lights = carObj->control.lights ^ 4;
            carObj->control.lights = lights;
            if ((lights & 2) == 0 && (carObj->AIFlags & 2) == 0) {
                if ((lights & 4) != 0)
                    R3DCar::TurnHeadLightOn(carObj, 0);
                else
                    R3DCar::TurnHeadLightOff(carObj, 1);
            }
        }
        break;
    default:
        break;
    }
    sint32 newGear = (uint8)Input::Gear(carObj->control.gear, carObj->specs->numGears);
    if (carObj->flywheelRpm <= carObj->specs->redline || (uint8)carObj->control.gear <= newGear)
        carObj->control.desiredGear = (char)newGear;
}
