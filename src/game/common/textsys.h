#ifndef TEXTSYS_H
#define TEXTSYS_H

#include "xport.h"
#include "game/common/car.h"

class TextSys
{
public:
    static void LoadWordsGeneric(sint32 language, char *path);
    static void LoadInGame(sint32 language);
    static void LoadWords(sint32 language);
    static char * Word(sint32 wordnum);
    static sint32 WordFlags(sint32 wordnum);
    static sint32 WordX(sint32 wordnum);
    static sint32 WordY(sint32 wordnum);
    static void UnloadWords(void);
};

extern char *wordFile;

#endif
