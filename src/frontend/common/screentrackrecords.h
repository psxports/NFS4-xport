#ifndef SCREENTRACKRECORDS_H
#define SCREENTRACKRECORDS_H

#include "xport.h"
#include "frontend/common/fescreen.h"

struct tRecordBuffer;

class tScreenTrackRecords : public tScreen
{
public:
    tRecordBuffer *TrackRecords;
    sint32 flare_intensity;
    sint32 flareextra;
    bool fReadNewData;
    virtual void GetShapeInfo(sint16 &numPermShapes, sint16 &numSwapShapes, char **permFileName, char **swapFileName);
    virtual void DrawBackground(void);

    virtual void Initialize(void);
    virtual void Cleanup(void);
    void DrawOneRecord(sint32 index, bool newrecord, sint32 y);
    void DrawRecords(sint16 maxitem);
};

extern tScreenTrackRecords *screenTrackRecords;

#endif
