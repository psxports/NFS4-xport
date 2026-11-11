#include "xport.h"
#include "eaclib/psx/internal.h"

sint32 biglen;

sint32 typeofbigfile(const void *header)
{
    FUNCTION_MARKER(0x800E5F1C, "NFS4.EXE");
    if (getm(header, 2) == 0xC0FB)
        return 1;
    if (getm(header, 4) == 0x42494746)
        return 2;
    return 0;
}

sint32 sizeofbigfileheader(const void *header)
{
    FUNCTION_MARKER(0x800E5F84, "NFS4.EXE");
    sint32 kind = typeofbigfile(header);
    const uint8 *data = ((const uint8 *)(header));
    if (kind == 1)
        return (sint32)getm(data + 2, 2) + 4;
    if (kind == 2)
        return (sint32)getm(data + 12, 4);
    return 0;
}

char *locatebigentryz(char *header, const char *name, sint32 index, sint32 *offset, sint32 *size)
{
    FUNCTION_MARKER(0x800E5FFC, "NFS4.EXE");
    sint32 headerSize = sizeofbigfileheader(header);
    char *end = header + headerSize;
    sint32 kind = typeofbigfile(header);
    char *entry = 0;
    sint32 fieldSize = 0;
    sint32 nameOffset = 0;
    if (kind == 1)
    {
        entry = header + 6;
        fieldSize = 3;
        nameOffset = 6;
    }
    else if (kind == 2)
    {
        entry = header + 16;
        fieldSize = 4;
        nameOffset = 8;
    }
    if (kind == 1 || kind == 2)
    {
        sint32 ordinal = 0;
        while (entry < end)
        {
            bool matches;
            if (name == 0)
                matches = ordinal == index;
            else
            {
#if defined(_WIN32)
                matches = _strnicmp(entry + nameOffset, name, strlen(entry + nameOffset) + 1) == 0;
#else
                matches = strcasecmp(entry + nameOffset, name) == 0;
#endif
            }
            if (matches)
            {
                if (offset != 0)
                    *offset = getm(entry, fieldSize);
                if (size != 0)
                    *size = getm(entry + fieldSize, fieldSize);
                return entry + nameOffset;
            }
            entry += strlen(entry + nameOffset) + nameOffset + 1;
            ordinal = (sint32)((uint32)ordinal + 1);
        }
    }
    if (offset != 0)
        *offset = 0;
    if (size != 0)
        *size = 0;
    return 0;
}

char *locatebigentry(char *header, const char *name, sint32 index, sint32 *offset, sint32 *size)
{
    FUNCTION_MARKER(0x800E61B8, "NFS4.EXE");
    return locatebigentryz(header, name, index, offset, size);
}

sint32 locatebigoffset(char *header, const char *name)
{
    FUNCTION_MARKER(0x800E61DC, "NFS4.EXE");
    sint32 offset = 0;
    if (name != 0)
        locatebigentry(header, name, 0, &offset, &biglen);
    return offset;
}

char *locatebig(char *header, const char *name)
{
    FUNCTION_MARKER(0x800E6218, "NFS4.EXE");
    sint32 offset = locatebigoffset(header, name);
    if (offset != 0)
        return header + offset;
    return 0;
}

sint32 bigcount(const void *header)
{
    FUNCTION_MARKER(0x800E6258, "NFS4.EXE");
    sint32 kind = typeofbigfile(header);
    const uint8 *data = ((const uint8 *)(header));
    if (kind == 1)
        return (sint32)getm(data + 4, 2);
    if (kind == 2)
        return (sint32)getm(data + 8, 4);
    return 0;
}
