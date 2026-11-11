#include <stdio.h>
#include <string.h>
#include "xport.h"
#include "eaclib/psx/eaclib.h"
#include "game/common/paths.h"
#include "game/common/textsys.h"

struct linedef
{
    sint32 start;
    sint32 end;
    sint32 colour;
    sint32 thickness;
};

typedef linedef LINE;

struct kernpair
{
    uint16 previouscode;
    uint16 code;
    char kernvalue;
    char pad[3];
};

typedef kernpair KERN;

char *wordFile;
static char xport_langFileName_string_0[] = "text.eng";
static char xport_langFileName_string_1[] = "text.ger";
static char xport_langFileName_string_2[] = "text.fre";
static char xport_langFileName_string_3[] = "text.spa";
static char xport_langFileName_string_4[] = "text.ita";
static char xport_langFileName_string_5[] = "text.swe";
char *langFileName[6] = { xport_langFileName_string_0, xport_langFileName_string_1, xport_langFileName_string_2, xport_langFileName_string_3, xport_langFileName_string_4, xport_langFileName_string_5 };

void TextSys::LoadWordsGeneric(sint32 language, char *path)
{
    FUNCTION_MARKER(0x800B9158, "NFS4.EXE");
    if (language >= 7 && wordFile != 0)
        return;

    if ((uint32)language >= sizeof(langFileName) / sizeof(langFileName[0]))
        language = 0;

    if (wordFile != 0)
        purgememadr(wordFile);

    char string[250];
    sprintf(string, "%s%s", path, langFileName[language]);
    wordFile = static_cast<char *>(loadfileadr(string, 0));
}

void TextSys::LoadInGame(sint32 language)
{
    FUNCTION_MARKER(0x800B91DC, "NFS4.EXE");
    char fullpath[80];
    sprintf(fullpath, "%s%s", Paths_Paths[26], "p");
    LoadWordsGeneric(language, fullpath);
}

void TextSys::LoadWords(sint32 language)
{
    FUNCTION_MARKER(0x800B9228, "NFS4.EXE");
    LoadWordsGeneric(language, Paths_Paths[34]);
}

char * TextSys::Word(sint32 wordnum)
{
    FUNCTION_MARKER(0x800B9250, "NFS4.EXE");

    sint32 recordOffset = (sint32)((uint32)wordnum * 12 + 8);
    sint32 textOffset;
    memcpy(&textOffset, wordFile + recordOffset, sizeof(textOffset));
    return wordFile + textOffset;
}

sint32 TextSys::WordFlags(sint32 wordnum)
{
    FUNCTION_MARKER(0x800B9270, "NFS4.EXE");

    sint32 offset = (sint32)((uint32)wordnum * 12 + 3);
    return (uint8)wordFile[offset];
}

sint32 TextSys::WordX(sint32 wordnum)
{
    FUNCTION_MARKER(0x800B9290, "NFS4.EXE");

    sint32 offset = (sint32)((uint32)wordnum * 12 + 4);
    sint16 x;
    memcpy(&x, wordFile + offset, sizeof(x));
    return x;
}

sint32 TextSys::WordY(sint32 wordnum)
{
    FUNCTION_MARKER(0x800B92B0, "NFS4.EXE");

    sint32 offset = (sint32)((uint32)wordnum * 12 + 6);
    sint16 y;
    memcpy(&y, wordFile + offset, sizeof(y));
    return y;
}

void TextSys::UnloadWords(void)
{
    FUNCTION_MARKER(0x800B92D0, "NFS4.EXE");

    if (wordFile != 0)
        purgememadr(wordFile);
    wordFile = 0;
}
