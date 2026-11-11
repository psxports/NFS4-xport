#include "xport.h"
#include "eaclib/psx/internal.h"
#include "sdma.h"

static void UploadSoundDmaSpan(uint32 address, const uint8 *source, uint32 bytes)
{
    uint32 first = SPU_RAM_SIZE - address;
    if (bytes <= first)
    {
        if (!spu_upload(address, source, bytes))
            abort();
    }
    else if (!spu_upload(address, source, first) || !spu_upload(0, source + first, bytes - first))
        abort();
}

void iSNDdmtransfer(void)
{
    FUNCTION_MARKER(0x8010A880, "NFS4.EXE");

    sint32 queued = sndpd.queued;
    sint32 interruptsEnabled = xport_bios_enter_critical();
    if (queued > 0)
    {
        sndpd.active = 1;
        sndpd.completed = 0;
        uint32 priority = 0;
        uint32 token = 0xFFFFFFFFu;
        for (sint32 i = 0; i < 10; ++i)
        {
            const SoundDmaRequest *candidate = &(sndpd.requests[i]);
            if (candidate->token != 0 &&
                (candidate->priority > priority || (candidate->priority == priority && candidate->token < token)))
            {
                sndpd.current = (sint8)i;
                priority = candidate->priority;
                token = candidate->token;
            }
        }

        SoundDmaRequest *request = &(sndpd.requests[sndpd.current]);
        if (request->notify != 0)
        {
            void (*callback)(void) = sndpd.irqCallback;
            sndpd.unknown0[1] = 1;
            if (callback != 0)
                callback();
        }
        request->deadline = sndgs.clock + 15u;
        WriteSoundRegister(0x1A6, request->destinationUnits);
        WriteSoundRegister(0x1AA, (ReadSoundRegister(0x1AA) & 0xFFCFu) | 0x20u);

        uint32 address = (uint32)request->destinationUnits << 3;
        uint32 rounded = (uint32)request->lengthUnits << 6;
        uint32 bytes = request->sourceBytes;
        if (bytes > rounded)
            abort();
        if (bytes != 0)
            UploadSoundDmaSpan(address, request->source, bytes);
        if (rounded > bytes)
        {
            static const uint8 padding[64] = { 0 };
            UploadSoundDmaSpan((address + bytes) & (SPU_RAM_SIZE - 1u), padding, rounded - bytes);
        }
        sndpd.completed = 1;
    }
    if (interruptsEnabled != 0)
        xport_bios_exit_critical();
}

void iSNDdmcallback(void)
{
    FUNCTION_MARKER(0x8010AA64, "NFS4.EXE");
    sndpd.completed = 0;
    volatile uint32 delay = 13;
    volatile sint32 iterations = 0;
    while (iterations < 750)
    {
        delay = delay * 13u;
        iterations = iterations + 1;
    }
    uint32 control;
    if (!spu_mmio_read(0, 0x1F801DAAu, 2, &control))
        abort();
    if (!spu_mmio_write(0, 0x1F801DAAu, 2, control & 0xFFCFu, 3))
        abort();
    if (!spu_mmio_read(0, 0x1F801DAAu, 2, &control))
        abort();
    if ((control & 0x30u) != 0)
        for (sint32 poll = 1; poll < 4001; ++poll)
        {
            if (!spu_mmio_read(0, 0x1F801DAAu, 2, &control))
                abort();
            if ((control & 0x30u) == 0)
                break;
        }
    uint32 token = sndpd.requests[sndpd.current].token;
    sint32 interruptsEnabled = xport_bios_enter_critical();
    if (token != 0)
    {
        if (sndpd.unknown0[1] != 0)
        {
            void (*callback)(void) = sndpd.irqCallback;
            sndpd.unknown0[1] = 0;
            if (callback != 0)
                callback();
        }
        sndpd.requests[sndpd.current].token = 0;
        sndpd.queued = (sint8)((uint8)sndpd.queued - 1u);
        if (sndpd.queued <= 0)
        {
            sndpd.active = 0;
            if (interruptsEnabled != 0)
                xport_bios_exit_critical();
            return;
        }
    }
    if (interruptsEnabled != 0)
        xport_bios_exit_critical();
    iSNDdmtransfer();
}

void iSNDdmservice(void)
{
    FUNCTION_MARKER(0x8010AC20, "NFS4.EXE");
    if (sndpd.active == 0)
        iSNDdmtransfer();
    else if (sndpd.completed != 0)
        iSNDdmcallback();
}

uint32 iSNDdmqueue(const uint8 *source, sint32 spuAddress, sint32 size, sint32 priority, sint32 notify)
{
    FUNCTION_MARKER(0x8010ACA0, "NFS4.EXE");
    uint32 length = (uint32)size;
    if ((length & 63u) != 0)
        length += 64u;
    sndpd.generation += 1u;
    uint32 generation = sndpd.generation;
    sint32 interruptsEnabled = xport_bios_enter_critical();
    if (generation == 0)
        sndpd.generation = 1;
    for (sint32 i = 0; i < 10; ++i)
    {
        SoundDmaRequest *request = &sndpd.requests[i];
        if (request->token == 0)
        {
            sndpd.queued = (sint8)((uint8)sndpd.queued + 1u);
            request->source = source;
            request->token = sndpd.generation;
            request->destinationUnits = (uint16)((uint32)spuAddress >> 3);
            request->lengthUnits = (uint8)((sint32)length >> 6);
            request->priority = (uint8)priority;
            request->notify = (uint8)notify;
            request->sourceBytes = (uint16)size;
            if (interruptsEnabled != 0)
                xport_bios_exit_critical();
            iSNDdmservice();
            return sndpd.generation;
        }
    }
    if (interruptsEnabled != 0)
        xport_bios_exit_critical();
    return 0;
}

uint32 iSNDdmqueuesplit(const uint8 *source, sint32 spuAddress, sint32 size, sint32 priority)
{
    FUNCTION_MARKER(0x8010AD94, "NFS4.EXE");
    uint32 request = 0;
    while (size > 0)
    {
        sint32 chunk = size < 4096 ? size : 4096;
        while (10 - sndpd.queued < 7)
            iSNDdmservice();
        request = iSNDdmqueue(source, spuAddress, chunk, priority, 0);
        source += chunk;
        size -= chunk;
        spuAddress = (sint32)(((uint32)(spuAddress)) + ((uint32)(chunk)));
    }
    return request;
}

sint32 iSNDdmcomplete(uint32 request)
{
    FUNCTION_MARKER(0x8010AE6C, "NFS4.EXE");
    iSNDdmservice();
    if (sndpd.queued > 0)
    {
        for (sint32 i = 0; i < 10; ++i)
            if (sndpd.requests[i].token == request)
                return 0;
    }
    return 1;
}
