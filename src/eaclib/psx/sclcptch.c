#include "xport.h"
#include "eaclib/psx/internal.h"

uint8 sndcents[256] =
{
    0, 1, 2, 2, 3, 4, 4, 5, 6, 7, 7, 8, 9, 9, 10, 11,
    12, 12, 13, 14, 14, 15, 16, 17, 17, 18, 19, 20, 20, 21, 22, 23,
    23, 24, 25, 26, 26, 27, 28, 29, 30, 30, 31, 32, 33, 33, 34, 35,
    36, 37, 37, 38, 39, 40, 41, 41, 42, 43, 44, 45, 45, 46, 47, 48,
    49, 50, 50, 51, 52, 53, 54, 55, 55, 56, 57, 58, 59, 60, 61, 61,
    62, 63, 64, 65, 66, 67, 67, 68, 69, 70, 71, 72, 73, 74, 75, 75,
    76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 86, 87, 88, 89, 90,
    91, 92, 93, 94, 95, 96, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106,
    107, 108, 108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 119, 120, 121, 122,
    123, 124, 125, 126, 127, 128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138,
    139, 140, 142, 143, 144, 145, 146, 147, 148, 149, 150, 151, 152, 154, 155, 156,
    157, 158, 159, 160, 161, 163, 164, 165, 166, 167, 168, 169, 171, 172, 173, 174,
    175, 176, 178, 179, 180, 181, 182, 183, 185, 186, 187, 188, 189, 191, 192, 193,
    194, 196, 197, 198, 199, 200, 202, 203, 204, 205, 207, 208, 209, 210, 212, 213,
    214, 216, 217, 218, 219, 221, 222, 223, 225, 226, 227, 229, 230, 231, 232, 234,
    235, 236, 238, 239, 240, 242, 243, 245, 246, 247, 249, 250, 251, 253, 254, 255
};

sint32 iSNDdetunetolinear(sint32 detune)
{
    FUNCTION_MARKER(0x80101900, "NFS4.EXE");
    uint32 scale = 0x1000;
    while (detune >= 1200)
    {
        detune -= 1200;
        scale <<= 1;
    }
    while (detune < -1199)
    {
        scale = (((sint32)scale >> 1));
        detune += 1200;
    }
    sint32 index = (detune * 0x369D) >> 16;
    if (index < -255)
        index = -255;
    if (index < 0)
        return (sint32)(scale * (sndcents[index + 256] + 256)) >> 9;
    return (sint32)(scale * (sndcents[index] + 256)) >> 8;
}

void iSNDcalcpitch(sint32 channel)
{
    FUNCTION_MARKER(0x801019BC, "NFS4.EXE");
    SoundVoicePrefix *voice = &((SoundVoicePrefix *)(sndgs.voiceMemory))[channel];
    if (voice->pitchAdjustment == 0)
    {
        sint32 detune = (sint16)voice->basePitch;
        if (voice->unknown5A != 0)
        {
            sint32 bend = (sint8)voice->unknown2F;
            if (voice->unknown4C != 0)
                bend = voice->unknown4C[bend];
            detune += ((bend - 64) * voice->unknown5A) >> 6;
        }
        if (voice->unknown54 != 0)
        {
            sint32 envelope = voice->unknown54[voice->pitchEnvelopeIndex];
            detune += ((envelope - 64) * voice->unknown58) >> 6;
        }
        voice->pitchAdjustment = (uint16)iSNDdetunetolinear(detune);
    }
    uint32 product = ((uint32)(voice->pitchAdjustment)) * voice->pitchMultiplier;
    voice->pitch = (uint16)(product >> 12);
}
