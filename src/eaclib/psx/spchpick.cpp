#include "xport.h"
#include "eaclib/psx/internal.h"

extern "C" sint32 VoxSentence_GetNumPhrases(const uint8 *sentence)
{
    FUNCTION_MARKER(0x80100730, "NFS4.EXE");
    FUNCTION_MARKER(0x8010B100, "NFS4.EXE");
    return sentence[3] >> 2;
}

sint32 iSPCH::MatchSample(sint32 bank, SPCHSample *sample, uint8 *component, sint32 *parameters)
{
    FUNCTION_MARKER(0x8010077C, "NFS4.EXE");
    sint32 count = (sint8)component[3];
    if (count >= 5)
        return 0;
    for (sint32 i = 0; i < count; ++i)
    {
        uint32 rule = sample->rules[i];
        if (rule >= 32)
            return 0;
        uint32 bit = 1u << rule;
        uint32 parameter = component[4 + i] & 15;
        uint8 *maskData = component + 8 + i * 4;
        uint32 mask = uint32(maskData[0]) | (uint32(maskData[1]) << 8) | (uint32(maskData[2]) << 16) | (uint32(maskData[3]) << 24);
        if ((bit & mask) == 0 || (parameter != 0 && (bit & uint32(parameters[parameter])) == 0))
            return 0;
    }
    return 1;
}

sint32 iSPCH::GetPhraseBank(uint8 *component, sint32 *parameters, SPCHSampleChoice *choice)
{
    FUNCTION_MARKER(0x80100880, "NFS4.EXE");
    uint32 id = component[0] + (component[1] << 8);
    uint32 kind = component[2] >> 4;
    uint32 parameter = component[2] & 15;
    choice->offsetMultiplier = -1;
    choice->bank = -1;
    if (kind == 0)
        choice->bank = (sint16)FindBank(id);
    else if (kind == 1)
    {
        sint32 bank = parameters[parameter];
        if (bank >= 0)
        {
            uint8 *header = gVoxBanks[bank];
            if ((uint32)(header[0] + (header[1] << 8)) != id)
                bank = -1;
        }
        choice->bank = (sint16)bank;
    }
    else if (kind == 2)
    {
        sint32 bank = FindBank(id);
        if (TestSubBankBounds(bank, parameters[parameter]))
            choice->offsetMultiplier = (sint16)parameters[parameter];
        else
            bank = -1;
        choice->bank = (sint16)bank;
    }
    return choice->bank >= 0;
}

uint8 *iSPCH::GetBankBits(uint8 *bank)
{
    FUNCTION_MARKER(0x80100994, "NFS4.EXE");
    return bank + 8 + bank[3] * ((bank[2] & 15) + 2);
}

void iSPCH::ClearCycleBit(uint8 *bank, sint32 index)
{
    FUNCTION_MARKER(0x801009B8, "NFS4.EXE");
    sint32 quotient = index / 8;
    sint32 remainder = index - quotient * 8;
    uint32 mask = ~(1u << (uint32(remainder) & 31));
    uint8 *bits = GetBankBits(bank);
    bits[quotient + 1] = (uint8)(bits[quotient + 1] & mask);
}

uint32 iSPCH::CheckBankBit(uint8 *bank, sint32 index)
{
    FUNCTION_MARKER(0x80100A1C, "NFS4.EXE");
    sint32 quotient = index / 8;
    sint32 remainder = index - quotient * 8;
    uint32 mask = 1u << (uint32(remainder) & 31);
    uint8 *bits = GetBankBits(bank);
    return uint32(bits[quotient]) & mask;
}

sint32 iSPCH::CheckTemplateSample(SPCHSampleChoice *choice, uint8 *bank, sint32 index)
{
    FUNCTION_MARKER(0x80100A70, "NFS4.EXE");
    sint32 count = bank[6] + (bank[7] << 8);
    sint32 multiplier = choice->offsetMultiplier;
    if (multiplier >= count)
        return 0;
    sint32 bit = (index + (bank[3] * multiplier));
    return CheckBankBit(bank, bit);
}

sint32 iSPCH::SampleExists(SPCHSampleChoice *choice, uint8 *bank, sint32 index)
{
    FUNCTION_MARKER(0x80100AC0, "NFS4.EXE");
    if (bank[3] < index)
        return 0;
    sint32 result = 1;
    if (choice->offsetMultiplier != -1)
        result = CheckTemplateSample(choice, bank, index);
    if (bank[2] & 0xF0)
        result = CheckBankBit(bank, (index + 8));
    return result;
}

sint32 iSPCH::ChooseSamples(SPCHSampleChoice *choice, sint32 capacity, uint8 *component, sint32 *parameters)
{
    FUNCTION_MARKER(0x80100B4C, "NFS4.EXE");
    sint32 bankIndex = choice->bank;
    uint8 *bank = gVoxBanks[bankIndex];
    sint32 count = bank[3];
    sint32 output = choice->unknown6;
    sint32 selected = 0;
    for (sint32 i = 0; i < count; ++i)
    {
        SPCHSample sample;
        if (UnPackSample(bank, i, &sample) != 0 && MatchSample(bankIndex, &sample, component, parameters) != 0 && SampleExists(choice, bank, i) != 0)
        {
            ispch_gPickSamples[output] = (uint8)i;
            ++selected;
            ++output;
            if (selected >= capacity)
                break;
        }
    }
    return selected;
}

sint32 iSPCH::SampleLength(SPCHSampleChoice *choice)
{
    FUNCTION_MARKER(0x80100C5C, "NFS4.EXE");
    sint32 index = ispch_gPickSamples[choice->pickIndex];
    uint8 *bank = gVoxBanks[choice->bank];
    SPCHSample sample;
    if (UnPackSample(bank, index, &sample))
        return sample.size;
    return 0;
}

sint32 iSPCH::ConvertTime(sint32 size)
{
    FUNCTION_MARKER(0x80100CC4, "NFS4.EXE");
    sint32 rate = gDataRate;
    if (rate == 0)
        return 0;
    sint32 scaled = (size * 100);
    if (rate == -1 && scaled == (-2147483647 - 1))
        abort();
    return scaled / rate;
}

sint32 iSPCH::SentenceLength(uint8 *sentence)
{
    FUNCTION_MARKER(0x80100D20, "NFS4.EXE");
    sint32 count = sentence[3] >> 2;
    uint32 total = 0;
    for (sint32 i = 0; i < count; ++i)
        total += uint32(SampleLength(&ispch_gChoice[i]));
    return ConvertTime((sint32)total);
}

void iSPCH::OrderSentences(SPCHEventDefinitionPrefix *definition, uint8 *order)
{
    FUNCTION_MARKER(0x80100D94, "NFS4.EXE");
    uint8 weights[104];
    uint8 *data = reinterpret_cast<uint8 *>(definition);
    sint32 count = data[6];
    sint32 total = 0;
    for (sint32 i = 0; i < count; ++i)
    {
        uint8 *entry = data + 12 + i * 2;
        uint8 *sentence = data + (entry[0] + (entry[1] << 8)) * 4;
        weights[i] = sentence[0];
        total += sentence[0];
    }
    sint32 output = 0;
    while (total > 0)
    {
        sint32 value = Rand(total);
        sint32 index;
        for (index = 0; index < count; ++index)
        {
            value -= weights[index];
            if (value < 0)
                break;
        }
        order[output++] = (uint8)index;
        total -= weights[index];
        weights[index] = 0;
    }
    for (sint32 i = 0; i < count; ++i)
    {
        uint8 *entry = data + 12 + i * 2;
        uint8 *sentence = data + (entry[0] + (entry[1] << 8)) * 4;
        if (sentence[0] == 0)
            order[output++] = (uint8)i;
    }
}

sint32 iSPCH::RepeatEvent(SPCHEventDefinitionPrefix *definition)
{
    FUNCTION_MARKER(0x80100EE0, "NFS4.EXE");
    sint32 result = 1;
    if (definition->id == gVoxInGame.event)
    {
        sint32 limit = (sint8)definition->unknown6[2];
        if (limit > 0)
            result = gVoxInGame.count < limit;
    }
    return result;
}

sint32 iSPCH::ShortRuleStatus(uint8 *sentence, sint32 filter)
{
    FUNCTION_MARKER(0x80100F24, "NFS4.EXE");
    sint32 kind = sentence[3] & 3;
    return kind == 0 || (kind == 1 && filter == 2) || (kind == 2 && filter != 2);
}

sint32 iSPCH::SentenceGetChoices(uint8 *sentence, sint32 *parameters, uint32 required, uint32 unknown, sint32 filter)
{
    FUNCTION_MARKER(0x80100F8C, "NFS4.EXE");
    sint32 count = sentence[3] >> 2;
    sint32 total = 0;
    if (count >= 13)
        return 1;
    if (!ShortRuleStatus(sentence, filter) || !CheckSentenceRules(required & 255, unknown & 255, sentence))
        return 0;
    for (sint32 i = 0; i < count; ++i)
    {
        SPCHSampleChoice *choice = &ispch_gChoice[i];
        choice->unknown6 = (sint16)total;
        uint8 *component = sentence + sentence[4 + i] * 4;
        if (!GetPhraseBank(component, parameters, choice))
        {
            if (choice->bank == -2)
            {
                choice->bank = -1;
                return -1;
            }
            return 0;
        }
        sint32 available = ChooseSamples(choice, (100 - total), component, parameters);
        total = (total + available);
        if (available == 0)
            return 0;
        choice->unknown4 = (sint16)available;
    }
    return 1;
}

void iSPCH::RandomizeSentencePicks(uint8 *sentence)
{
    FUNCTION_MARKER(0x801010CC, "NFS4.EXE");
    sint32 count = VoxSentence_GetNumPhrases(sentence);
    for (sint32 i = 0; i < count; ++i)
    {
        sint32 length = ispch_gChoice[i].unknown4;
        sint32 swaps = length / 2;
        uint8 *picks = ispch_gPickSamples + ispch_gChoice[i].unknown6;
        for (sint32 j = 0; j < swaps; ++j)
        {
            sint32 random = Rand(length);
            uint8 first = picks[j];
            uint8 second = picks[random];
            picks[j] = second;
            picks[random] = first;
        }
    }
}

sint32 iSPCH::IterateChoice(uint8 *sentence)
{
    FUNCTION_MARKER(0x801011AC, "NFS4.EXE");
    sint32 index = VoxSentence_GetNumPhrases(sentence) - 1;
    uint8 *choice = ispch_gPickSamples + offsetof(SPCHChoiceState, choices) + index * 12;
    sint16 length;
    sint16 start;
    memcpy(&length, choice + 4, sizeof(length));
    memcpy(&start, choice + 6, sizeof(start));
    sint32 limit = start + length;
    for (;;)
    {
        uint16 pick;
        memcpy(&pick, choice + 8, sizeof(pick));
        pick = (uint16)(pick + 1);
        memcpy(choice + 8, &pick, sizeof(pick));
        sint16 signedPick;
        memcpy(&signedPick, &pick, sizeof(signedPick));
        if (signedPick < limit)
            return 0;
        memcpy(&pick, choice + 6, sizeof(pick));
        memcpy(choice + 8, &pick, sizeof(pick));
        --index;
        choice -= 12;
        memcpy(&length, choice + 4, sizeof(length));
        memcpy(&start, choice + 6, sizeof(start));
        limit = start + length;
        if (index < 0)
            return 1;
    }
}

sint32 iSPCH::ChooseShortSentence(uint8 *sentence)
{
    FUNCTION_MARKER(0x8010125C, "NFS4.EXE");
    sint32 count = sentence[3] >> 2;
    for (sint32 i = 0; i < count; ++i)
        ispch_gChoice[i].pickIndex = ispch_gChoice[i].unknown6;
    sint32 done = 0;
    sint32 chosen = 0;
    while (done == 0 && chosen == 0)
    {
        sint32 length = SentenceLength(sentence);
        sint32 limit = GetFilterLength();
        if (length < limit)
        {
            chosen = 1;
            done = 1;
        }
        else
            done = IterateChoice(sentence);
    }
    return chosen;
}

sint32 iSPCH::SentenceMakeChoice(uint8 *sentence, sint32 filter)
{
    FUNCTION_MARKER(0x80101310, "NFS4.EXE");
    if (filter == 1)
    {
        RandomizeSentencePicks(sentence);
        return ChooseShortSentence(sentence);
    }
    sint32 count = sentence[3] >> 2;
    if (count <= 0)
        return 0;
    for (sint32 i = 0; i < count; ++i)
    {
        SPCHSampleChoice *choice = &ispch_gChoice[i];
        sint32 index = Rand(choice->unknown4);
        choice->pickIndex = (sint16)((uint16)choice->unknown6 + index);
    }
    return 1;
}

void iSPCH::ConstantRuleSet(SPCHEventDefinitionPrefix *definition, uint8 *sentence)
{
    FUNCTION_MARKER(0x801013BC, "NFS4.EXE");
    if (gSentenceRuleSet == 0)
        return;
    sint32 count = sentence[3] >> 2;
    for (sint32 i = 0; i < count; ++i)
    {
        uint8 *component = sentence + sentence[4 + i] * 4;
        SPCHSampleChoice *choice = &ispch_gChoice[i];
        for (sint32 j = 0; j < 4; ++j)
        {
            uint32 rule = component[4 + j] >> 4;
            if (rule != 15)
            {
                sint32 id = GetRuleID(definition, rule);
                uint8 *bank = gVoxBanks[choice->bank];
                sint32 sampleId = ispch_gPickSamples[choice->pickIndex];
                SPCHSample sample;
                if (UnPackSample(bank, sampleId, &sample))
                    gSentenceRuleSet(definition->id, (uint32)id, 1u << (sample.rules[j] & 31));
            }
        }
    }
}

void iSPCH::MakeSampleRequests(uint8 *sentence, sint32 event)
{
    FUNCTION_MARKER(0x80101508, "NFS4.EXE");
    sint32 count = sentence[3] >> 2;
    uint32 total = 0;
    for (sint32 i = 0; i < count; ++i)
    {
        SPCHSampleChoice *choice = &ispch_gChoice[i];
        uint8 *bank = gVoxBanks[choice->bank];
        sint32 sampleId = ispch_gPickSamples[choice->pickIndex];
        if ((bank[2] & 0xF0) && gClearCycle != 0)
            ClearCycleBit(bank, sampleId);
        SPCHSample sample;
        if (UnPackSample(bank, sampleId, &sample))
        {
            uint32 offset = uint32(sample.offset);
            sint32 multiplier = choice->offsetMultiplier;
            if (multiplier != -1)
                offset += uint32(multiplier) * uint32((bank[4] + (bank[5] << 8)) << 8);
            total += uint32(sample.size);
            gSampleRequest(choice->bank, (sint32)offset, sample.size, event);
        }
    }
    ConvertTime((sint32)total);
}

void iSPCH::ClearChosen(void)
{
    FUNCTION_MARKER(0x80101650, "NFS4.EXE");
    gSentenceChoice.valid = 0;
}

void iSPCH::SaveChosenSentence(SPCHEventDefinitionPrefix *definition, uint8 *sentence, uint32 index, sint32 *parameters)
{
    FUNCTION_MARKER(0x8010165C, "NFS4.EXE");
    gSentenceChoice.definition = definition;
    gSentenceChoice.sentence = sentence;
    gSentenceChoice.index = index;
    for (sint32 i = 0; i < 12; ++i)
        gSentenceChoice.parameters[i] = parameters[i];
    gSentenceChoice.valid = 1;
}

sint32 iSPCH::OneChosen(void)
{
    FUNCTION_MARKER(0x801016A4, "NFS4.EXE");
    return gSentenceChoice.valid;
}

void iSPCH::PlayChosen(void)
{
    FUNCTION_MARKER(0x801016B4, "NFS4.EXE");
    if (OneChosen() != 0)
    {
        sint32 event = gSentenceChoice.parameters[0];
        RuleSet(gSentenceChoice.definition, gSentenceChoice.index, gSentenceChoice.parameters);
        ConstantRuleSet(gSentenceChoice.definition, gSentenceChoice.sentence);
        MakeSampleRequests(gSentenceChoice.sentence, event);
        if (event == gVoxInGame.event)
            gVoxInGame.count = (gVoxInGame.count + 1);
        else
        {
            gVoxInGame.event = event;
            gVoxInGame.count = 1;
        }
    }
    ClearChosen();
}

sint32 iSPCH::ChooseSentence(sint32 *parameters)
{
    FUNCTION_MARKER(0x80101754, "NFS4.EXE");
    uint8 unknown = 0;
    SPCHEventDefinitionPrefix *definition = FindEvent(uint32(parameters[0]));
    if (definition == 0 || !RepeatEvent(definition))
        return 0;
    ClearChosen();
    sint32 filter = gFilterSetting;
    if (gVoxEvents.unknown4 == 1)
    {
        filter = (filter + 1);
        if (filter > 2)
            filter = 2;
    }
    if ((definition->flags & 1) && filter == 1)
        filter = 0;
    uint32 required = GetRuleSettings(definition, parameters, &unknown);
    uint8 order[104];
    OrderSentences(definition, order);
    sint32 count = definition->unknown6[0];
    if (count == 0)
        return -1;
    sint32 result = 0;
    for (sint32 i = 0; i < count && result == 0; ++i)
    {
        uint32 index = order[i];
        if (index >= (uint32)count)
            break;
        uint8 *data = reinterpret_cast<uint8 *>(definition);
        uint8 *entry = data + 12 + index * 2;
        uint8 *sentence = data + (entry[0] + (entry[1] << 8)) * 4;
        sint32 choices = SentenceGetChoices(sentence, parameters, required & 255, unknown, filter);
        if (choices > 0)
        {
            result = SentenceMakeChoice(sentence, filter);
            if (result != 0)
                SaveChosenSentence(definition, sentence, index, parameters);
        }
    }
    return result;
}

void SPCH::SetPreLoadTicks(sint32 ticks)
{
    FUNCTION_MARKER(0x801018F4, "NFS4.EXE");
    gPreLoadTicks = ticks;
}
