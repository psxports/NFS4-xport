#include "xport.h"
#include "eaclib/psx/internal.h"
#include "spchdata.h"

GameTimerCallback gameExitCallbacks[32];
SPCHEventQueue gVoxEvents;
uint16 gLastSubTick;
sint32 gLastTick;
sint32 gPreLoadTicks;
sint32 gFilterSetting;
SpeechReparmFuncPtr gReparm;
SPCHSentenceChoice gSentenceChoice;
SPCHInGameState gVoxInGame;
uint8 **gVoxBanks;
uint32 gClearCycle = 1;
sint32 gDataRate;
SPCHChoiceState gSPCHChoiceState;
uint8 *gEventDats[4];
uint8 * const ispch_gPickSamples = reinterpret_cast<uint8 *>(&gSPCHChoiceState);
uint32 &gChooseShort = gSPCHChoiceState.chooseShort;
SPCHSampleChoice (&ispch_gChoice)[12] = gSPCHChoiceState.choices;
SpeechSampleRequestFuncPtr gSampleRequest;
SpeechRuleSetFuncPtr gSentenceRuleSet;
SpeechRuleTestFuncPtr gSentenceRuleTest;
sint32 gNumBanks;
sint32 gGameNum;
uint32 gSPCH_Initialized;
MemAllocFuncPtr gMemAlloc;
MemFreeFuncPtr gMemFree;
