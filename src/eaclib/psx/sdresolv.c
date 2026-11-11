#include "xport.h"
#include "eaclib/psx/internal.h"

sint32 iSNDplatformresolve(uint8 *tags, const uint8 *sampleData, void *sampleTable)
{
    FUNCTION_MARKER(0x8010B75C, "NFS4.EXE");
    uint8 *cursor = tags;
    uint8 *sample = 0;
    uint32 sourceOffset = 0;
    sint32 frames = 0;
    uint32 channels = 1;
    uint32 tag;
    uint32 value;
    uint8 *payload;
    while (iSNDgettag(&cursor, &tag, &value, &payload) != 0 && tag != 0xFE)
    {
        if (tag == 0x88)
            sourceOffset = value;
        else if (tag == 0x8A)
            sample = payload;
        else if (tag == 0x82)
            channels = value;
        else if (tag == 0x85)
            frames = (sint32)value;
    }
    if (sampleData == 0)
        return 7;
    uint8 *entry = ((uint8 *)(sampleTable));
    uint32 key;
    memcpy(&key, entry, sizeof(key));
    while (key != 0xFFFFFFFF)
    {
        memcpy(&key, entry, sizeof(key));
        if (key == sourceOffset)
        {
            uint32 address;
            memcpy(&address, entry + 4, sizeof(address));
            memcpy(sample, &address, sizeof(address));
            break;
        }
        entry += 8;
        memcpy(&key, entry, sizeof(key));
    }
    uint32 resolved;
    memcpy(&resolved, sample, sizeof(resolved));
    if (resolved != 0)
        return 7;
    sint32 blocks = frames / 28;
    if (frames != blocks * 28)
        ++blocks;
    sint32 size = (sint32)(((uint32)(blocks)) * (channels << 4));
    sint32 address = iSNDpsxmalloc(size);
    if (address == 0)
        return -6;
    uint32 request = iSNDdmqueuesplit(sampleData + sourceOffset, address, size, 1);
    memcpy(sample, &address, sizeof(address));
    memcpy(entry, &sourceOffset, sizeof(sourceOffset));
    memcpy(entry + 4, &address, sizeof(address));
    while (iSNDdmcomplete(request) == 0)
    {
    }
    return 7;
}

sint32 iSNDplatformremove(uint8 *tags, void *sampleTable)
{
    FUNCTION_MARKER(0x8010B958, "NFS4.EXE");
    uint8 *cursor = tags;
    uint8 *sample = 0;
    uint32 tag;
    uint32 value;
    uint8 *payload;
    while (iSNDgettag(&cursor, &tag, &value, &payload) != 0 && tag != 0xFE)
    {
        if (tag == 0x8A)
            sample = payload;
    }
    uint8 *entry = ((uint8 *)(sampleTable));
    uint32 key;
    memcpy(&key, entry, sizeof(key));
    if (key != 0xFFFFFFFF)
    {
        uint32 address;
        memcpy(&address, sample, sizeof(address));
        do
        {
            if (key == address)
                return 0;
            entry += 8;
            memcpy(&key, entry, sizeof(key));
        } while (key != 0xFFFFFFFF);
    }
    uint32 address;
    memcpy(&address, sample, sizeof(address));
    memcpy(entry, &address, sizeof(address));
    memcpy(&address, sample, sizeof(address));
    if (address != 0)
        iSNDpsxfree((sint32)address);
    return 0;
}
