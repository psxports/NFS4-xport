#ifndef SPCHINIT_H
#define SPCHINIT_H

#include "xport.h"
#include "callback.h"
#include "spchdata.h"

#ifdef __cplusplus
class iSPCH
{
public:
    static SPCHEventDefinitionPrefix *SearchEventDat(uint8 *data, uint32 id);
    static SPCHEventDefinitionPrefix *FindEvent(uint32 id);
    static void InitEventDat(void);
    static void InitEventQueue(void);
    static sint32 FindEventSlot(uint32 priority);
    static sint32 ChooseEvent(void);
    static void ClearOldEvents(sint32 selected);
    static void InitBanks(void);
    static void DisposeBanks(void);

    static void *BankMemAlloc(sint32 count);
    static sint32 GetFreeBank(void);
    static sint32 FindBank(uint32 id);
    static sint32 TestSubBankBounds(sint32 bank, sint32 subBank);
    static void SetCycleBits(uint8 *bank);

    static char *MemAlloc(sint32 size, char *label);
    static void MemFree(char *data);
    static void InitInGame(void);
    static uint32 EACrandom(void);
    static void EACseedrandom(uint32 seed);
    static sint32 Rand(sint32 limit);
    static sint32 BindData(uint8 *data);
    static sint32 MatchSample(sint32 bank, SPCHSample *sample, uint8 *component, sint32 *parameters);
    static sint32 GetPhraseBank(uint8 *component, sint32 *parameters, SPCHSampleChoice *choice);
    static uint8 *GetBankBits(uint8 *bank);
    static void ClearCycleBit(uint8 *bank, sint32 index);
    static uint32 CheckBankBit(uint8 *bank, sint32 index);
    static sint32 CheckTemplateSample(SPCHSampleChoice *choice, uint8 *bank, sint32 index);
    static sint32 SampleExists(SPCHSampleChoice *choice, uint8 *bank, sint32 index);
    static sint32 ChooseSamples(SPCHSampleChoice *choice, sint32 capacity, uint8 *component, sint32 *parameters);
    static sint32 SampleLength(SPCHSampleChoice *choice);
    static sint32 ConvertTime(sint32 size);
    static sint32 SentenceLength(uint8 *sentence);
    static void OrderSentences(SPCHEventDefinitionPrefix *definition, uint8 *order);
    static sint32 RepeatEvent(SPCHEventDefinitionPrefix *definition);
    static sint32 ShortRuleStatus(uint8 *sentence, sint32 filter);
    static sint32 SentenceGetChoices(uint8 *sentence, sint32 *parameters, uint32 required, uint32 unknown, sint32 filter);
    static void RandomizeSentencePicks(uint8 *sentence);
    static sint32 IterateChoice(uint8 *sentence);
    static sint32 ChooseShortSentence(uint8 *sentence);
    static sint32 SentenceMakeChoice(uint8 *sentence, sint32 filter);
    static void ConstantRuleSet(SPCHEventDefinitionPrefix *definition, uint8 *sentence);
    static void MakeSampleRequests(uint8 *sentence, sint32 event);
    static void ClearChosen(void);
    static void SaveChosenSentence(SPCHEventDefinitionPrefix *definition, uint8 *sentence, uint32 index, sint32 *parameters);
    static sint32 OneChosen(void);
    static void PlayChosen(void);
    static sint32 ChooseSentence(sint32 *parameters);
    static sint32 SentenceUsesParm(uint8 *sentence, uint32 parameter);
    static sint32 GetRuleID(SPCHEventDefinitionPrefix *definition, uint32 rule);
    static void RuleSet(SPCHEventDefinitionPrefix *definition, uint32 index, sint32 *parameters);
    static uint32 GetRuleSettings(SPCHEventDefinitionPrefix *definition, sint32 *parameters, uint8 *unknown);
    static sint32 CheckSentenceRules(uint32 required, uint32 unknown, uint8 *sentence);
    static void InitSample(SPCHSample *sample);
    static sint32 UnPackSample(uint8 *bank, sint32 index, SPCHSample *sample);
};

class SPCH
{
public:
    static sint32 AddEvent(sint32 *parameters);
    static void ClearEventQueue(void);
    static void PlaySpeech(void);
    static sint32 ChooseSpeech(void);
    static sint32 AddBank(void *bank);
    static void Deinit(void);
    static sint32 GetSampleDataRate(sint32 frequency, sint32 bits, sint32 format);

    static void *InitBankMem(MemAllocFuncPtr allocate, MemFreeFuncPtr release, sint32 count);
    static sint32 Init(SpeechSampleRequestFuncPtr request, uint32 gameNum, sint32 dataRate);
    static void SetPreLoadTicks(sint32 ticks);
};
#endif

#endif
