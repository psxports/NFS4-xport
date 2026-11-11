#include "xport.h"
#include "eaclib/psx/internal.h"

void iSPCH::InitBanks(void)
{
    FUNCTION_MARKER(0x800EB1E0, "NFS4.EXE");
    gVoxBanks = 0;
    gNumBanks = 0;
}

void iSPCH::DisposeBanks(void)
{
    FUNCTION_MARKER(0x800EB1F4, "NFS4.EXE");
    uint8 **banks = gVoxBanks;
    if (banks != 0)
        MemFree(reinterpret_cast<char *>(banks));
    gVoxBanks = 0;
    gNumBanks = 0;
}

void *iSPCH::BankMemAlloc(sint32 count)
{
    FUNCTION_MARKER(0x800EB234, "NFS4.EXE");
    if (gVoxBanks == 0)
    {
        static char label[] = "spch banks";
        gNumBanks = count;
        uint8 **banks = reinterpret_cast<uint8 **>(MemAlloc((sint32)(uint32(count) << 2), label));
        gVoxBanks = banks;
        if (banks != 0)
        {
            sint32 slots = gNumBanks;
            for (sint32 i = 0; i < slots; ++i)
                banks[i] = 0;
        }
    }
    return gVoxBanks;
}

sint32 iSPCH::GetFreeBank(void)
{
    FUNCTION_MARKER(0x800EB2B8, "NFS4.EXE");
    sint32 count = gNumBanks;
    if (count <= 0)
        return -1;
    uint8 **banks = gVoxBanks;
    for (sint32 i = 0; i < count; ++i)
        if (banks[i] == 0)
            return i;
    return -1;
}

sint32 iSPCH::FindBank(uint32 id)
{
    FUNCTION_MARKER(0x800EB310, "NFS4.EXE");
    uint8 **banks = gVoxBanks;
    if (banks == 0)
        return -1;
    sint32 count = gNumBanks;
    if (count <= 0)
        return -1;
    id &= 0xFFFF;
    for (sint32 i = 0; i < count; ++i)
    {
        uint8 *bank = banks[i];
        if (bank != 0 && uint32(bank[0] + (bank[1] << 8)) == id)
            return i;
    }
    return -1;
}

sint32 iSPCH::TestSubBankBounds(sint32 bank, sint32 subBank)
{
    FUNCTION_MARKER(0x800EB37C, "NFS4.EXE");
    uint8 **banks = gVoxBanks;
    if (banks == 0 || bank < 0)
        return 0;
    uint8 *header = banks[bank];
    sint32 count = header[6] + (header[7] << 8);
    if (count == 0xFFFF || subBank < 0)
        return 0;
    return subBank < count;
}

void iSPCH::SetCycleBits(uint8 *bank)
{
    FUNCTION_MARKER(0x800EB3C8, "NFS4.EXE");
    uint8 *bits = GetBankBits(bank);
    sint32 groups = bits[0];
    if (groups == 0)
        return;
    sint32 remainder = gGameNum % groups;
    sint32 count = bank[3];
    sint32 first = remainder * count / groups;
    sint32 last = (remainder + 1) * count / groups;
    sint32 length = last - first;
    sint32 quotient = first / 8;
    sint32 byte = quotient + 1;
    sint32 bit = first - quotient * 8;
    for (sint32 i = 0; i < length; ++i)
    {
        uint32 mask = 1u << (uint32(bit) & 31);
        bits[byte] = (uint8)(bits[byte] | mask);
        ++bit;
        if (bit == 8)
        {
            bit = 0;
            ++byte;
        }
    }
}

sint32 SPCH::AddBank(void *bank)
{
    FUNCTION_MARKER(0x800EB520, "NFS4.EXE");
    sint32 index = -1;
    if (gVoxBanks != 0)
    {
        index = iSPCH::GetFreeBank();
        if (index >= 0)
        {
            uint8 *data = static_cast<uint8 *>(bank);
            if (data[2] & 0xF0)
                iSPCH::SetCycleBits(data);
            gVoxBanks[index] = data;
        }
    }
    return index;
}
