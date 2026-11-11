#include <stddef.h>
#include "xport.h"
#include "game/common/group.h"
#include "game/common/simplemem.h"
#include "lib/psx/blkmov.h"

SerializedGroup *SerializedGroup::LocateNextGroupType(sint32 type)
{
    FUNCTION_MARKER(0x8009C128, "NFS4.EXE");
    SerializedGroup *group = reinterpret_cast<SerializedGroup *>(reinterpret_cast<uint8 *>(this) + m_length);
    return group->m_type == type ? group : 0;
}

SerializedGroup *SerializedGroup::LocateGroupType(sint32 type, sint32 index)
{
    FUNCTION_MARKER(0x8009C150, "NFS4.EXE");
    sint32 count = 0;
    sint32 numElems = m_num_elements;
    SerializedGroup *group = this + 1;
    for (;;)
    {
        numElems = (sint32)((uint32)numElems - 1U);
        if (numElems == -1)
            return 0;
        if (group->m_type == type)
        {
            bool found = count == index;
            count = (sint32)((uint32)count + 1U);
            if (found)
                return group;
        }
        uint32 remainder = (uint32)group->m_length & 3U;
        if (remainder != 0)
            group->m_length = (sint32)((uint32)group->m_length + 4U - remainder);
        group = reinterpret_cast<SerializedGroup *>(reinterpret_cast<uint8 *>(group) + group->m_length);
    }
}

SerializedGroup *SerializedGroup::LocateGroupNum(sint32 index)
{
    FUNCTION_MARKER(0x8009C1C0, "NFS4.EXE");
    sint32 numElems = m_num_elements;
    SerializedGroup *group = this + 1;
    if (index >= numElems)
        return 0;
    for (sint32 i = 0; i < index; ++i)
        group = reinterpret_cast<SerializedGroup *>(reinterpret_cast<uint8 *>(group) + group->m_length);
    return group;
}

Group *SerializedGroup::LocateCreateGroupType(sint32 type, SimpleMem *mem, sint32 index)
{
    FUNCTION_MARKER(0x8009C204, "NFS4.EXE");
    if (LocateGroupType(type, index) == 0)
        return 0;
    SerializedGroup *source = LocateGroupType(type, index);
    return CreateLiteGroup(source, mem);
}

Group *SerializedGroup::CreateLiteGroup(SerializedGroup *source, SimpleMem *mem)
{
    FUNCTION_MARKER(0x8009C27C, "NFS4.EXE");
    sint32 newLen = (sint32)((uint32)source->m_length - 12U);
    Group *ret = static_cast<Group *>(mem->Alloc(newLen, 0));
    ret->m_num_elements = source->m_num_elements;

    blockmove(source + 1, ret + 1, newLen);
    return ret;
}

Group *SerializedGroup::CreateLiteGroupDataSize(SerializedGroup *source, SimpleMem *mem, sint32 dataSize)
{
    FUNCTION_MARKER(0x8009C2E4, "NFS4.EXE");
    sint32 newLen = (sint32)((uint32)dataSize + 4U);
    Group *ret = static_cast<Group *>(mem->Alloc(newLen, 0));
    ret->m_num_elements = 0;

    blockmove(source + 1, ret + 1, newLen);
    return ret;
}
