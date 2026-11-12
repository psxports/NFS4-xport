#include "xport.h"
#include "eaclib/psx/internal.h"
#include "stream.h"

uint32 requestidcounter;

extern "C" sint32 validatehandle(NativeStreamHandle *handle, NativeStreamControl **control, NativeStreamHandle **filter)
{
    FUNCTION_MARKER(0x800FC2F8, "NFS4.EXE");
    if (handle == 0)
        return 1;
    NativeStreamControl *owner = handle->control;
    if (owner->magic != 0x4D525453)
        return 1;
    *filter = handle;
    *control = owner;
    return 0;
}

extern "C" sint32 inbetween(uintptr_t start, uintptr_t end, uintptr_t position)
{
    FUNCTION_MARKER(0x800FC334, "NFS4.EXE");
    if (start <= end)
        return start <= position && position < end;
    return start <= position || position < end;
}

extern "C" void decbufferusage(NativeStreamControl *control, uint32 length)
{
    FUNCTION_MARKER(0x800FC374, "NFS4.EXE");
    sint32 interruptsEnabled = xport_bios_enter_critical();
    sint32 oldUsage = control->bufferUsage;
    sint32 usage = (oldUsage - length);
    control->bufferUsage = usage;
    if (interruptsEnabled != 0)
        xport_bios_exit_critical();
    sint32 threshold = control->greedyLevel;
    if (oldUsage >= threshold && usage < threshold)
    {
        sint32 state = control->state;
        control->greedyEnabled = 1;
        if (state == 1)
            XPORT_UNPLACED::FILE::priorityop(control->fileOperation, control->priority);
    }
}

extern "C" NativeStreamRequest *getfreerequest(NativeStreamControl *control)
{
    FUNCTION_MARKER(0x800FC400, "NFS4.EXE");
    sint32 interruptsEnabled = xport_bios_enter_critical();
    NativeStreamRequest *request = control->unknown58;
    if (request != 0)
    {
        uint32 generation = requestidcounter + 256;
        control->unknown58 = request->next;
        requestidcounter = generation;
        if (generation == 0)
            requestidcounter = 256;
        request->index = (request->index & 255) | requestidcounter;
    }
    if (interruptsEnabled != 0)
        xport_bios_exit_critical();
    return request;
}

extern "C" void queuerequest(NativeStreamControl *control, NativeStreamRequest *request)
{
    FUNCTION_MARKER(0x800FC478, "NFS4.EXE");
    request->state = 1;
    request->next = 0;
    sint32 interruptsEnabled = xport_bios_enter_critical();
    if (control->unknown54 == 0)
    {
        request->previous = 0;
        control->unknown4C = request;
        control->unknown50 = request;
    }
    else
    {
        request->previous = control->unknown54;
        control->unknown54->next = request;
    }
    control->unknown54 = request;
    if (interruptsEnabled != 0)
        xport_bios_exit_critical();
}

static NativeStreamRequest *locaterequest(NativeStreamControl *control, uint32 key)
{
    FUNCTION_MARKER(0x800FC4E4, "NFS4.EXE");
    sint32 index = key & 255;
    if (index >= control->requestCount)
        return 0;
    NativeStreamRequest *request = &control->requests[index];
    if (request->index != key || request->state == 0)
        return 0;
    return request;
}

extern "C" void freerequest(NativeStreamControl *control, NativeStreamRequest *request)
{
    FUNCTION_MARKER(0x800FC548, "NFS4.EXE");
    if (request == control->unknown4C)
        control->unknown4C = request->next;
    else
        request->previous->next = request->next;
    if (request == control->unknown54)
        control->unknown54 = request->previous;
    else
        request->next->previous = request->previous;
    if (request == control->unknown50)
        control->unknown50 = request->next != 0 ? request->next : request->previous;
    request->state = 0;
    request->next = control->unknown58;
    control->unknown58 = request;
}

extern "C" sint32 filterchunk(NativeStreamControl *control, const uint32 *data)
{
    FUNCTION_MARKER(0x800FC5E4, "NFS4.EXE");
    sint32 count = control->ruleCount;
    uint32 word = *data;
    if (count <= 0)
        return -2;
    NativeStreamRule *rule = control->rules;
    for (sint32 i = 0; i < count; ++i, ++rule)
    {
        uint32 mask = rule->mask;
        uint32 value = rule->value;
        if ((word & mask) == value)
            return rule->target;
    }
    return -2;
}

extern "C" sint32 parsechunks(NativeStreamControl *control)
{
    FUNCTION_MARKER(0x800FC634, "NFS4.EXE");
    NativeStreamRequest *request = control->unknown50;
    for (;;)
    {
        uint32 *chunk = reinterpret_cast<uint32 *>(control->cursor44);
        uintptr_t written = reinterpret_cast<uintptr_t>(control->cursor48);
        if ((sint32)(written - reinterpret_cast<uintptr_t>(chunk)) < 8)
            return 0;
        uint32 length = chunk[1];
        if ((length & 0xFF000000) != 0 || written < reinterpret_cast<uintptr_t>(chunk) + length)
            return 0;
        sint32 target = filterchunk(control, chunk);
        if (target >= 0)
            chunk[1] |= (uint32)target << 24;
        sint32 interruptsEnabled = xport_bios_enter_critical();
        bool cancelled = request->state == 4;
        if (!cancelled)
        {
            if (target < 0)
                chunk[0] = 0xFFFFFFFE;
            else
            {
                NativeStreamHandle *filter = &control->handles[target - 1];
                uint32 available = filter->available + length;
                filter->available = available;
                if (available == length)
                    filter->chunk = chunk;
            }
            control->cursor44 += length;
            if (target >= 0)
            {
                sint32 oldUsage = control->bufferUsage;
                sint32 threshold = control->greedyLevel;
                sint32 usage = (oldUsage + length);
                control->bufferUsage = usage;
                if (oldUsage < threshold && usage >= threshold)
                    control->greedyEnabled = 0;
            }
        }
        if (interruptsEnabled != 0)
            xport_bios_exit_critical();
        if (cancelled)
            return 0;
        if (chunk[0] == request->endMarker)
            return 1;
    }
}

extern "C" void opencallback(uint32 operation, uint32 result, void *userData)
{
    FUNCTION_MARKER(0x800FC810, "NFS4.EXE");
    NativeStreamControl *control = static_cast<NativeStreamControl *>(userData);
    NativeFileSlot *file = reinterpret_cast<NativeFileSlot *>(XPORT_UNPLACED::FILE::completeop(control->fileOperation));
    control->unknown9C = file;
    if (file != 0)
        restartstream(control, control->priority);
}

extern "C" void closecallback(uint32 operation, uint32 result, void *userData)
{
    FUNCTION_MARKER(0x800FC850, "NFS4.EXE");
    NativeStreamControl *control = static_cast<NativeStreamControl *>(userData);
    XPORT_UNPLACED::FILE::completeop(control->fileOperation);
    uint32 next = XPORT_UNPLACED::FILE::open(reinterpret_cast<char *>(control->unknown5C), 1, control->priority, control);
    control->fileOperation = next;
    if (next != 0)
        XPORT_UNPLACED::FILE::callbackop(next, opencallback);
}

extern "C" void readcallback(uint32 operation, uint32 result, void *userData)
{
    FUNCTION_MARKER(0x800FC8A8, "NFS4.EXE");
    NativeStreamControl *control = static_cast<NativeStreamControl *>(userData);
    NativeStreamRequest *request = control->unknown50;
    sint32 count;
    bool finished;
    if (request->memoryBacked == 1)
    {
        count = control->readLength;
        sint32 end = (control->offset + count);
        finished = request->sizeOrOffset <= end;
    }
    else
    {
        count = static_cast<sint32>(XPORT_UNPLACED::FILE::completeop(control->fileOperation));
        finished = count < control->readLength;
    }
    control->offset = (control->offset + count);
    control->cursor48 = reinterpret_cast<uint8 *>(reinterpret_cast<uintptr_t>(control->cursor48) + (uint32)count);
    sint32 endMarker = parsechunks(control);
    if (request->state != 4)
    {
        if (!finished && endMarker == 0)
        {
            restartstream(control, control->priority);
            return;
        }
        sint32 interruptsEnabled = xport_bios_enter_critical();
        request->state = 3;
        if (interruptsEnabled != 0)
            xport_bios_exit_critical();
    }
    startnextrequest(control, control->priority);
}

extern "C" void startnextrequest(NativeStreamControl *control, sint32 priority)
{
    FUNCTION_MARKER(0x800FC9B4, "NFS4.EXE");
    sint32 interruptsEnabled = xport_bios_enter_critical();
    NativeStreamRequest *request = control->unknown50;
    bool stop = request == 0;
    if (request != 0 && request->state != 1)
    {
        if (request->next == 0)
            stop = true;
        else
            control->unknown50 = request->next;
    }
    NativeStreamRequest *active = 0;
    if (stop)
        control->state = 0;
    else
    {
        active = control->unknown50;
        active->firstChunk = control->cursor44;
        active->state = 2;
    }
    if (interruptsEnabled != 0)
        xport_bios_exit_critical();
    if (stop)
        return;
    control->cursor48 = control->cursor44;
    if (active->memoryBacked == 1)
        control->offset = 0;
    else
    {
        control->offset = active->sizeOrOffset;
        char *name = reinterpret_cast<char *>(control->unknown5C);
        if (strcmp(active->name, name) != 0)
        {
            strcpy(name, active->name);
            if (control->unknown9C != 0)
            {
                uint32 next = XPORT_UNPLACED::FILE::close(control->unknown9C, priority, control);
                control->fileOperation = next;
                if (next != 0)
                    XPORT_UNPLACED::FILE::callbackop(next, closecallback);
            }
            else
            {
                uint32 next = XPORT_UNPLACED::FILE::open(name, 1, priority, control);
                control->fileOperation = next;
                if (next != 0)
                    XPORT_UNPLACED::FILE::callbackop(next, opencallback);
            }
            return;
        }
    }
    restartstream(control, priority);
}

extern "C" void restartstream(NativeStreamControl *control, sint32 priority)
{
    FUNCTION_MARKER(0x800FCB44, "NFS4.EXE");
    uint8 *parsed = control->cursor44;
    while (control->cursor40 != parsed)
    {
        uint32 *chunk = reinterpret_cast<uint32 *>(control->cursor40);
        if (chunk[0] == 0xFFFFFFFF)
            control->cursor40 = control->bufferStart;
        else if (chunk[0] == 0xFFFFFFFE)
            control->cursor40 += chunk[1];
        else
            break;
    }
    sint32 interruptsEnabled = xport_bios_enter_critical();
    NativeStreamRequest *head = control->unknown4C;
    while (head->next != 0 && head->next->state != 1 && !inbetween(reinterpret_cast<uintptr_t>(control->cursor40), reinterpret_cast<uintptr_t>(control->cursor48), reinterpret_cast<uintptr_t>(head->next->firstChunk) - 1))
    {
        freerequest(control, head);
        head = control->unknown4C;
    }
    if (interruptsEnabled != 0)
        xport_bios_exit_critical();
    sint32 capacity;
    uintptr_t first = reinterpret_cast<uintptr_t>(control->cursor40);
    uintptr_t written = reinterpret_cast<uintptr_t>(control->cursor48);
    if (first > written)
        capacity = (sint32)(first - written - 1);
    else
    {
        capacity = (sint32)(reinterpret_cast<uintptr_t>(control->bufferEnd) - written - 8);
        if (capacity < 0x2000)
        {
            sint32 partial = (sint32)(written - reinterpret_cast<uintptr_t>(control->cursor44));
            sint32 freeStart = (sint32)(first - reinterpret_cast<uintptr_t>(control->bufferStart));
            if (freeStart < (partial + 1))
            {
                control->state = 2;
                return;
            }
            memcpy(control->bufferStart, control->cursor44, partial);
            uint32 *marker = reinterpret_cast<uint32 *>(control->cursor44);
            marker[0] = 0xFFFFFFFF;
            marker[1] = 8;
            uint8 *next = control->bufferStart + partial;
            capacity = (sint32)(reinterpret_cast<uintptr_t>(control->cursor40) - reinterpret_cast<uintptr_t>(next) - 1);
            control->cursor44 = control->bufferStart;
            control->cursor48 = next;
        }
    }
    if (capacity < 0x2000)
    {
        control->state = 2;
        return;
    }
    NativeStreamRequest *request = control->unknown50;
    if (request->memoryBacked == 1)
    {
        sint32 offset = control->offset;
        sint32 size = request->sizeOrOffset;
        sint32 end = (offset + capacity);
        control->readLength = size < end ? (size - offset) : capacity;
        memcpy(control->cursor48, request->data, control->readLength);
        request->data += control->readLength;
        readcallback(0, 0, control);
        return;
    }
    control->readLength = 0x2000;
    uint32 operation = XPORT_UNPLACED::FILE::read(control->unknown9C, control->offset, control->cursor48, control->readLength, priority, control);
    control->fileOperation = operation;
    if (operation != 0)
        XPORT_UNPLACED::FILE::callbackop(operation, readcallback);
}

sint32 STREAM::overhead(sint32 requests, sint32 files, sint32 filters)
{
    FUNCTION_MARKER(0x800FCDE0, "NFS4.EXE");
    uint32 bytes = (uint32)sizeof(NativeStreamControl) + requests * (uint32)sizeof(NativeStreamRequest) + files * (uint32)sizeof(NativeStreamRule);
    bytes = (bytes + (uint32)sizeof(void *) - 1U) & (~(((uint32)sizeof(void *))) + 1);
    return (bytes + filters * (uint32)sizeof(NativeStreamHandle));
}

NativeStreamHandle *STREAM::create(sint32 requests, sint32 files, sint32 filters, void *buffer, sint32 size)
{
    FUNCTION_MARKER(0x800FCE14, "NFS4.EXE");
    sint32 headerBytes = overhead(requests, files, filters);
    if ((size - headerBytes) < 0x6000)
        return 0;
    if (requests < 2 || requests > 256 || (uint32)files - 1 >= 16)
        return 0;
    if (filters <= 0 || filters > files)
        return 0;
    uint8 *bytes = static_cast<uint8 *>(buffer);
    NativeStreamControl *control = static_cast<NativeStreamControl *>(buffer);
    control->magic = 0x4D525453;
    control->mutex = allocmutex();
    control->requests = reinterpret_cast<NativeStreamRequest *>(bytes + sizeof(NativeStreamControl));
    control->bufferEnd = bytes + size;
    control->priorityMask = 150;
    control->requestCount = requests;
    control->ruleCount = files;
    control->handleCount = filters;
    control->state = 0;
    control->greedyLevel = 0;
    control->greedyEnabled = 0;
    control->bufferUsage = 0;
    control->unknown4C = 0;
    control->unknown50 = 0;
    control->unknown54 = 0;
    control->rules = reinterpret_cast<NativeStreamRule *>(control->requests + requests);
    uintptr_t handles = reinterpret_cast<uintptr_t>(control->rules + files);
    handles = (handles + sizeof(void *) - 1U) & (~((uintptr_t)sizeof(void *) - 1));
    control->handles = reinterpret_cast<NativeStreamHandle *>(handles);
    control->bufferStart = reinterpret_cast<uint8 *>(control->handles + filters);
    control->priority = 50;
    control->cursor40 = control->bufferStart;
    control->cursor44 = control->bufferStart;
    control->cursor48 = control->bufferStart;
    control->unknown58 = control->requests;
    memset(control->unknown5C, 0, 64);
    control->unknown9C = 0;
    for (sint32 i = 0; i < requests; ++i)
    {
        NativeStreamRequest *request = &control->requests[i];
        request->index = i;
        request->state = 0;
        request->next = &control->requests[i + 1];
    }
    control->requests[requests - 1].next = 0;
    for (sint32 i = 0; i < files; ++i)
    {
        NativeStreamRule *rule = &control->rules[i];
        rule->mask = 0;
        rule->value = 0;
        rule->target = 1;
    }
    for (sint32 i = 0; i < filters; ++i)
    {
        NativeStreamHandle *handle = &control->handles[i];
        handle->control = control;
        handle->index = i + 1;
        handle->available = 0;
    }
    return control->handles;
}

void STREAM::destroy(NativeStreamHandle *handle)
{
    FUNCTION_MARKER(0x800FD144, "NFS4.EXE");
    NativeStreamControl *control;
    NativeStreamHandle *filter;
    if (validatehandle(handle, &control, &filter) != 0)
        return;
    kill(handle);
    while (control->state == 1)
    {
        if (iscurrentthread(0))
            systemtask(0);
        yieldthread(0);
    }
    MemMutex *mutex = control->mutex;
    control->magic = 0;
    freemutex(mutex);
    XPORT_UNPLACED::FILE::closesync(control->unknown9C, 100);
}

void STREAM::setpriority(NativeStreamHandle *handle, sint32 mask, sint32 priority)
{
    FUNCTION_MARKER(0x800FD1F8, "NFS4.EXE");
    NativeStreamControl *control;
    NativeStreamHandle *filter;
    if (validatehandle(handle, &control, &filter) == 0)
    {
        control->priorityMask = mask;
        control->priority = priority;
    }
}

void STREAM::setgreedylevel(NativeStreamHandle *handle, sint32 level)
{
    FUNCTION_MARKER(0x800FD248, "NFS4.EXE");
    NativeStreamControl *control;
    NativeStreamHandle *filter;
    if (validatehandle(handle, &control, &filter) == 0)
    {
        sint32 oldLevel = control->greedyLevel;
        sint32 usage = control->bufferUsage;
        bool wasGreedy = usage < oldLevel;
        bool isGreedy = usage < level;
        control->greedyLevel = level;
        if (wasGreedy != isGreedy)
            setgreedystate(handle, isGreedy);
    }
}

void STREAM::setgreedystate(NativeStreamHandle *handle, sint32 enabled)
{
    FUNCTION_MARKER(0x800FD2B4, "NFS4.EXE");
    NativeStreamControl *control;
    NativeStreamHandle *filter;
    if (validatehandle(handle, &control, &filter) == 0)
    {
        control->greedyEnabled = enabled;
        if (enabled != 0 && control->state == 1)
        {
            uint32 operation = control->fileOperation;
            sint32 priority = control->priority;
            XPORT_UNPLACED::FILE::priorityop(operation, priority);
        }
    }
}

uint32 STREAM::queuefile(NativeStreamHandle *handle, const char *filename, sint32 offset, uint32 endMarker)
{
    FUNCTION_MARKER(0x800FD314, "NFS4.EXE");
    NativeStreamControl *control;
    NativeStreamHandle *filter;
    if (validatehandle(handle, &control, &filter) != 0)
        return 0;
    NativeStreamRequest *request = getfreerequest(control);
    if (request == 0)
        return 0;
    request->memoryBacked = 0;
    strncpy(request->name, filename, 63);
    request->sizeOrOffset = offset;
    request->endMarker = endMarker;
    queuerequest(control, request);
    sint32 interruptsEnabled = xport_bios_enter_critical();
    sint32 state = control->state;
    if (state == 0)
        control->state = 1;
    if (interruptsEnabled != 0)
        xport_bios_exit_critical();
    if (state == 0)
        startnextrequest(control, control->greedyEnabled != 0 ? control->priority : control->priorityMask);
    return request->index;
}

uint32 STREAM::queuemem(NativeStreamHandle *handle, const void *data, sint32 size, uint32 endMarker)
{
    FUNCTION_MARKER(0x800FD428, "NFS4.EXE");
    NativeStreamControl *control;
    NativeStreamHandle *filter;
    if (validatehandle(handle, &control, &filter) != 0)
        return 0;
    NativeStreamRequest *request = getfreerequest(control);
    if (request == 0)
        return 0;
    if (size == 0)
    {
        const uint8 *cursor = static_cast<const uint8 *>(data);
        uint32 tag;
        memcpy(&tag, cursor, sizeof(tag));
        while (tag != endMarker)
        {
            uint32 length;
            memcpy(&length, cursor + 4, sizeof(length));
            cursor += length;
            size = (size + length);
            memcpy(&tag, cursor, sizeof(tag));
        }
        uint32 length;
        memcpy(&length, cursor + 4, sizeof(length));
        size = (size + length);
    }
    request->memoryBacked = 1;
    request->data = static_cast<const uint8 *>(data);
    request->sizeOrOffset = size;
    request->endMarker = endMarker;
    queuerequest(control, request);
    sint32 interruptsEnabled = xport_bios_enter_critical();
    sint32 state = control->state;
    if (state == 0)
        control->state = 1;
    if (interruptsEnabled != 0)
        xport_bios_exit_critical();
    if (state == 0)
        startnextrequest(control, 0);
    return request->index;
}

void STREAM::cancelrequest(NativeStreamHandle *handle, uint32 key)
{
    FUNCTION_MARKER(0x800FD554, "NFS4.EXE");
    NativeStreamControl *control;
    NativeStreamHandle *filter;
    if (validatehandle(handle, &control, &filter) != 0)
        return;
    sint32 interruptsEnabled = xport_bios_enter_critical();
    NativeStreamRequest *request = locaterequest(control, key);
    bool skip = true;
    uint8 *oldest = 0;
    uint8 *first = 0;
    uint8 *last = 0;
    if (request != 0 && request->state != 4)
    {
        if (request->state == 1)
            freerequest(control, request);
        else
        {
            request->state = 4;
            oldest = control->cursor40;
            first = request == control->unknown4C ? oldest : request->firstChunk;
            NativeStreamRequest *next = request->next;
            last = next == 0 || next->state == 1 ? control->cursor44 : next->firstChunk;
            skip = false;
        }
    }
    if (interruptsEnabled != 0)
        xport_bios_exit_critical();
    if (skip)
        return;
    for (sint32 index = 0; index < control->handleCount; ++index)
    {
        filter = &control->handles[index];
        if ((sint32)filter->available <= 0)
            continue;
        if (inbetween(reinterpret_cast<uintptr_t>(oldest), reinterpret_cast<uintptr_t>(first), reinterpret_cast<uintptr_t>(filter->chunk)))
        {
            uint32 tag = filter->index << 24;
            uint8 *cursor = first;
            while (cursor != last)
            {
                uint32 *chunk = reinterpret_cast<uint32 *>(cursor);
                if (chunk[0] == 0xFFFFFFFF)
                    cursor = control->bufferStart;
                else
                {
                    uint32 length = chunk[1] & 0xFFFFFF;
                    if ((chunk[1] & 0xFF000000) == tag)
                    {
                        interruptsEnabled = xport_bios_enter_critical();
                        filter->available -= length;
                        if (interruptsEnabled != 0)
                            xport_bios_exit_critical();
                        decbufferusage(control, length);
                        chunk[0] = 0xFFFFFFFE;
                        chunk[1] = length;
                    }
                    cursor += length;
                }
            }
        }
        else
        {
            while (inbetween(reinterpret_cast<uintptr_t>(first), reinterpret_cast<uintptr_t>(last), reinterpret_cast<uintptr_t>(filter->chunk)))
            {
                uint32 *chunk = get(filter);
                release(filter, chunk);
                if ((sint32)filter->available <= 0)
                    break;
            }
        }
    }
}

void STREAM::kill(NativeStreamHandle *handle)
{
    FUNCTION_MARKER(0x800FD808, "NFS4.EXE");
    NativeStreamControl *control;
    NativeStreamHandle *filter;
    if (validatehandle(handle, &control, &filter) != 0)
        return;
    NativeStreamRequest *tail = control->unknown54;
    if (tail == 0)
        return;
    while ((uint32)tail->state - 1 < 2)
    {
        cancelrequest(handle, tail->index);
        tail = control->unknown54;
    }
    while (control->unknown4C != control->unknown50)
        freerequest(control, control->unknown4C);
    control->unknown50->state = 4;
    for (sint32 index = 0; index < control->handleCount; ++index)
        control->handles[index].available = 0;
    decbufferusage(control, (uint32)control->bufferUsage);
    uint8 *cursor = control->cursor40;
    while (cursor != control->cursor44)
    {
        uint32 *chunk = reinterpret_cast<uint32 *>(cursor);
        if (chunk[0] == 0xFFFFFFFF)
            cursor = control->bufferStart;
        else
        {
            uint32 length = chunk[1] & 0xFFFFFF;
            chunk[0] = 0xFFFFFFFE;
            chunk[1] = length;
            cursor += length;
        }
    }
    if (control->state == 2)
        control->state = 0;
}

uint32 *STREAM::get(NativeStreamHandle *handle)
{
    FUNCTION_MARKER(0x800FD9AC, "NFS4.EXE");
    NativeStreamControl *control;
    NativeStreamHandle *filter;
    if (validatehandle(handle, &control, &filter) != 0 || filter->available == 0)
        return 0;
    uint32 *chunk = filter->chunk;
    uint32 length = chunk[1] & 0xFFFFFF;
    chunk[1] = length;
    sint32 interruptsEnabled = xport_bios_enter_critical();
    sint32 remaining = (filter->available - length);
    filter->available = remaining;
    if (interruptsEnabled != 0)
        xport_bios_exit_critical();
    if (remaining > 0)
    {
        uint32 tag = filter->index << 24;
        uint32 *next = reinterpret_cast<uint32 *>(reinterpret_cast<uint8 *>(chunk) + length);
        while ((next[1] & 0xFF000000) != tag)
        {
            if (next[0] == 0xFFFFFFFF)
                next = reinterpret_cast<uint32 *>(control->bufferStart);
            else
                next = reinterpret_cast<uint32 *>(reinterpret_cast<uint8 *>(next) + (next[1] & 0xFFFFFF));
        }
        filter->chunk = next;
    }
    return chunk;
}

void STREAM::release(NativeStreamHandle *handle, void *chunk)
{
    FUNCTION_MARKER(0x800FDAD0, "NFS4.EXE");
    NativeStreamControl *control;
    NativeStreamHandle *filter;
    if (validatehandle(handle, &control, &filter) != 0)
        return;
    uintptr_t address = reinterpret_cast<uintptr_t>(chunk);
    if (address < reinterpret_cast<uintptr_t>(control->bufferStart) || address > reinterpret_cast<uintptr_t>(control->bufferEnd) - 8)
        return;
    uint32 *header = static_cast<uint32 *>(chunk);
    if (header[0] == 0xFFFFFFFE)
        return;
    uint32 length = header[1];
    header[0] = 0xFFFFFFFE;
    decbufferusage(control, length);
    sint32 interruptsEnabled = xport_bios_enter_critical();
    sint32 state = control->state;
    if (state == 2)
        control->state = 1;
    if (interruptsEnabled != 0)
        xport_bios_exit_critical();
    if (state == 2)
        restartstream(control, control->greedyEnabled != 0 ? control->priority : control->priorityMask);
}

uint32 STREAM::gettable(NativeStreamHandle *handle)
{
    FUNCTION_MARKER(0x800FDBCC, "NFS4.EXE");
    NativeStreamControl *control;
    NativeStreamHandle *filter;
    if (validatehandle(handle, &control, &filter) != 0)
        return 0;
    return filter->available;
}

sint32 STREAM::state(NativeStreamHandle *handle)
{
    FUNCTION_MARKER(0x800FDC04, "NFS4.EXE");
    NativeStreamControl *control;
    NativeStreamHandle *filter;
    if (validatehandle(handle, &control, &filter) != 0)
        return 0;
    return control->state;
}

sint32 STREAM::buffersize(NativeStreamHandle *handle)
{
    FUNCTION_MARKER(0x800FDC98, "NFS4.EXE");
    NativeStreamControl *control;
    NativeStreamHandle *filter;
    if (validatehandle(handle, &control, &filter) != 0)
        return 0;
    return control->bufferEnd - control->bufferStart;
}
