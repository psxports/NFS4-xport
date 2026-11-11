#include <stddef.h>
#include <string.h>
#include "xport.h"
#include "eaclib/psx/eaclib.h"
#include "eaclib/psx/ssysinit.h"
#include "game/common/audioclc.h"
#include "game/common/audiocmn.h"
#include "game/common/audiomus.h"
#include "game/psx/audio.h"

sint32 gMusicHandle;
sint32 backwardsTrackStartTrigger[5] = { 4, 4, 7, 6, 4 };
sint32 Audio::direct3davail;
void *Audio::gHeap;

void Audio::InitDriver(sint32 buffersize, sint32 spusize)
{
    FUNCTION_MARKER(0x800BBB84, "NFS4.EXE");
    for (sint32 i = 0; i < 7; ++i) {
        gSndBnk[i].bnkID = i;
        gSndBnk[i].pdata = 0;
    }
    if (AudioCmn::kAudioOn != 0 || AudioCmn::kAudioStreamingOn != 0) {
        SNDSYSOPTS opts;
        SNDSYS::getopts(&opts);
        opts.set.maxbanks = 48;
        SNDSYS::setopts(&opts);
        SNDSYS::vectortoreal();
        gHeap = reservememadr("audio heap", 4096, 0);
        SNDSYS::init(gHeap, 4096);
        AudioCmn::InitReverb();
    }
    if (buffersize > 0) {
        static char songs[] = "amus";
        AudioMus::SysStartUp(buffersize, spusize, songs);
    }
    direct3davail = 0;
}

void Audio::DeInitDriver(void)
{
    FUNCTION_MARKER(0x800BBC64, "NFS4.EXE");
    AudioMus::SysCleanUp();
    if (AudioCmn::kAudioOn != 0 || AudioCmn::kAudioStreamingOn != 0) {
        SNDSYS::restore();
        purgememadr(gHeap);
    }
}

void Audio::CleanUp(void)
{
    FUNCTION_MARKER(0x800BBCC0, "NFS4.EXE");
    for (sint32 i = 0; i < 7; ++i) {
        if (gSndBnk[i].pdata != 0) {
            purgememadr(gSndBnk[i].pdata);
            gSndBnk[i].pdata = 0;
        }
    }
}

void Audio::FECleanUp(void)
{
    FUNCTION_MARKER(0x800BBD1C, "NFS4.EXE");
    cSNDbankremove(-1, 0);
    if (gSndBnk[0].pdata != 0) {
        purgememadr(gSndBnk[0].pdata);
        gSndBnk[0].pdata = 0;
    }
}

sint32 AudioCmn::AddBank(char *filename, sint32 size, char *pdata, sint32 BankNum)
{
    FUNCTION_MARKER(0x800BBD64, "NFS4.EXE");
    sint32 bhandle = -1;
    char *ptemp = 0;
    char *p = filename;
    for (char *current = filename; *current != 0; ++current) {
        if (*current == '\\')
            p = current + 1;
    }
    sint32 check = SNDbankadd(&bhandle, pdata);
    if (check == 7) {
        sint32 headerSize = SNDbankheadersize(bhandle);
        ptemp = static_cast<char *>(reservememadr(p, headerSize, 0));
        SNDbankheadercopy(ptemp, bhandle);
        headerSize = SNDbankheadersize(bhandle);
        size = (sint32)((uint32)size - (uint32)headerSize);
    } else {
        AudioClc::SndError(check);
    }
    purgememadr(pdata);
    gSndBnk[BankNum].pdata = ptemp;
    gSndBnk[BankNum].bnkID = bhandle;
    return size;
}

sint32 AudioCmn::LoadBank(char *filename, sint32 BankNum)
{
    FUNCTION_MARKER(0x800BBE8C, "NFS4.EXE");
    char bankdata[80];
    strcpy(bankdata, filename);
    strcat(bankdata, ".bnk");
    char *pdata = static_cast<char *>(loadfileadrz(bankdata, 16));
    if (pdata == 0)
        return 0;
    sint32 size = filesize(bankdata);
    return AddBank(filename, size, pdata, BankNum);
}
