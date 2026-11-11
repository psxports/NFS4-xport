#include "xport.h"
#include "eaclib/psx/internal.h"

sint32 iSPCH::SentenceUsesParm(uint8 *sentence, uint32 parameter)
{
    FUNCTION_MARKER(0x8010B158, "NFS4.EXE");
    sint32 count = sentence[3] >> 2;
    sint32 result = 0;
    for (sint32 i = 0; i < count; ++i)
    {
        uint8 *component = sentence + sentence[4 + i] * 4;
        if (uint32(component[2] & 15) == parameter)
            return 1;
        for (sint32 j = 0; j < 4; ++j)
        {
            if (uint32(component[4 + j] & 15) == parameter)
            {
                result = 1;
                break;
            }
        }
    }
    return result;
}

sint32 iSPCH::GetRuleID(SPCHEventDefinitionPrefix *definition, uint32 rule)
{
    FUNCTION_MARKER(0x8010B220, "NFS4.EXE");
    uint8 *data = reinterpret_cast<uint8 *>(definition);
    uint8 *rules = data + 12 + data[6] * 2;
    if (rule < 8)
        return rules[rule * 2];
    return -1;
}

void iSPCH::RuleSet(SPCHEventDefinitionPrefix *definition, uint32 index, sint32 *parameters)
{
    FUNCTION_MARKER(0x8010B294, "NFS4.EXE");
    if (gSentenceRuleSet == 0)
        return;
    uint8 *data = reinterpret_cast<uint8 *>(definition);
    sint32 count = (sint8)data[7];
    uint8 *rules = data + 12 + data[6] * 2;
    uint8 *entry = data + 12 + index * 2;
    uint8 *sentence = data + (entry[0] + (entry[1] << 8)) * 4;
    for (sint32 i = 0; i < count; ++i, rules += 2)
    {
        uint32 id = rules[0];
        uint32 parameter = rules[1] & 15;
        uint32 kind = rules[1] >> 4;
        if ((kind == 0 || kind == 3) && SentenceUsesParm(sentence, parameter))
            gSentenceRuleSet(definition->id, id, uint32(parameters[parameter]));
    }
}

uint32 iSPCH::GetRuleSettings(SPCHEventDefinitionPrefix *definition, sint32 *parameters, uint8 *unknown)
{
    FUNCTION_MARKER(0x8010B3CC, "NFS4.EXE");
    uint8 *data = reinterpret_cast<uint8 *>(definition);
    sint32 count = (sint8)data[7];
    uint8 *rules = data + 12 + data[6] * 2;
    uint32 accepted = 0;
    uint32 untested = 0;
    for (uint32 parameter = 1; parameter <= 12; ++parameter)
    {
        for (sint32 i = 0; i < count; ++i)
        {
            uint8 *rule = rules + i * 2;
            uint32 id = rule[0];
            uint32 index = rule[1] & 15;
            uint32 kind = rule[1] >> 4;
            sint32 value;
            if (parameter == 12)
            {
                if (index != 0)
                    continue;
                value = 0;
            }
            else
            {
                if (index != parameter)
                    continue;
                value = parameters[parameter];
            }
            uint32 mask = 1u << ((7u - uint32(i)) & 31);
            if (kind == 4)
            {
                if (parameters[index] != 0)
                    accepted |= mask;
            }
            else
            {
                sint32 status = -1;
                if (gSentenceRuleTest != 0)
                    status = gSentenceRuleTest(definition->id, id, value);
                if (status > 0)
                    accepted |= mask;
                else if (status < 0)
                    untested |= mask;
            }
        }
    }
    *unknown = (uint8)untested;
    return accepted & 255;
}

sint32 iSPCH::CheckSentenceRules(uint32 required, uint32 unknown, uint8 *sentence)
{
    FUNCTION_MARKER(0x8010B58C, "NFS4.EXE");
    return (((uint32(sentence[2]) ^ required) & sentence[1] & ~unknown) == 0);
}
