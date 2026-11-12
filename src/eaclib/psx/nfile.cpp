#include "xport.h"
#include "eaclib/psx/internal.h"
#include "nfile.h"

NativeFileDevicePrefix gFileDevice;
uint32 fileOperationGeneration;

sint32 XPORT_UNPLACED::FILE::init(sint32 fileCount, sint32 bufferSize, sint32 operationCount)
{
    FUNCTION_MARKER(0x800EBBF4, "NFS4.EXE");
    if (gFileDevice.operationCount != 0)
        return 0;
    sint32 bytes = overhead(fileCount, bufferSize, operationCount);
    void *memory = reservememadr("File Sys", bytes, 0);
    return initwithmem(fileCount, bufferSize, operationCount, memory);
}

sint32 XPORT_UNPLACED::FILE::initwithmem(sint32 fileCount, sint32 bufferSize, sint32 operationCount, void *memory)
{
    FUNCTION_MARKER(0x800EBC78, "NFS4.EXE");
    if (fileCount == 0)
        fileCount = 24;
    if (bufferSize == 0)
        bufferSize = 2048;
    if (operationCount == 0)
        operationCount = 10;
    if (gFileDevice.operationCount != 0)
        return 0;
    gFileDevice.operationCount = operationCount;
    gFileDevice.fileCount = fileCount;
    gFileDevice.priorityLimit = 255;
    gFileDevice.operations = static_cast<NativeFileOperation *>(memory);
    sint32 bytes = overhead(fileCount, bufferSize, operationCount);
    blockclear(memory, bytes);
    gFileDevice.files = reinterpret_cast<NativeFileSlot *>(static_cast<char *>(memory) + (uint32)operationCount * sizeof(NativeFileOperation));
    initfileio();
    return 1;
}

sint32 XPORT_UNPLACED::FILE::overhead(sint32 fileCount, sint32 bufferSize, sint32 operationCount)
{
    FUNCTION_MARKER(0x800EBD74, "NFS4.EXE");
    if (fileCount == 0)
        fileCount = 24;
    if (bufferSize == 0)
        bufferSize = 2048;
    if (operationCount == 0)
        operationCount = 10;
    uint32 bytes = fileCount * (uint32)sizeof(NativeFileSlot) + operationCount * (uint32)sizeof(NativeFileOperation);
    bytes += (uint32)bufferSize * 20;
    return bytes;
}

sint32 XPORT_UNPLACED::FILE::opstatus(sint32 handle)
{
    FUNCTION_MARKER(0x800EBDC4, "NFS4.EXE");
    if (handle == 0)
        return -3;
    NativeFileOperation *operation = gFileDevice.operations + ((uint32)handle >> 24);
    if (((uint32)handle & 0xFFFFF) != (operation->key & 0xFFFFF))
        return -3;
    return operation->status;
}

sint32 XPORT_UNPLACED::FILE::operror(sint32 handle)
{
    FUNCTION_MARKER(0x800EBE1C, "NFS4.EXE");
    NativeFileOperation *operation = gFileDevice.operations + ((uint32)handle >> 24);
    return operation->error;
}

void XPORT_UNPLACED::FILE::callbackop(uint32 operation, NativeFileCompletion callback)
{
    FUNCTION_MARKER(0x800EBE4C, "NFS4.EXE");
    NativeFileOperation *request = gFileDevice.operations + (operation >> 24);
    sint32 status = request->status;
    request->callback = callback;
    if (status != 0)
    {
        ++gFileDevice.callbackDepth;
        callback(operation, request->status, request->userData);
        --gFileDevice.callbackDepth;
    }
}

void XPORT_UNPLACED::FILE::priorityop(uint32 operation, sint32 priority)
{
    FUNCTION_MARKER(0x800EBECC, "NFS4.EXE");
    NativeFileOperation *request = gFileDevice.operations + (operation >> 24);
    sint32 interruptsWereEnabled = EnterCriticalSectionPSX();
    sint32 previousPriority = request->priority;
    request->priority = priority;
    if ((sint32)gFileDevice.queuedCount >= 2 && request != gFileDevice.active && request->status == 0 && previousPriority != priority)
    {
        NativeFileOperation *previous = 0;
        NativeFileOperation *current = gFileDevice.queue;
        while (current != 0 && current != request)
        {
            previous = current;
            current = current->next;
        }
        if (previous != 0)
            previous->next = request->next;
        else
            gFileDevice.queue = request->next;
        request->next = gFileDevice.queue;
        previous = 0;
        while (request->next != 0)
        {
            current = request->next;
            if (request->priority < current->priority)
                break;
            previous = current;
            request->next = current->next;
        }
        if (previous != 0)
            previous->next = request;
        else
            gFileDevice.queue = request;
    }
    if (interruptsWereEnabled != 0)
        ExitCriticalSection();
}

void XPORT_UNPLACED::FILE::cancelop(uint32 operation)
{
    FUNCTION_MARKER(0x800EC008, "NFS4.EXE");
    sint32 interruptsWereEnabled = EnterCriticalSectionPSX();
    if (operation != 0)
    {
        NativeFileOperation *request = gFileDevice.operations + (operation >> 24);
        uint32 type = (operation >> 20) & 15;
        if ((operation & 0xFFFFF) == (request->key & 0xFFFFF) && type != 3 && type != 10)
        {
            sint32 action = 0;
            if (gFileDevice.active == request)
            {
                request->cancelled = 1;
                action = 1;
            }
            else
            {
                if (request->status != 1)
                {
                    NativeFileOperation *previous = 0;
                    NativeFileOperation *current = gFileDevice.queue;
                    while (current != 0 && current != request)
                    {
                        previous = current;
                        current = current->next;
                    }
                    if (current == 0)
                    {
                        if (interruptsWereEnabled != 0)
                            ExitCriticalSection();
                        return;
                    }
                    if (previous != 0)
                        previous->next = request->next;
                    else
                        gFileDevice.queue = request->next;
                    --gFileDevice.queuedCount;
                    action = 2;
                }
                request->status = -1;
            }
            if (action == 2 && request->callback != 0)
                request->callback(request->key, -1, request->userData);
        }
    }
    if (interruptsWereEnabled != 0)
        ExitCriticalSection();
}

sint32 XPORT_UNPLACED::FILE::waitop(uint32 operation)
{
    FUNCTION_MARKER(0x800EC1BC, "NFS4.EXE");
    uint32 index = operation >> 24;
    NativeFileOperation *request = gFileDevice.operations + index;
    if (operation == 0 || (operation & 0xFFFFF) != (request->key & 0xFFFFF))
        return -3;
    while (request->status == 0)
    {
        systemtask(0);
        if ((operation & 0xFFFFF) != (gFileDevice.operations[index].key & 0xFFFFF))
            return -3;
    }
    return request->status;
}

intptr XPORT_UNPLACED::FILE::completeop(sint32 handle)
{
    FUNCTION_MARKER(0x800EC2B0, "NFS4.EXE");
    NativeFileOperation *operation = gFileDevice.operations + ((uint32)handle >> 24);
    intptr result = 0;
    if (operation->status == 1)
    {
        switch ((operation->key >> 20) & 15)
        {
        case 2:
        case 9:
            result = reinterpret_cast<intptr>(operation->file);
            break;
        case 3:
        case 7:
        case 10:
            result = operation->status;
            break;
        case 4:
        case 5:
            result = operation->length;
            break;
        case 6:
        case 8:
            result = operation->offset;
            break;
        }
    }
    freeop(operation);
    return result;
}

uint32 XPORT_UNPLACED::FILE::open(const char *name, sint32 mode, sint32 priority, void *userData)
{
    FUNCTION_MARKER(0x800EC36C, "NFS4.EXE");
    NativeFileOperation *operation = reserveop();
    operation->userData = userData;
    operation->offset = mode;
    operation->priority = priority;
    operation->key = (operation->key & 0xFF0FFFFF) | 0x200000;
    operation->file = reservehandle();
    if (operation->file == 0)
    {
        operation->error = 2;
        iFILE_perror(operation);
    }
    (void)strlen(name);
    strncpy(operation->file->name, name, 64);
    iFILE_ExecCommand(operation);
    return operation->key;
}

uint32 XPORT_UNPLACED::FILE::close(NativeFileSlot *filehandle, sint32 priority, void *userData)
{
    FUNCTION_MARKER(0x800EC42C, "NFS4.EXE");
    NativeFileOperation *operation = reserveop();
    NativeBigArchive *archive = gFileDevice.archives;
    operation->priority = priority;
    operation->userData = userData;
    operation->file = filehandle;
    operation->key = (operation->key & 0xFF0FFFFF) | 0x300000;
    while (archive != 0)
    {
        if (archive->file == operation->file)
        {
            operation->error = 3;
            iFILE_perror(operation);
            break;
        }
        archive = archive->next;
    }
    iFILE_ExecCommand(operation);
    return operation->key;
}

sint32 XPORT_UNPLACED::FILE::read(NativeFileSlot *filehandle, sint32 offset, void *destination, sint32 size, sint32 priority, void *userData)
{
    FUNCTION_MARKER(0x800EC4EC, "NFS4.EXE");
    NativeFileOperation *operation = reserveop();
    operation->key = (operation->key & 0xFF0FFFFF) | 0x400000;
    operation->userData = userData;
    operation->priority = priority;
    if (filehandle == 0)
    {
        operation->error = 6;
        iFILE_perror(operation);
    }
    NativeFileSlot *file = filehandle;
    operation->file = file;
    if (file->length < (offset + size))
    {
        size = (file->length - offset);
        if (size < 0)
            size = 0;
    }
    operation->length = size;
    operation->destination = destination;
    operation->offset = offset;
    iFILE_ExecCommand(operation);
    return operation->key;
}

uint32 XPORT_UNPLACED::FILE::size(NativeFileSlot *filehandle, sint32 priority, void *userData)
{
    FUNCTION_MARKER(0x800EC5D0, "NFS4.EXE");
    NativeFileOperation *operation = reserveop();
    operation->userData = userData;
    operation->priority = priority;
    operation->key = (operation->key & 0xFF0FFFFF) | 0x600000;
    if (filehandle == 0)
    {
        operation->error = 6;
        iFILE_perror(operation);
    }
    operation->file = filehandle;
    iFILE_ExecCommand(operation);
    return operation->key;
}

void iFILE::addbigreadcallback(uint32 operation, uint32 status, void *userData)
{
    FUNCTION_MARKER(0x800EC660, "NFS4.EXE");
    NativeBigArchive *archive = static_cast<NativeBigArchive *>(userData);
    NativeFileOperation *request = gFileDevice.operations + (operation >> 24);
    NativeFileOperation *owner = archive->operation;
    NativeFileSlot *file = request->file;
    sint32 priority = request->priority;
    archive->file = file;
    owner->file = file;
    XPORT_UNPLACED::FILE::completeop(operation);
    if (typeofbigfile(archive->indexData) == 0)
        purgememadr(archive->indexData);
    sint32 size = sizeofbigfileheader(archive->indexData);
    if (getblocksize(archive->indexData) < size)
    {
        char *expanded = static_cast<char *>(reservememadr("bigfile buf", size, 0x10));
        blockmove(archive->indexData, expanded, 0x800);
        purgememadr(archive->indexData);
        archive->indexData = expanded;
        uint32 next = XPORT_UNPLACED::FILE::read(archive->file, 0x800, expanded + 0x800, (size - 0x800), priority, archive);
        XPORT_UNPLACED::FILE::callbackop(next, addbigreadcallback);
    }
    else
    {
        archive->next = gFileDevice.archives;
        gFileDevice.archives = archive;
        iFILE_ExecCommand(archive->operation);
    }
}

void iFILE::addbigopencallback(uint32 operation, uint32 status, void *userData)
{
    FUNCTION_MARKER(0x800EC7A0, "NFS4.EXE");
    NativeBigArchive *archive = static_cast<NativeBigArchive *>(userData);
    sint32 priority = gFileDevice.operations[operation >> 24].priority;
    NativeFileSlot *file = reinterpret_cast<NativeFileSlot *>(XPORT_UNPLACED::FILE::completeop(operation));
    if (status == 1)
    {
        uint32 next = XPORT_UNPLACED::FILE::read(file, 0, archive->indexData, 0x800, priority, archive);
        XPORT_UNPLACED::FILE::callbackop(next, addbigreadcallback);
    }
    else
    {
        archive->operation->error = 4;
        iFILE_ExecCommand(archive->operation);
    }
}

uint32 XPORT_UNPLACED::FILE::addbig(const char *name, uint32 flags, sint32 priority, void *userData)
{
    FUNCTION_MARKER(0x800EC858, "NFS4.EXE");
    NativeBigArchive *existing = gFileDevice.archives;
    NativeFileOperation *operation = reserveop();
    operation->userData = userData;
    operation->priority = priority;
    operation->key = (operation->key & 0xFF0FFFFF) | 0x900000;
    iscurrentthread(0);
    for (; existing != 0; existing = existing->next) {
        if (strncmp(name, existing->file->name, 64) == 0) {
            operation->file = existing->file;
            iFILE_ExecCommand(operation);
            return operation->key;
        }
    }
    NativeBigArchive *archive = ::new (reservememadr(name, sizeof(NativeBigArchive), flags)) NativeBigArchive;
    archive->operation = operation;
    archive->indexData = static_cast<char *>(reservememadr("bigfile header", 0x800, 0x10));
    uint32 next = open(name, 1, priority, archive);
    callbackop(next, iFILE::addbigopencallback);
    return operation->key;
}

void iFILE::delbigclosecallback(uint32 operation, uint32 status, void *userData)
{
    FUNCTION_MARKER(0x800EC980, "NFS4.EXE");
    NativeFileOperation *owner = static_cast<NativeFileOperation *>(userData);
    XPORT_UNPLACED::FILE::completeop(operation);
    iFILE_ExecCommand(owner);
}

uint32 XPORT_UNPLACED::FILE::delbig(NativeFileSlot *handle, sint32 priority, void *userData)
{
    FUNCTION_MARKER(0x800EC9AC, "NFS4.EXE");
    NativeBigArchive *archive = gFileDevice.archives;
    NativeFileOperation *operation = reserveop();
    NativeFileSlot *file = handle;
    NativeFileSlot *slot = gFileDevice.files;
    operation->userData = userData;
    operation->priority = priority;
    operation->key = (operation->key & 0xFF0FFFFF) | 0xA00000;
    if (file->archiveOffset != 0)
        operation->error = 1;
    for (sint32 index = 0; index < gFileDevice.fileCount; ++index, ++slot)
    {
        if (slot != 0 && slot->archiveOffset != 0 && slot->archive->file == file)
        {
            operation->status = -2;
            operation->error = 1;
            break;
        }
    }
    NativeBigArchive *previous = 0;
    while (archive != 0 && archive->file != file)
    {
        previous = archive;
        archive = archive->next;
    }
    if (archive == 0)
        operation->error = 1;
    NativeFileSlot *closeFile = archive->file;
    if (previous == 0)
        gFileDevice.archives = archive->next;
    else
        previous->next = archive->next;
    purgememadr(archive->indexData);
    purgememadr(archive);
    uint32 next = close(closeFile, priority, operation);
    callbackop(next, iFILE::delbigclosecallback);
    return operation->key;
}

NativeFileAtomicResult XPORT_UNPLACED::FILE::atomic(NativeFileAtomicResult (*callback)(sint32, void *), uint32 unused, sint32 priority, void *userData)
{
    FUNCTION_MARKER(0x800ECB40, "NFS4.EXE");
    sint32 previousPriority = gFileDevice.priorityLimit;
    gFileDevice.priorityLimit = priority;
    NativeFileAtomicResult result = callback(priority, userData);
    gFileDevice.priorityLimit = previousPriority;
    iFILE_ExecCommand(0);
    return result;
}

extern "C" void iFILE_ExecCommand(NativeFileOperation *operation)
{
    FUNCTION_MARKER(0x800ECB98, "NFS4.EXE");
    sint32 interruptsWereEnabled = EnterCriticalSectionPSX();
    if (operation != 0)
    {
        NativeFileOperation *previous = 0;
        operation->next = gFileDevice.queue;
        if (operation->next != 0)
        {
            sint32 priority = operation->priority;
            do
            {
                NativeFileOperation *current = operation->next;
                if (priority < current->priority)
                    break;
                previous = current;
                operation->next = current->next;
            } while (operation->next != 0);
        }
        if (previous != 0)
            previous->next = operation;
        else
            gFileDevice.queue = operation;
        ++gFileDevice.queuedCount;
    }
    if (gFileDevice.active != 0)
    {
        if (interruptsWereEnabled != 0)
            ExitCriticalSection();
        return;
    }
    operation = gFileDevice.queue;
    if (operation == 0 || gFileDevice.priorityLimit < operation->priority)
        operation = 0;
    else
    {
        --gFileDevice.queuedCount;
        gFileDevice.queue = operation->next;
    }
    gFileDevice.active = operation;
    if (interruptsWereEnabled != 0)
        ExitCriticalSection();
    if (operation == 0)
        return;

    switch ((operation->key >> 20) & 15)
    {
    case 2:
    case 8:
    {
        char memberName[64];
        char archiveName[64];
        memberName[0] = 0;
        char *separator = strchr(operation->file->name, '|');
        uint32 searchMode;
        if (separator != 0)
        {
            searchMode = 2;
            if (operation->file->name[0] != '|')
            {
                separator = strchr(operation->file->name, '|');
                searchMode = 4;
                archiveName[0] = 0;
                ptrdiff_t length = separator - operation->file->name;
                strncpy(archiveName, operation->file->name, length);
                archiveName[length] = 0;
            }
            separator = strchr(operation->file->name, '|');
            strcpy(memberName, separator + 1);
        }
        else
        {
            searchMode = 1;
            if ((operation->offset & 1) != 0)
            {
                searchMode = 3;
                strcpy(memberName, operation->file->name);
            }
        }
        sint32 successful = 0;
        if ((searchMode & 1) != 0)
        {
            NativeFileSlot *file = operation->file;
            if (openfile(file->name, operation->offset, &file->stream) != 0)
            {
                successful = 1;
                if (((operation->key >> 20) & 15) == 8)
                    closefile(operation->file->stream);
                else
                {
                    sint32 length = getfilesize(operation->file->stream);
                    operation->file->length = length;
                }
            }
        }
        if (successful == 0 && (searchMode & 6) != 0)
        {
            NativeBigArchive *archive = gFileDevice.archives;
            while (archive != 0)
            {
                if ((searchMode & 4) == 0 || strcmp(archive->file->name, archiveName) == 0)
                {
                    sint32 offset;
                    sint32 length;
                    if (locatebigentryz(archive->indexData, memberName, 0, &offset, &length) != 0)
                    {
                        operation->file->archive = archive;
                        operation->file->length = length;
                        operation->file->archiveOffset = offset;
                        successful = 1;
                    }
                }
                archive = archive->next;
                if (successful != 0)
                    break;
            }
        }
        if (((operation->key >> 20) & 15) == 8)
        {
            freehandle(operation->file);
            operation->offset = successful;
        }
        iFILE::CommandCompleteCallback(successful);
        return;
    }
    case 3:
    {
        NativeFileSlot *file = operation->file;
        operation->error = 0;
        if (file != 0)
        {
            if (file->archiveOffset == 0)
                operation->error = closefile(file->stream);
            freehandle(operation->file);
            operation->file = 0;
        }
        iFILE::CommandCompleteCallback(operation->error == 0 ? 1 : 0);
        return;
    }
    case 4:
        if (operation->length <= 0)
            iFILE::CommandCompleteCallback(1);
        else
        {
            NativeFileSlot *file = operation->file;
            if (file->archiveOffset != 0)
            {
                sint32 offset = (file->archiveOffset + operation->offset);
                readfile(file->archive->file->stream, operation->destination, offset, operation->length);
            }
            else
                readfile(file->stream, operation->destination, operation->offset, operation->length);
        }
        return;
    case 5:
        writefile(operation->file->stream, operation->destination, operation->offset, operation->length);
        return;
    case 6:
        operation->offset = operation->file->length;
        iFILE::CommandCompleteCallback(1);
        return;
    case 7:
    case 9:
    case 10:
        iFILE::CommandCompleteCallback(operation->error == 0 ? 1 : 0);
        return;
    default:
        return;
    }
}

extern "C" void iFILE_perror(NativeFileOperation *)
{
    FUNCTION_MARKER(0x800ED0D4, "NFS4.EXE");
}

extern "C" NativeFileOperation *reserveop(void)
{
    FUNCTION_MARKER(0x800ED0DC, "NFS4.EXE");
    sint32 interruptsWereEnabled = EnterCriticalSectionPSX();
    sint32 index = 0;
    for (; index < gFileDevice.operationCount; ++index)
    {
        NativeFileOperation *operation = gFileDevice.operations + index;
        uint32 key = operation->key;
        if (((key >> 20) & 15) != 0)
            continue;
        operation->key = (key & 0xFF0FFFFF) | 0x100000;
        operation = gFileDevice.operations + index;
        operation->key = (operation->key & 0x00FFFFFF) | ((uint32)(uint8)index << 24);
        operation = gFileDevice.operations + index;
        uint32 generation = fileOperationGeneration;
        uint32 keyGeneration = generation & 0xFFFFF;
        key = operation->key;
        fileOperationGeneration = generation + 1;
        operation->key = (key & 0xFFF00000) | keyGeneration;
        if ((sint32)fileOperationGeneration > 0xFFFFF)
            fileOperationGeneration = 0;
        break;
    }
    if (interruptsWereEnabled != 0)
        ExitCriticalSection();
    if (index == gFileDevice.operationCount)
        return 0;
    return gFileDevice.operations + index;
}

extern "C" void freeop(NativeFileOperation *operation)
{
    FUNCTION_MARKER(0x800ED1F8, "NFS4.EXE");
    sint32 interruptsWereEnabled = EnterCriticalSectionPSX();
    blockclear(operation, sizeof(*operation));
    if (interruptsWereEnabled != 0)
        ExitCriticalSection();
}

extern "C" NativeFileSlot *reservehandle(void)
{
    FUNCTION_MARKER(0x800ED240, "NFS4.EXE");
    sint32 interruptsWereEnabled = EnterCriticalSectionPSX();
    sint32 index = 0;
    sint32 count = gFileDevice.fileCount;
    if (count > 0)
    {
        NativeFileSlot *files = gFileDevice.files;
        for (; index < count; ++index)
        {
            if (files[index].descriptor == 0)
            {
                files[index].descriptor = 1;
                break;
            }
        }
    }
    if (interruptsWereEnabled != 0)
        ExitCriticalSection();
    if (index == gFileDevice.fileCount)
        return 0;
    return gFileDevice.files + index;
}

extern "C" void freehandle(NativeFileSlot *file)
{
    FUNCTION_MARKER(0x800ED2F0, "NFS4.EXE");
    sint32 interruptsWereEnabled = EnterCriticalSectionPSX();
    blockclear(file, sizeof(*file));
    if (interruptsWereEnabled != 0)
        ExitCriticalSection();
}

void iFILE::CommandCompleteCallback(sint32 successful)
{
    FUNCTION_MARKER(0x800ED020, "NFS4.EXE");
    NativeFileOperation *operation = gFileDevice.active;
    if (operation == 0)
        return;
    sint32 status = -1;
    if (operation->cancelled == 0)
        status = successful != 0 ? 1 : -2;
    operation->status = status;
    gFileDevice.active = 0;
    if (operation->callback != 0)
    {
        ++gFileDevice.callbackDepth;
        NativeFileCompletion callback = operation->callback;
        callback(operation->key, operation->status, operation->userData);
        --gFileDevice.callbackDepth;
    }
    if (gFileDevice.callbackDepth == 0)
        iFILE_ExecCommand(0);
}
