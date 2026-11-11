#ifndef UDFF_H
#define UDFF_H

#include "xport.h"
#include "psx.h"
#include "game/common/car.h"

class Udff
{
public:
    enum tAccessType
    {
        UDFF_FILE = 0,
        UDFF_MEMORY = 1
    };

    struct tInfo
    {
        tAccessType type;
        void *handle;
        char *memPtr;
    };

    typedef tInfo *tHandle;

    static tInfo * Opena(char *name, char *mem, sint32 abortFlag);
    static void Close(tInfo *handle);
    static sint32 GetInt(tInfo *handle);
    static void GetBuffer(tInfo *handle, char *mem, sint32 size);
};

#endif
