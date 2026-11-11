#ifndef FETOOLS_H
#define FETOOLS_H

#include "xport.h"

class FeTools
{
public:
    static sint32 gScrollTicksOut;

    static void DrawPSXButton(uint8 padType, uint16 button, sint32 x, sint32 y);
    static void init(void);
    static void deinit(void);
    static void FormatMoney(char *string, sint32 amount);
};

void s_upper(char *string);
void s_lower(char *string);

extern sint16 currentSize;
extern char *font12;
extern char *font18;
extern char *fontTitle;

#endif
