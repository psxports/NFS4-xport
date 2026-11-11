#ifndef AIH_BASICCOP_H
#define AIH_BASICCOP_H

#include "xport.h"
#include "game/common/coorddef.h"

class Blockade
{
public:
    static void AddRoadFlare(coorddef *pos);
    static void AddObject(sint32 slice, coorddef *pos, sint32 objectID);
};

#endif
