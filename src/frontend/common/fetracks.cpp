#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include "xport.h"
#include "eaclib/psx/eaclib.h"
#include "frontend/common/femenu.h"
#include "frontend/common/fetracks.h"
#include "game/common/nfs3.h"
#include "game/common/paths.h"
#include "lib/psx/blkmov.h"

class tracks
{
public:
    static sint32 maxTrackIndex;
};

struct tTrackInformation;

struct tTrackInformation;

struct tTrackInformation
{
    char fTrackID;
    uint8 fSimNumber;
    uint8 fTrackDifficulty;
    uint8 fAvailable;
    uint8 fIsEgg;
    uint8 fLengthKM;
    uint8 fLengthMiles;
    uint8 fNumMoments;
    char fShapeName[8];
    char fSplineName[8];
    char fCountry;
    char fDispatch;
    char fReverseCall;
    char fLanguage;
    char fTrafficCars[6];
    sint16 fTX;
    sint16 fTY;
    sint16 fSX;
    sint16 fSY;
    uint8 fSpeedoCountry;
    uint8 fPad;
    sint32 fRotate;
};

struct tSaveTrackInfo
{
    uint8 fTrackActivated[16];
};

sint16 CountryMeasurement[8] = { 0, 1, 0, 1, 1, 0, 1, 1 };
sint32 tracks::maxTrackIndex;

void tTrackManager::Initialize(void)
{
    FUNCTION_MARKER(0x80021A10, "FRONT.BIN");
    fNumTracks = 0;
    fTracks = 0;
    for (sint16 i = 0; i < 16; ++i)
    {
        fAvailableTracks[i] = 0;
        fViewableTracks[i] = 0;
    }
}

void tTrackManager::LoadTracks(tSaveTrackInfo &load)
{
    FUNCTION_MARKER(0x80021A50, "FRONT.BIN");
    for (uint32 i = 0; i < 16; ++i)
        fAvailableTracks[i] = load.fTrackActivated[i] != 0;
}

void tTrackManager::SaveTracks(tSaveTrackInfo &save)
{
    FUNCTION_MARKER(0x80021A7C, "FRONT.BIN");
    for (uint32 i = 0; i < 16; ++i)
        save.fTrackActivated[i] = (uint8)fAvailableTracks[i];
}

void tTrackManager::GetTrack(sint16 trackNumber, tTrackInformation &trackInfo)
{
    FUNCTION_MARKER(0x80021AA4, "FRONT.BIN");
    blockmove(&fTracks[trackNumber], &trackInfo, 48);
    trackInfo.fAvailable = (uint8)fAvailableTracks[(sint8)trackInfo.fTrackID];
}

void tTrackManager::LoadDescription(void)
{
    FUNCTION_MARKER(0x80021B14, "FRONT.BIN");
    char filename[80];
    sprintf(filename, "%s%s", Paths_Paths[37], "fetrk.trk");
    ReleaseDescription();
    char *input = static_cast<char *>(loadfileadr(filename, 16));
    memcpy(&fNumTracks, input, sizeof(fNumTracks));
    fTracks = static_cast<tTrackInformation *>(reservememadr("Track List", fNumTracks * 48, 0));
    char *data = input + 4;
    blockmove(data, fTracks, fNumTracks * 48);
    for (uint32 i = 0; i < fNumTracks; ++i)
    {
        if (fTracks[i].fAvailable)
            fAvailableTracks[(sint8)fTracks[i].fTrackID] = 1;
        if (!fTracks[i].fIsEgg)
            fViewableTracks[(sint8)fTracks[i].fTrackID] = 1;
    }
    purgememadr(input);
}

void tTrackManager::ReleaseDescription(void)
{
    FUNCTION_MARKER(0x80021C4C, "FRONT.BIN");
    if (fTracks != 0)
        purgememadr(fTracks);
    fTracks = 0;
    fNumTracks = 0;
}

void tTrackManager::SetTrackAvailable(sint16 track, bool avail)
{
    FUNCTION_MARKER(0x80021C8C, "FRONT.BIN");
    fAvailableTracks[track] = avail;
}

void tTrackManager::SetClassAvailable(tTrackClassType trackClass, bool avail)
{
    FUNCTION_MARKER(0x80021CA0, "FRONT.BIN");
    for (uint32 i = 0; i < fNumTracks; ++i)
    {
        if (fTracks[i].fTrackDifficulty == trackClass)
            fAvailableTracks[(sint8)fTracks[i].fTrackID] = avail;
    }
}

tTrackInformation *tTrackManager::GetTrackByID(sint16 track)
{
    FUNCTION_MARKER(0x80021D00, "FRONT.BIN");
    for (uint32 i = 0; i < fNumTracks; ++i)
    {
        if ((sint8)fTracks[i].fTrackID == track)
            return &fTracks[i];
    }
    return fTracks;
}

tListIteratorTrack::tListIteratorTrack(char *valPtr, char *index, tTrackManager *trackManager) : tListIteratorIndexed(0, valPtr, index)
{
    FUNCTION_MARKER(0x80021D54, "FRONT.BIN");
    fTrackManager = trackManager;
}

tListIteratorTrack::~tListIteratorTrack(void)
{
    FUNCTION_MARKER(0x80021DAC, "FRONT.BIN");
}

sint16 tListIteratorTrack::TextValue(tPlayer atIndex)
{
    FUNCTION_MARKER(0x80021DD4, "FRONT.BIN");
    tTrackInformation *trackInfo = &fTrackManager->fTracks[fValue[*fIndex]];
    return (sint8)trackInfo->fTrackID + 213;
}

void tListIteratorTrack::Increment(tPlayer atIndex)
{
    FUNCTION_MARKER(0x80021E14, "FRONT.BIN");
    do
    {
        char *value = fValue + *fIndex;
        *value = (char)(*value + 1);
        value = fValue + *fIndex;
        uint8 current = *value;
        if (current >= fTrackManager->fNumTracks)
            *value = 0;
    } while (ValidTrack(fValue[*fIndex]) != 1);
}

void tListIteratorTrack::Decrement(tPlayer atIndex)
{
    FUNCTION_MARKER(0x80021EC0, "FRONT.BIN");
    do
    {
        char *value = fValue + *fIndex;
        uint8 current = *value;
        if (current == 0)
            *value = (char)(fTrackManager->fNumTracks - 1);
        else
            *value = (char)(current - 1);
    } while (ValidTrack(fValue[*fIndex]) != 1);
}

sint32 tListIteratorTrack::ValidTrack(char track)
{
    FUNCTION_MARKER(0x80021F50, "FRONT.BIN");
    tTrackInformation *trackInfo = &fTrackManager->fTracks[track];
    sint32 trackID = (sint8)trackInfo->fTrackID;
    sint32 result = fTrackManager->fAvailableTracks[trackID];
    if (frontEnd.raceType == 0)
        result = (result | fTrackManager->fViewableTracks[trackID]) != 0;
    else if (frontEnd.raceType == 1)
    {
        result = (result | fTrackManager->fViewableTracks[trackID]) != 0;
        if (trackInfo->fIsEgg)
            result = 0;
        if (trackInfo->fTrackDifficulty >= 3)
            result = 0;
    }
    return result;
}
