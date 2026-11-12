#include "xport.h"
#include "eaclib/psx/internal.h"

extern "C" sint32 GetFilterLength(void)
{
    FUNCTION_MARKER(0x800E6FE4, "NFS4.EXE");
    const SPCHEventDataHeader *header = (const SPCHEventDataHeader *)gEventDats[0];
    return header->filterLength;
}

extern "C" sint32 GetFilterPriority(void)
{
    FUNCTION_MARKER(0x800E6FFC, "NFS4.EXE");
    const SPCHEventDataHeader *header = (const SPCHEventDataHeader *)gEventDats[0];
    return header->filterPriority;
}

SPCHEventDefinitionPrefix *iSPCH::SearchEventDat(uint8 *data, uint32 id)
{
    FUNCTION_MARKER(0x800E6EC4, "NFS4.EXE");
    const uint16 *words = (const uint16 *)data;
    sint32 count = words[1];
    const uint16 *offsets = words + 6;
    for (sint32 i = 0; i < count; ++i)
    {
        SPCHEventDefinitionPrefix *definition = (SPCHEventDefinitionPrefix *)(data + (uint32)offsets[i] * 4);
        if (definition->id == id)
            return definition;
    }
    return 0;
}

SPCHEventDefinitionPrefix *iSPCH::FindEvent(uint32 id)
{
    FUNCTION_MARKER(0x800E6F4C, "NFS4.EXE");
    for (sint32 i = 0; i < 4; ++i)
    {
        uint8 *data = gEventDats[i];
        if (data != 0)
        {
            SPCHEventDefinitionPrefix *definition = SearchEventDat(data, id);
            if (definition != 0)
                return definition;
        }
    }
    return 0;
}

void iSPCH::InitEventDat(void)
{
    FUNCTION_MARKER(0x800E6FBC, "NFS4.EXE");
    for (sint32 i = 3; i >= 0; --i)
        gEventDats[i] = 0;
}

void iSPCH::InitEventQueue(void)
{
    FUNCTION_MARKER(0x800E7014, "NFS4.EXE");
    gVoxEvents.count = 0;
    gVoxEvents.unknown4 = 0;
    for (sint32 i = 0; i < 16; ++i)
    {
        SPCHEventSlot *slot = &gVoxEvents.slots[i];
        slot->active = 0;
        slot->subTick = 0;
        slot->tick = 0;
        slot->definition = 0;
        for (sint32 j = 0; j < 12; ++j)
            slot->parameters[j] = 0;
    }
    gLastTick = 0;
    gLastSubTick = 0;
}

sint32 iSPCH::FindEventSlot(uint32 priority)
{
    FUNCTION_MARKER(0x800E7088, "NFS4.EXE");
    if (gVoxEvents.count < 16)
    {
        for (sint32 i = 0; i < 16; ++i)
        {
            if (gVoxEvents.slots[i].active == 0)
                return i;
        }
    }
    sint32 tick = gettick();
    for (sint32 i = 0; i < 16; ++i)
    {
        SPCHEventSlot *slot = &gVoxEvents.slots[i];
        uint32 age = slot->definition->maxAge;
        if (age != 0 && age < tick - (uint32)slot->tick)
        {
            slot->active = 0;
            gVoxEvents.count = (gVoxEvents.count - 1);
            return i;
        }
    }
    for (sint32 i = 0; i < 16; ++i)
    {
        SPCHEventSlot *slot = &gVoxEvents.slots[i];
        if (priority >= slot->definition->priority)
        {
            slot->active = 0;
            gVoxEvents.count = (gVoxEvents.count - 1);
            return i;
        }
    }
    return -1;
}

sint32 SPCH::AddEvent(sint32 *parameters)
{
    FUNCTION_MARKER(0x800E71B8, "NFS4.EXE");
    SPCHEventDefinitionPrefix *definition = iSPCH::FindEvent((uint32)parameters[0]);
    if (definition == 0)
        return 0;
    sint32 probability = definition->probability;
    if (probability < iSPCH::Rand(100))
        return 0;
    sint32 index = iSPCH::FindEventSlot(definition->priority);
    if (index < 0)
        return 0;
    sint32 tick = gettick();
    if (tick == gLastTick)
        gLastSubTick = (uint16)(gLastSubTick + 1);
    else
        gLastSubTick = 0;
    gLastTick = tick;
    SPCHEventSlot *slot = &gVoxEvents.slots[index];
    slot->definition = definition;
    slot->tick = tick;
    slot->subTick = gLastSubTick;
    for (sint32 i = 0; i < 12; ++i)
        slot->parameters[i] = parameters[i];
    gVoxEvents.count = (gVoxEvents.count + 1);
    slot->active = 1;
    return 0;
}

sint32 iSPCH::ChooseEvent(void)
{
    FUNCTION_MARKER(0x800E7300, "NFS4.EXE");
    uint32 tick = gettick() + (uint32)gPreLoadTicks;
    sint32 selected = -1;
    sint32 bestPriority = 0;
    uint32 bestAge = 0xFFFFFFFF;
    uint16 bestSubTick = 0;
    for (sint32 i = 0; i < 16; ++i)
    {
        SPCHEventSlot *slot = &gVoxEvents.slots[i];
        if (slot->active != 0)
        {
            SPCHEventDefinitionPrefix *definition = slot->definition;
            uint32 age = tick - slot->tick;
            uint32 maxAge = definition->maxAge;
            bool expired = maxAge != 0 && maxAge < age;
            bool filtered = false;
            if (gFilterSetting == 1 && (definition->flags & 1) != 0)
                filtered = definition->priority < GetFilterPriority();
            if (expired || filtered)
            {
                slot->active = 0;
                gVoxEvents.count = (gVoxEvents.count - 1);
            }
            else
            {
                sint32 priority = definition->priority;
                if (priority > bestPriority || (priority == bestPriority && (age < bestAge || (age == bestAge && slot->subTick > bestSubTick))))
                {
                    selected = i;
                    bestPriority = priority;
                    bestAge = age;
                    bestSubTick = slot->subTick;
                }
            }
        }
    }
    return selected;
}

void SPCH::ClearEventQueue(void)
{
    FUNCTION_MARKER(0x800E74E0, "NFS4.EXE");
    for (sint32 i = 0; i < 16; ++i)
    {
        SPCHEventSlot *slot = &gVoxEvents.slots[i];
        if (slot->active != 0)
        {
            slot->active = 0;
            gVoxEvents.count = (gVoxEvents.count - 1);
        }
    }
}

void iSPCH::ClearOldEvents(sint32 selected)
{
    FUNCTION_MARKER(0x800E7528, "NFS4.EXE");
    uint32 tick = gVoxEvents.slots[selected].tick;
    uint16 subTick = gVoxEvents.slots[selected].subTick;
    gVoxEvents.unknown4 = 0;
    for (sint32 i = 0; i < 16; ++i)
    {
        SPCHEventSlot *slot = &gVoxEvents.slots[i];
        if (i != selected && slot->active != 0)
        {
            uint32 otherTick = slot->tick;
            if (otherTick < tick || (otherTick == tick && slot->subTick < subTick))
            {
                if ((slot->definition->flags & 4) == 0)
                {
                    slot->active = 0;
                    gVoxEvents.count = (gVoxEvents.count - 1);
                }
            }
            else if (slot->definition->probability == 100)
                gVoxEvents.unknown4 = 1;
        }
    }
}

void SPCH::PlaySpeech(void)
{
    FUNCTION_MARKER(0x800E7644, "NFS4.EXE");
    if (iSPCH::OneChosen() != 0 || ChooseSpeech() != 0)
        iSPCH::PlayChosen();
}

sint32 SPCH::ChooseSpeech(void)
{
    FUNCTION_MARKER(0x800E7684, "NFS4.EXE");
    sint32 result = 0;
    if (gVoxEvents.count != 0)
    {
        sint32 selected = iSPCH::ChooseEvent();
        if (selected >= 0)
        {
            iSPCH::ClearOldEvents(selected);
            sint32 *parameters = gVoxEvents.slots[selected].parameters;
            result = iSPCH::ChooseSentence(parameters);
            if (result == 0 && gReparm != 0)
            {
                sint32 attempt = 0;
                sint32 status;
                do
                {
                    status = gReparm(attempt, parameters);
                    if (status >= 0)
                        result = iSPCH::ChooseSentence(parameters);
                    attempt = (attempt + 1);
                } while (result == 0 && status > 0);
            }
            if (result < 0)
                result = 0;
            gVoxEvents.count = (gVoxEvents.count - 1);
            gVoxEvents.slots[selected].active = 0;
        }
    }
    return result;
}
