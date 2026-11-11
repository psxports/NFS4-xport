#include "xport.h"
#include "eaclib/psx/internal.h"

const char *strrstr(const char *text, const char *separators)
{
    FUNCTION_MARKER(0x800E8940, "NFS4.EXE");
    const char *last = 0;
    while (*separators != 0)
    {
        const char *found = strrchr(text, (uint8)*separators);
        if (((uintptr_t)(last)) < ((uintptr_t)(found)))
            last = found;
        ++separators;
    }
    return last;
}

sint32 wildcard(const char *name, char *pattern)
{
    FUNCTION_MARKER(0x800E89BC, "NFS4.EXE");
    char *split = ((char *)(strrstr(pattern, "&|!~^")));
    if (split != 0)
    {
        uint8 operation = (uint8)*split;
        if (operation == '&')
        {
            *split = 0;
            sint32 result = 0;
            if (wildcard(name, pattern) != 0)
                result = wildcard(name, split + 1) != 0;
            *split = '&';
            return result;
        }
        if (operation == '|' || operation == '!' || operation == '~')
        {
            *split = 0;
            sint32 result = wildcard(name, pattern) != 0 || wildcard(name, split + 1) != 0;
            *split = operation == '|' ? '|' : '!';
            return result;
        }
        if (operation == '^')
        {
            *split = 0;
            sint32 left = wildcard(name, pattern);
            sint32 right = wildcard(name, split + 1);
            *split = '^';
            return left ^ right;
        }
    }
    for (;;)
    {
        uint8 operation = (uint8)*pattern;
        if (operation == '*')
        {
            ++pattern;
            for (;;)
            {
                if (wildcard(name, pattern) != 0)
                    return 1;
                if (*name++ == 0)
                    return 0;
            }
        }
        if (operation == '?' || operation == '#')
        {
            uint8 character = (uint8)*name;
            sint32 matched = character;
            if (operation == '#')
            {
                XASSERT(character < 128);
                matched = character >= '0' && character <= '9';
            }
            ++pattern;
            ++name;
            if (matched == 0)
                return 0;
            continue;
        }
        if (operation == '~')
            return wildcard(name, pattern + 1) == 0;
        uint8 expected = (uint8)tolower((uint8)*pattern);
        uint8 actual = (uint8)tolower((uint8)*name);
        if (expected != actual)
            return 0;
        if (*pattern++ == 0)
            return 1;
        ++name;
    }
}
