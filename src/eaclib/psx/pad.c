#include "xport.h"
#include "eaclib/psx/eaclib.h"
#include "eaclib/psx/pad.h"
#include "lib/psx/blkfill.h"
#include "lib/psx/blkmov.h"
#include "pad.h"


    static uint16 PAD_convert(PAD_COMMON *pad);

    void padinit(void)
    {
        FUNCTION_MARKER(0x800E40E8, "NFS4.EXE");

        if (gPadinfo.initialized == 0)
        {
            PadInitDirect((uint8 *)&Padglobal[0], (uint8 *)&Padglobal[1]);
            PadStartCom();
            blockclear(&gPadinfo, 84);
            gPadinfo.initialized = 1;
            addtimer(PAD_update);
        }
    }

    void PAD_restore(void)
    {
        FUNCTION_MARKER(0x800E4158, "NFS4.EXE");

        if (gPadinfo.initialized != 0)
        {
            deltimer(PAD_update);
            PadStopCom();
            gPadinfo.initialized = 0;
        }
    }

    uint16 PAD_state(sint32 padID)
    {
        FUNCTION_MARKER(0x800E41AC, "NFS4.EXE");

        if (gPadinfo.initialized == 0 || (uint32)padID >= 8)
            return 0;
        return PAD_convert(&gPadinfo.buf[padID]);
    }

    static uint16 PAD_convert(PAD_COMMON *pad)
    {
        FUNCTION_MARKER(0x800E41FC, "NFS4.EXE");

        return (uint16)~pad->data.standard.state;
    }

    void PAD_update(void)
    {
        FUNCTION_MARKER(0x800E4210, "NFS4.EXE");

        for (sint32 i = 0; i < 2; ++i)
        {
            if (Padglobal[i].nopad != 0)
                blockfill(&gPadinfo.buf[i * 4], 32, 255);
            else
            {
                blockmove(&Padglobal[i], &gPadinfo.buf[i * 4], 8);
                blockfill(&gPadinfo.buf[i * 4 + 1], 24, 255);
            }
        }
        for (sint32 i = 0; i < 8; ++i)
        {
            uint8 active = gPadinfo.buf[i].nopad == 0;
            if (active != gPadinfo.state[i].bActive)
            {
                uint8 time = gPadinfo.state[i].time;
                gPadinfo.state[i].time = (uint8)(time + 1);
                if (time >= 6)
                {
                    gPadinfo.state[i].bActive = active;
                    gPadinfo.state[i].time = 0;
                }
            }
        }
    }


PAD_INFO gPadinfo;
PAD_COMMON Padglobal[2];
