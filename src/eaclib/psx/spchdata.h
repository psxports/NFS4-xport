#ifndef SPCHDATA_H
#define SPCHDATA_H

#include "xport.h"
#include "eaclib/psx/callback.h"

struct SPCHEventDefinitionPrefix
{
    uint16 id;
    uint16 maxAge;
    uint16 priority;
    uint8 unknown6[3];
    sint8 probability;
    uint8 flags;
};

struct SPCHEventDataHeader
{
    uint16 version;
    uint16 eventCount;
    sint32 filterLength;
    sint32 filterPriority;
};

struct SPCHEventSlot
{
    uint16 active;
    uint16 subTick;
    sint32 tick;
    SPCHEventDefinitionPrefix *definition;
    sint32 parameters[12];
};

struct SPCHEventQueue
{
    sint32 count;
    sint32 unknown4;
    SPCHEventSlot slots[16];
};

struct SPCHSentenceChoice
{
    SPCHEventDefinitionPrefix *definition;
    uint8 *sentence;
    uint32 index;
    sint32 valid;
    sint32 parameters[12];
};

struct SPCHInGameState
{
    sint32 event;
    sint32 count;
};

extern SPCHSentenceChoice gSentenceChoice;
extern SPCHInGameState gVoxInGame;
extern SPCHEventQueue gVoxEvents;
extern uint16 gLastSubTick;
extern sint32 gLastTick;
extern sint32 gPreLoadTicks;
extern sint32 gFilterSetting;

typedef sint32 (*SpeechReparmFuncPtr)(sint32 attempt, sint32 *parameters);

extern SpeechReparmFuncPtr gReparm;
extern uint8 *gEventDats[4];

struct SPCHSample
{
    sint32 size;
    sint32 offset;
    sint32 ruleCount;
    uint8 rules[4];
};

struct SPCHSampleChoice
{
    sint16 bank;
    sint16 offsetMultiplier;
    sint16 unknown4;
    sint16 unknown6;
    sint16 pickIndex;
    sint16 unknown10;
};

extern uint8 **gVoxBanks;
extern sint32 gNumBanks;
extern uint32 gClearCycle;
extern sint32 gDataRate;
extern sint32 gGameNum;
extern uint32 gSPCH_Initialized;
extern MemAllocFuncPtr gMemAlloc;
extern MemFreeFuncPtr gMemFree;
extern uint32 seedX[6];

struct SPCHChoiceState
{
    uint8 pickSamples[100];
    uint32 chooseShort;
    SPCHSampleChoice choices[12];
};

extern SPCHChoiceState gSPCHChoiceState;
extern uint8 * const ispch_gPickSamples;
extern uint32 &gChooseShort;
extern SPCHSampleChoice (&ispch_gChoice)[12];

typedef sint32 (*SpeechSampleRequestFuncPtr)(sint32 bank, sint32 offset, sint32 size, sint32 event);

typedef void (*SpeechRuleSetFuncPtr)(uint32 event, uint32 rule, uint32 value);

typedef sint32 (*SpeechRuleTestFuncPtr)(uint32 event, uint32 rule, sint32 value);

extern SpeechSampleRequestFuncPtr gSampleRequest;
extern SpeechRuleSetFuncPtr gSentenceRuleSet;
extern SpeechRuleTestFuncPtr gSentenceRuleTest;

#endif
