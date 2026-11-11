#include "xport.h"
#include "eaclib/psx/textcrnt.h"
#include "game/psx/font.h"
#include "game/psx/textpix.h"


    sint32 textnpixels(char *str, sint32 n)
    {
        FUNCTION_MARKER(0x800E3D8C, "NFS4.EXE");
        sint32 stringlen = 0;
        for (sint32 i = 0; i < n; ++i)
        {
            sint32 code = currentfont.decoder(&str);
            if (code == 0)
                break;
            charactertbl *ch = getcharacter(code);
            if (ch != 0)
            {
                sint32 dx = (sint8)ch->advance;
                stringlen = (sint32)((uint32)stringlen + dx);
            }
        }
        return stringlen;
    }

    sint32 textpixels(char *string)
    {
        FUNCTION_MARKER(0x800E3E20, "NFS4.EXE");
        return textnpixels(string, 256);
    }

