#include "xport.h"
#include "eaclib/psx/internal.h"

charactertbl *textbsearch(sint32 key, char *base, uint32 nmemb, uint32 size)
{
    FUNCTION_MARKER(0x800F4470, "NFS4.EXE");
    sint32 lim = (sint32)nmemb;
    while (lim != 0)
    {
        uint32 offset = (uint32)(lim >> 1) * size;
        char *probe = base + (sint32)offset;
        sint32 cmp = (sint32)((uint32)key - geti(probe, 2));
        if (cmp == 0)
            return ((charactertbl *)(probe));
        if (cmp > 0)
        {
            base = probe + size;
            lim = (sint32)((uint32)lim - 1);
        }
        lim >>= 1;
    }
    return 0;
}

charactertbl *getcharacter(sint32 code)
{
    FUNCTION_MARKER(0x800F4510, "NFS4.EXE");
    char *base = currentfont.characters;
    uint32 count = currentfont.characterCount;
    uint32 index = (uint32)code - 32U;
    if (index < count) {
        charactertbl *ch = ((charactertbl *)(base + index * 11U));
        if (geti(ch, 2) == (uint32)code)
            return ch;
    }
    return textbsearch(code, base, count, 11);
}
