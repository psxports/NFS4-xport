#include "xport.h"
#include "eaclib/psx/eaclib.h"
#include "game/psx/unpack.h"
#include "lib/psx/getm.h"


    sint32 unpackz(void *src, void *dst)
    {
        FUNCTION_MARKER(0x800E3E68, "NFS4.EXE");
        const uint8 *data = ((const uint8 *)(src));
        uint32 signature = data[1];
        if (signature != 0xFB && signature != 0x32)
            return 0;
        uint32 type = data[0] & 0xFEu;
        uint8 *destination = ((uint8 *)(dst));
        if (type == 0x10)
            return unrefpack(data, destination, 1);
        if (type == 0x30 || type == 0x32 || type == 0x34)
            return (sint32)unhuff(data, destination, 1);
        if (type == 0x46)
            return unbtree(data, destination);
        return 0;
    }

    sint32 unpack(void *src, void *dst)
    {
        FUNCTION_MARKER(0x800E3F1C, "NFS4.EXE");
        return unpackz(src, dst);
    }

    sint32 unpacksizez(void *src)
    {
        FUNCTION_MARKER(0x800E3F3C, "NFS4.EXE");
        const uint8 *data = ((const uint8 *)(src));
        uint32 signature = data[1];
        if (signature != 0xFB && signature != 0x32)
            return 0;
        uint32 type = data[0] & 0xFEu;
        if (type == 0x10 || type == 0x18 || type == 0x30 || type == 0x32 || type == 0x34 || type == 0x46 || type == 0x4A)
            return getm(data + 2, 3);
        return 0;
    }

    sint32 unpacksize(void *src)
    {
        FUNCTION_MARKER(0x800E4004, "NFS4.EXE");
        return unpacksizez(src);
    }

