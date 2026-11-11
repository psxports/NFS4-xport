#ifndef FETRACKS_H
#define FETRACKS_H

#include "xport.h"
#include "frontend/common/trackclass.h"

struct tTrackInformation;

struct tSaveTrackInfo;

class tTrackManager
{
public:
    uint32 fNumTracks;
    tTrackInformation *fTracks;
    sint32 fAvailableTracks[16];
    sint32 fViewableTracks[16];
    void Initialize(void);
    void LoadTracks(tSaveTrackInfo &load);
    void SaveTracks(tSaveTrackInfo &save);
    void GetTrack(sint16 trackNumber, tTrackInformation &trackInfo);
    void LoadDescription(void);
    void ReleaseDescription(void);
    void SetTrackAvailable(sint16 track, bool avail);
    void SetClassAvailable(tTrackClassType trackClass, bool avail);
    tTrackInformation *GetTrackByID(sint16 track);
};

extern tTrackManager trackManager;
extern sint16 CountryMeasurement[8];

#endif
