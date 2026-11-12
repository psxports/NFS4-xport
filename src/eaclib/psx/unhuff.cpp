#include "xport.h"
#include "eaclib/psx/internal.h"

struct HuffInput
{
    const uint8 *next;
    uint32 history;
    uint32 look;
    sint32 available;

    void refill();

    void consume(sint32 width);

    uint32 take(sint32 width);

    uint32 number();
};

void HuffInput::refill()
{
    uint32 high = next[0];
    uint32 low = next[1];
    next += 2;
    history = (history << 16) | (high << 8) | low;
    look = history << ((~(((uint32)available)) + 1) & 31);
    available += 16;
}

void HuffInput::consume(sint32 width)
{
    available = (available - width);
    look <<= (uint32)width & 31;
    if (available < 0)
        refill();
}

uint32 HuffInput::take(sint32 width)
{
    if (width <= 16)
    {
        uint32 result = look >> ((32 - (uint32)width) & 31);
        consume(width);
        return result;
    }
    uint32 high = look >> ((48 - (uint32)width) & 31);
    consume(width - 16);
    uint32 low = look >> 16;
    consume(16);
    return (high << 16) | low;
}

uint32 HuffInput::number()
{
    if ((sint32)look < 0)
        return take(3);
    sint32 width = 2;
    if (look >> 16)
    {
        do
        {
            look <<= 1;
            ++width;
        } while ((sint32)look >= 0);
        available = ((available + 1) - width);
        look <<= 1;
        if (available < 0)
            refill();
    }
    else
    {
        ++width;
        uint32 bit;
        do
        {
            bit = look >> 31;
            consume(1);
            ++width;
        } while (bit == 0);
        --width;
    }
    return take(width) + ((uint32)1 << ((uint32)width & 31));
}

extern "C" uint32 unhuff(const uint8 *data, uint8 *destination, sint32 unpack)
{
    FUNCTION_MARKER(0x800F459C, "NFS4.EXE");
    if (data == 0)
        return 0;

    XASSERT(unpack != 0);
    HuffInput input;
    input.history = (uint32(data[0]) << 24) | (uint32(data[1]) << 16) | (uint32(data[2]) << 8) | data[3];
    uint32 signature = input.history >> 16;
    input.next = data + 4;
    input.available = 0;
    input.look = input.history << 16;
    if ((signature & 0x100) != 0)
    {
        for (sint32 i = 0; i < 4; ++i)
            input.history = (input.history << 8) | *input.next++;
        input.available = 8;
        input.look = input.history << 8;
    }
    signature &= 0xfeff;
    uint32 sizeHigh = input.take(8);
    uint32 size = (sizeHigh << 16) | input.take(16);
    uint32 escape = input.take(8);
    sint32 counts[16];
    uint32 offsets[17];
    uint32 thresholds[17];
    uint8 alphabet[256];
    uint8 fastSymbols[256];
    uint8 fastLengths[256];
    uint8 used[256];
    uint32 total = 0;
    uint32 code = 0;
    sint32 depth = 1;
    for (;; ++depth)
    {
        code <<= 1;
        offsets[depth] = code - total;
        uint32 count = input.number() - 4;
        counts[depth - 1] = count;
        total += count;
        code += count;
        uint32 threshold = count == 0 ? 0 : (code << ((16 - (uint32)depth) & 31)) & 0xffff;
        thresholds[depth] = threshold;
        if (count != 0 && threshold == 0)
            break;
    }
    thresholds[depth] = 0xffffffff;
    for (sint32 i = 0; i < 256; ++i)
        used[i] = 0;
    uint8 cursor = 255;
    for (uint32 i = 0; i < total; ++i)
    {
        uint32 skip = input.number() - 3;
        do
        {
            ++cursor;
            if (used[cursor] == 0)
                --skip;
        } while (skip != 0);
        used[cursor] = 1;
        alphabet[i] = cursor;
    }
    for (sint32 i = 0; i < 256; ++i)
        fastLengths[i] = 0x40;
    uint32 entry = 0;
    uint32 symbolIndex = 0;
    for (sint32 width = 1; width <= depth && width <= 8; ++width)
    {
        sint32 repeat = 1 << (8 - width);
        for (sint32 i = 0; i < counts[width - 1]; ++i)
        {
            uint8 symbol = alphabet[symbolIndex++];
            uint8 length = (uint8)width;
            for (sint32 j = 0; j < repeat; ++j)
            {
                fastSymbols[entry] = symbol;
                fastLengths[entry++] = length;
            }
        }
    }
    uint8 *output = destination;
    for (;;)
    {
        uint32 prefix = input.look >> 24;
        sint32 width = fastLengths[prefix];
        if (width <= 8 && fastSymbols[prefix] != escape)
        {
            *output++ = fastSymbols[prefix];
            input.consume(width);
            continue;
        }
        if (width > 8)
        {
            width = 9;
            while (thresholds[width] <= input.look >> 16)
                ++width;
        }
        uint32 bits = input.look >> ((32 - (uint32)width) & 31);
        uint8 symbol = alphabet[bits - offsets[width]];
        input.consume(width);
        if (symbol != escape)
        {
            *output++ = symbol;
            continue;
        }
        uint32 repeat = input.number() - 4;
        if (repeat != 0)
        {
            uint8 previous = output[-1];
            uint8 *end = output + (sint32)repeat;
            do { *output++ = previous; } while (output < end);
            continue;
        }
        uint32 finished = input.take(1);
        if (finished != 0)
            break;
        *output++ = (uint8)input.take(8);
    }
    if (signature == 0x32fb || signature == 0x34fb)
    {
        uint32 first = 0;
        uint32 second = 0;
        for (uint32 i = 0; i < size; ++i)
        {
            first += destination[i];
            if (signature == 0x34fb)
            {
                second += first;
                destination[i] = (uint8)second;
            }
            else
                destination[i] = (uint8)first;
        }
    }
    return size;
}
