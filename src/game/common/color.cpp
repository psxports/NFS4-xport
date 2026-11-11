#include <stdlib.h>
#include "xport.h"
#include "game/common/color.h"

char IsNumChar(char c)
{
    FUNCTION_MARKER(0x80091EC0, "NFS4.EXE");
    if ((uint8)((uint8)c - '0') < 10)
        return 1;
    return c == '-' || c == '+';
}

sint32 Risk_ReadNextValue(char **aScript)
{
    FUNCTION_MARKER(0x80091EFC, "NFS4.EXE");
    char s[256];
    sint32 n = 0;
    char *Script = *aScript;
    while (IsNumChar(*Script) == 0)
    {
        char c = *Script++;
        if (c == '/')
        {
            while (*Script++ != '/')
            {
            }
        }
    }
    while (IsNumChar(*Script) != 0)
        s[n++] = *Script++;
    s[n] = '\0';
    sint32 value = atoi(s);
    *aScript = Script;
    return value;
}
