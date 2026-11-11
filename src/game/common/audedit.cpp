#include <stdio.h>
#include "xport.h"
#include "eaclib/psx/eaclib.h"
#include "game/common/audedit.h"
#include "game/common/track.h"

CAudioList *gGameAudioList;

void AudList::PurgeAudio(void)
{
    FUNCTION_MARKER(0x8007B52C, "NFS4.EXE");
    CAudioList *audio = gGameAudioList;
    if (audio)
        purgememadr(audio);
}

void AudList::LoadAudioFile(sint32 AudioFileIndex)
{
    FUNCTION_MARKER(0x8007B554, "NFS4.EXE");
    static char ext[] = "";
    char *path = Track::MakeTrackPathName(ext);
    char fname[128];
    sprintf(fname, "%s%02d.aud", path, AudioFileIndex);
    gGameAudioList = static_cast<CAudioList *>(loadfileadrz(fname, 0));
}
