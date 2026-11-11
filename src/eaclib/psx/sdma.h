#ifndef SDMA_H
#define SDMA_H

#include "xport.h"

typedef struct SoundDmaRequest SoundDmaRequest;
typedef struct SoundPlatformChannel SoundPlatformChannel;
typedef struct SoundPlatformPacketBufferPrefix SoundPlatformPacketBufferPrefix;
typedef struct SoundDmaState SoundDmaState;

struct SoundDmaRequest
{
    uint32 token;
    const uint8 *source;
    uint16 destinationUnits;
    uint8 lengthUnits;
    uint8 priority;
    uint8 notify;
    uint8 reserved;
    uint16 sourceBytes;
    uint32 deadline;
};

struct SoundPlatformChannel
{
    uint32 sampleAddress;
    uint32 sampleLength;
    sint32 pitch;
    uint32 unknown0C;
    uint32 unknown10;
    uint32 unknown14;
    uint16 pan;
    uint16 basePitch;
    uint8 state;
    uint8 unknown1D;
    uint8 unknown1E;
    uint8 channelCount;
    sint8 partner;
    uint8 unknown21;
    sint8 volume;
    sint8 effectLevel;
    uint8 unknown24[2];
    uint8 envelopeSeen;
    sint8 stream;
    uint8 pitchChanged;
    uint8 unknown29[3];
};

struct SoundPlatformPacketBufferPrefix
{
    sint32 spuAddress;
    sint32 bufferSize;
    uint32 rightOffset;
    uint32 unknown0C;
    uint32 refillThreshold;
    uint32 writtenFrames;
    uint32 playedFrames;
    uint32 lastPosition;
    uint32 dmaRequest;
    const uint8 *packetData[2];
    uint8 *data;
    uint16 unknown30;
    uint16 unknown32;
    uint16 unknown34;
    uint16 unknown36;
    uint16 packetCount;
    uint16 packetCurrent;
    uint16 unknown3C;
    uint16 unknown3E;
    uint16 irqMargin;
    sint8 channel;
    uint8 shift;
    uint16 packetSize;
    uint16 framesPerPacket;
    uint8 *work[2];
};

struct SoundDmaState
{
    uint8 unknown0[4];
    sint32 fxMode;
    uint32 generation;
    sint8 queued;
    sint8 current;
    uint8 active;
    uint8 completed;
    SoundDmaRequest requests[10];
    SoundPlatformChannel channels[24];
    SoundPlatformPacketBufferPrefix *packetBuffers[1];
    uint8 unknown4FC[548];
    void (*serveCallback)(void);
    void (*stopCallback)(SoundPlatformChannel *channel);
    void (*irqCallback)(void);
};

#ifdef __cplusplus
extern "C" {
#endif
extern SoundDmaState sndpd;
#ifdef __cplusplus
}
#endif

#endif
