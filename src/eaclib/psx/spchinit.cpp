#include "xport.h"
#include "eaclib/psx/internal.h"
#include "spchinit.h"

char *iSPCH::MemAlloc(sint32 size, char *label)
{
    FUNCTION_MARKER(0x800EB5A4, "NFS4.EXE");
    MemAllocFuncPtr allocate = gMemAlloc;
    if (allocate != 0)
        return allocate(size, label);
    return 0;
}

void iSPCH::MemFree(char *data)
{
    FUNCTION_MARKER(0x800EB5D4, "NFS4.EXE");
    MemFreeFuncPtr release = gMemFree;
    if (release != 0)
        release(data);
}

void SPCH::Deinit(void)
{
    FUNCTION_MARKER(0x800EB600, "NFS4.EXE");
    if (gSPCH_Initialized == 0x01789A34)
    {
        gSampleRequest = 0;
        gSentenceRuleTest = 0;
        gSPCH_Initialized = 0;
        gSentenceRuleSet = 0;
        iSPCH::DisposeBanks();
        iSPCH::InitEventDat();
    }
}

void iSPCH::InitInGame(void)
{
    FUNCTION_MARKER(0x800EB654, "NFS4.EXE");
    gVoxInGame.event = -1;
    gVoxInGame.count = 0;
}

sint32 SPCH::GetSampleDataRate(sint32 frequency, sint32 bits, sint32 format)
{
    FUNCTION_MARKER(0x800EB66C, "NFS4.EXE");
    sint32 rate = (sint32)((uint32)frequency * (uint32)bits);
    rate /= 8;
    if (format == 1)
        rate /= 10;
    else if (format == 2)
        rate = rate * 2 / 7;
    return rate;
}

void *SPCH::InitBankMem(MemAllocFuncPtr allocate, MemFreeFuncPtr release, sint32 count)
{
    FUNCTION_MARKER(0x800EB6F0, "NFS4.EXE");
    if (gSPCH_Initialized != 0x01789A34 || allocate == 0 || release == 0)
        return 0;
    gMemAlloc = allocate;
    gMemFree = release;
    return iSPCH::BankMemAlloc(count);
}

sint32 SPCH::Init(SpeechSampleRequestFuncPtr request, uint32 gameNum, sint32 dataRate)
{
    FUNCTION_MARKER(0x800EB748, "NFS4.EXE");
    gSampleRequest = request;
    gGameNum = (sint32)gameNum;
    gDataRate = dataRate;
    gMemAlloc = 0;
    gMemFree = 0;
    gSentenceRuleTest = 0;
    gSentenceRuleSet = 0;
    iSPCH::EACseedrandom(gameNum);
    iSPCH::ClearChosen();
    SetPreLoadTicks(0);
    gFilterSetting = 0;
    iSPCH::InitEventDat();
    iSPCH::InitInGame();
    iSPCH::InitBanks();
    iSPCH::InitEventQueue();
    gSPCH_Initialized = 0x01789A34;
    return 1;
}
