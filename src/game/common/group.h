#ifndef GROUP_H
#define GROUP_H

#include "xport.h"
#include "game/common/simplemem.h"

struct Group
{
    sint32 m_num_elements;
};

class SerializedGroup
{
public:
    sint32 m_type;
    sint32 m_length;
    sint32 dummy;
    sint32 m_num_elements;

    SerializedGroup *LocateNextGroupType(sint32 type);
    SerializedGroup *LocateGroupType(sint32 type, sint32 index);
    SerializedGroup *LocateGroupNum(sint32 index);
    Group *LocateCreateGroupType(sint32 type, SimpleMem *mem, sint32 index);
    Group *CreateLiteGroup(SerializedGroup *source, SimpleMem *mem);
    Group *CreateLiteGroupDataSize(SerializedGroup *source, SimpleMem *mem, sint32 dataSize);
};

#endif
