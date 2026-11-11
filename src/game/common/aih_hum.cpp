#include <stddef.h>
#include "game/common/aihigh.h"
#include "game/common/aistate.h"
#include "game/common/car.h"
#include "game/common/carobj.h"

void AIHigh_Human::HighExecute(void)
{
    FUNCTION_MARKER(0x80063270, "NFS4.EXE");
    HandleCops();
    carObj_->N.active = 1;
    state_->StateExecute();
}

AIHigh_Human::AIHigh_Human(Car::tObj *carObj) : AIHigh_Player(carObj)
{
    FUNCTION_MARKER(0x800632AC, "NFS4.EXE");
}
