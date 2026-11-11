#ifndef SALLOC_H
#define SALLOC_H

#include "xport.h"

typedef struct SoundVoicePrefix SoundVoicePrefix;

struct SoundVoicePrefix
{
    sint32 playHandle;
    uint8 unknown04[6];
    sint8 unknown0A;
    sint8 state;
    uint8 priority;
    uint8 unknown0D[3];
    uint32 age;
    sint32 volumeRamp;
    sint32 volumeTarget;
    sint32 volume;
    sint32 modulationDelta;
    sint32 modulationVolume;
    sint32 modulationTicks;
    uint8 unknown2C;
    sint8 outputVolume;
    uint8 unknown2E;
    uint8 unknown2F;
    sint8 envelopeCount;
    sint8 envelopeIndex;
    uint8 unknown32;
    uint8 unknown33;
    sint8 effectScale;
    sint8 effectLevel;
    uint8 unknown36;
    uint8 unknown37;
    uint8 unknown38;
    uint8 unknown39;
    uint8 volumeEnvelopeIndex;
    uint8 pitchEnvelopeIndex;
    uint8 unknown3C;
    uint8 inputPan;
    uint8 unknown3E[2];
    void *envelope;
    const uint8 *unknown44;
    const sint8 *unknown48;
    const sint8 *unknown4C;
    const sint8 *unknown50;
    const sint8 *unknown54;
    sint16 unknown58;
    sint16 unknown5A;
    uint16 basePitch;
    uint16 pitchAdjustment;
    uint16 pitchMultiplier;
    uint16 pitch;
};

#ifdef __cplusplus
extern "C" {
#endif
extern uint32 soundChannelGeneration;
extern uint8 soundGroupKey;
extern uint8 sndcents[256];
extern sint8 sndchanreserved[4];
#ifdef __cplusplus
}
#endif

#endif
