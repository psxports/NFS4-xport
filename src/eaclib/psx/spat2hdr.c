#include "xport.h"
#include "eaclib/psx/internal.h"
#include "spat2hdr.h"

sint32 SNDattributessetdef(SoundAttributesPrefix *attributes)
{
    FUNCTION_MARKER(0x801035B0, "NFS4.EXE");
    attributes->unknown0 = -1;
    attributes->volume = 127;
    attributes->pan = 64;
    attributes->unknown6 = 0;
    attributes->unknown4 = 0;
    attributes->unknown9 = 0;
    attributes->unknown10 = 0;
    attributes->unknown11 = 0;
    return 0;
}

sint32 iSNDpatchtohdr(uint8 *patch, SNDSAMPLEFORMAT *format, SoundAttributesPrefix *attributes, uint32 *frames)
{
    FUNCTION_MARKER(0x801035E4, "NFS4.EXE");
    SNDattributessetdef(attributes);
    uint8 *cursor = patch + ((patch[3] & 2) != 0 ? 8 : 4);
    format->samplerate = 0x5622;
    format->channels = 1;
    format->samplerep = 5;
    *frames = 0;
    uint32 tag;
    uint32 value;
    uint8 *payload;
    while (iSNDgettag(&cursor, &tag, &value, &payload) != 0)
    {
        switch (tag)
        {
        case 0x80:
            attributes->unknown11 = (uint8)value;
            break;
        case 0x82:
            format->channels = (uint8)value;
            break;
        case 0x84:
            format->samplerate = (uint16)value;
            break;
        case 0x85:
            *frames = value;
            break;
        case 0x13:
            attributes->unknown9 = (uint8)value;
            break;
        case 10:
            attributes->unknown10 = (uint8)value;
            break;
        case 5:
            attributes->unknown0 = (sint32)value;
            break;
        case 6:
            attributes->unknown6 = (uint8)value;
            break;
        }
    }
    return 0;
}
