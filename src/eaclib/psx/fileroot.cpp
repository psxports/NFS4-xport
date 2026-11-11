#include "xport.h"
#include "eaclib/psx/internal.h"

uint32 currentfilesystem;
uint32 availablefilesystems;

extern "C" void initfileio(void)
{
    FUNCTION_MARKER(0x800F3A34, "NFS4.EXE");
    availablefilesystems |= 1;
    currentfilesystem = 1;
}

static sint32 ResolveNativeDataPath(const char *name, char *path, size_t capacity)
{
    if (name == 0 || capacity < 6)
        return 0;
    sint32 filesystem = currentfilesystem;
    if (strncmp(name, "cdrom:", 6) == 0)
    {
        if ((availablefilesystems & 1) == 0)
            return -1;
        filesystem = 1;
        name += 6;
    }
    else if (strncmp(name, "sim:", 4) == 0)
    {
        if ((availablefilesystems & 2) == 0)
            return -1;
        filesystem = 2;
        name += 4;
    }
    if (filesystem != 1 && filesystem != 2)
        return -1;
    memcpy(path, "DATA/", 5);
    size_t used = 5;
    bool first = true;
    while (*name != 0 && *name != ';')
    {
        while (*name == '/' || *name == '\\')
            ++name;
        const char *component = name;
        while (*name != 0 && *name != ';' && *name != '/' && *name != '\\')
            ++name;
        size_t length = name - component;
        if (length == 0)
            break;
        if (length == 1 && component[0] == '.')
            continue;
        if (length == 2 && component[0] == '.' && component[1] == '.')
            return 0;
        if (used > 5)
        {
            if (used + 1 >= capacity)
                return 0;
            path[used++] = '/';
        }
        if (length >= capacity - used)
            return 0;
        size_t start = used;
        for (size_t index = 0; index < length; ++index)
        {
            uint8 value = (uint8)component[index];
            if (value == ':')
                return -1;
            if (value >= 'a' && value <= 'z')
                value -= 'a' - 'A';
            path[used++] = (char)value;
        }
        if (first && length == 4 && memcmp(path + start, "DATA", 4) == 0)
            used = 5;
        first = false;
    }
    if (used == 5)
        return 0;
    path[used] = 0;
    return filesystem;
}

extern "C" sint32 openfile(const char *name, uint32 mode, ::FILE **handle)
{
    FUNCTION_MARKER(0x800F3BE0, "NFS4.EXE");
    char path[256];
    sint32 filesystem = ResolveNativeDataPath(name, path, sizeof(path));
    if (filesystem < 0)
        return 0;
    ::FILE *stream = 0;
    if (filesystem == 1)
        stream = xport_fopen(path, "rb");
    else if (filesystem == 2)
    {
        switch (mode & 7)
        {
        case 0:
            stream = xport_fopen(path, "r+b");
            break;
        case 1:
            stream = xport_fopen(path, "rb");
            break;
        case 2:
            stream = xport_fopen(path, "rb");
            if (stream == 0)
                stream = xport_fopen(path, "wb");
            break;
        case 4:
            stream = xport_fopen(path, "rb");
            if (stream != 0)
            {
                fclose(stream);
                stream = xport_fopen(path, "wb");
            }
            break;
        case 6:
            stream = xport_fopen(path, "wb");
            break;
        default:
            break;
        }
    }
    *handle = stream;
    return stream != 0 ? 1 : 0;
}

extern "C" sint32 closefile(::FILE *handle)
{
    FUNCTION_MARKER(0x800F3E84, "NFS4.EXE");
    return handle != 0 ? fclose(handle) : 0;
}

extern "C" sint32 readfile(::FILE *handle, void *destination, sint32 offset, sint32 length)
{
    FUNCTION_MARKER(0x800F3EE0, "NFS4.EXE");
    sint32 successful = 0;
    if (handle != 0 && offset >= 0 && length >= 0 && (destination != 0 || length == 0))
    {
#if !defined(XPORT_XCRT)
        clearerr(handle);
#endif
        if (fseek(handle, offset, SEEK_SET) == 0)
        {
            size_t received = xport_fread(destination, 1, (size_t)length, handle);
            successful = received == (size_t)length ? 1 : 0;
#if !defined(XPORT_XCRT)
            if (ferror(handle) != 0)
                successful = 0;
#endif
        }
    }
    iFILE::CommandCompleteCallback(successful);
    return successful;
}

extern "C" sint32 writefile(::FILE *handle, const void *source, sint32 offset, sint32 length)
{
    FUNCTION_MARKER(0x800F4020, "NFS4.EXE");
    sint32 successful = 0;
    if (handle != 0 && offset >= 0 && length >= 0 && (source != 0 || length == 0))
    {
#if !defined(XPORT_XCRT)
        clearerr(handle);
#endif
        if (fseek(handle, offset, SEEK_SET) == 0)
        {
            size_t written = fwrite(source, 1, (size_t)length, handle);
            successful = written == (size_t)length ? 1 : 0;
#if !defined(XPORT_XCRT)
            sint32 flushed = fflush(handle);
            if (ferror(handle) != 0 || flushed != 0)
                successful = 0;
#endif
        }
    }
    iFILE::CommandCompleteCallback(successful);
    return 0;
}

extern "C" sint32 getfilesize(::FILE *handle)
{
    FUNCTION_MARKER(0x800F409C, "NFS4.EXE");
    if (handle == 0)
        return 0;
    if (fseek(handle, 0, SEEK_END) != 0)
        return -1;
    return ftell(handle);
}
