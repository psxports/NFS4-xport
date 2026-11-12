#include <stdio.h>
#include <string.h>
#include "xport.h"
#include "eaclib/psx/eaclib.h"
#include "game/common/aidatarecord.h"
#include "game/common/bworldsm.h"
#include "game/common/carobj.h"
#include "game/common/paths.h"
#include "lib/psx/fixdmult.h"

static AIDataRecord_t *recordCollection[24];
AIDataRecord::WhichRecord_t AIDataRecord::WhichRecord = AIDataRecord::NORECORD_R;
AIDataRecord::RecordMethod_t AIDataRecord::RecordMethod = AIDataRecord::NORMAL_M;
AIDataRecord_BestLine_t *AIDataRecord::BestLine;
AIDataRecord_TrackCurve_t *AIDataRecord::TrackCurve;

void *AIDataRecord_t::operator new(size_t size)
{
    return reservememadr(0, (sint32)size, 0);
}

void AIDataRecord_t::operator delete(void *memory)
{
    purgememadr(memory);
}

AIDataRecord_t::AIDataRecord_t(AIDataRecord::WhichRecord_t whichIsThis, char *preBuffer)
{
    FUNCTION_MARKER(0x8006CE24, "NFS4.EXE");
    AddRecordToCollection();
    preAllocatedBuffer_ = preBuffer;
    recordMethod_ = AIDataRecord::NORMAL_M;
}

AIDataRecord_t::~AIDataRecord_t(void)
{
    FUNCTION_MARKER(0x8006CE6C, "NFS4.EXE");
    SaveAndPurge();
    RemoveRecordFromCollection();
}

sint32 AIDataRecord_t::AddRecordToCollection(void)
{
    FUNCTION_MARKER(0x8006CEC4, "NFS4.EXE");
    for (sint32 recordLoop = 0; recordLoop < 24; ++recordLoop)
        if (recordCollection[recordLoop] == 0)
        {
            recordCollection[recordLoop] = this;
            return 1;
        }
    return 0;
}

sint32 AIDataRecord_t::RemoveRecordFromCollection(void)
{
    FUNCTION_MARKER(0x8006CF00, "NFS4.EXE");
    for (sint32 recordLoop = 0; recordLoop < 24; ++recordLoop)
        if (recordCollection[recordLoop] == this)
        {
            recordCollection[recordLoop] = 0;
            return 1;
        }
    return 0;
}

void AIDataRecord_t::StartUp1(void)
{
    FUNCTION_MARKER(0x8006CF3C, "NFS4.EXE");
    AIDataRecord::WhichRecord = AIDataRecord::NORECORD_R;
    AIDataRecord::RecordMethod = AIDataRecord::NORMAL_M;
    for (sint32 recordLoop = 23; recordLoop >= 0; --recordLoop)
        recordCollection[recordLoop] = 0;
    AIDataRecord::BestLine = new AIDataRecord_BestLine_t(AIDataRecord::BEST_LINE_R);
    AIDataRecord::TrackCurve = new AIDataRecord_TrackCurve_t(AIDataRecord::TRACK_CURVE_R);
}

void AIDataRecord_t::StartUp2(void)
{
    FUNCTION_MARKER(0x8006CFA8, "NFS4.EXE");
    for (sint32 recordLoop = 0; recordLoop < 24; ++recordLoop)
    {
        AIDataRecord_t *record = recordCollection[recordLoop];
        if (record != 0)
            record->Setup();
    }
}

void AIDataRecord_t::CleanUp1(void)
{
    FUNCTION_MARKER(0x8006D014, "NFS4.EXE");
    if (AIDataRecord::BestLine != 0)
    {
        delete AIDataRecord::BestLine;
        AIDataRecord::BestLine = 0;
    }
    if (AIDataRecord::TrackCurve != 0)
    {
        delete AIDataRecord::TrackCurve;
        AIDataRecord::TrackCurve = 0;
    }
}

void AIDataRecord_t::CleanUp2(void)
{
    FUNCTION_MARKER(0x8006D084, "NFS4.EXE");
}

void AIDataRecord_t::Setup(void)
{
    FUNCTION_MARKER(0x8006D08C, "NFS4.EXE");
    char *buffer = preAllocatedBuffer_;
    if (buffer == 0)
    {
        if (Load() != 0)
            return;
        buffer = (char *)reservememadr(name_, bSize_, 0);
    }
    dataBuffer_ = buffer;
}

sint32 AIDataRecord_t::Load(void)
{
    FUNCTION_MARKER(0x8006D0DC, "NFS4.EXE");
    if (recordMethod_ == AIDataRecord::NORMAL_M)
        dataBuffer_ = (char *)loadpackadrz(name_, 0);
    return dataBuffer_ != 0;
}

sint32 AIDataRecord_t::SaveAndPurge(void)
{
    FUNCTION_MARKER(0x8006D120, "NFS4.EXE");
    char *buffer = dataBuffer_;
    if (buffer == 0)
        return 0;
    if (preAllocatedBuffer_ == 0)
        purgememadr(buffer);
    dataBuffer_ = 0;
    return 1;
}

AIDataRecord_AccTable_t::AIDataRecord_AccTable_t(char *preBuffer, sint32 scale, AIDataRecord::WhichRecord_t whichIsThis) : AIDataRecord_t(whichIsThis, preBuffer)
{
    FUNCTION_MARKER(0x8006D170, "NFS4.EXE");
    numElements_ = 112;
    bSize_ = 224;
    scale_ = scale;
    sprintf(name_, "%s%s.acc", Paths_Paths[5], Cars::gHumanRaceCarList[0]->carName);
}

sint32 AIDataRecord_AccTable_t::Get(sint32 speed)
{
    FUNCTION_MARKER(0x8006D1F4, "NFS4.EXE");
    sint16 value;
    memcpy(&value, dataBuffer_ + speed * 2, sizeof(value));
    return value * 256;
}

void AIDataRecord_AccTable_t::Setup(void)
{
    FUNCTION_MARKER(0x8006D20C, "NFS4.EXE");
    AIDataRecord_t::Setup();
    for (sint32 loop = 0; loop < 112; ++loop)
    {
        sint32 value = Get(loop);
        value = fixedmult(value, scale_);
        sint16 stored = (sint16)(value >> 8);
        memcpy(dataBuffer_ + loop * 2, &stored, sizeof(stored));
    }
}

AIDataRecord_AccTable_t::~AIDataRecord_AccTable_t(void)
{
    FUNCTION_MARKER(0x8006D594, "NFS4.EXE");
}

AIDataRecord_BestLine_t::AIDataRecord_BestLine_t(AIDataRecord::WhichRecord_t whichIsThis) : AIDataRecord_t(whichIsThis, 0)
{
    FUNCTION_MARKER(0x8006D278, "NFS4.EXE");
    numElements_ = gNumSlices;
    bSize_ = numElements_;
    if (recordMethod_ == AIDataRecord::NORMAL_M)
        sprintf(name_, "%sTr%02d.qbe", Paths_Paths[12], GameSetup::gData.track);
    else
        sprintf(name_, "%sTr%02d.bes", Paths_Paths[13], GameSetup::gData.track);
}

AIDataRecord_BestLine_t::~AIDataRecord_BestLine_t(void)
{
    FUNCTION_MARKER(0x8006D574, "NFS4.EXE");
}

AIDataRecord_TrackCurve_t::AIDataRecord_TrackCurve_t(AIDataRecord::WhichRecord_t whichIsThis) : AIDataRecord_t(whichIsThis, 0)
{
    FUNCTION_MARKER(0x8006D30C, "NFS4.EXE");
    numElements_ = gNumSlices;
    bSize_ = (numElements_ + 1);
    if (recordMethod_ == AIDataRecord::NORMAL_M)
        sprintf(name_, "%sTr%02d.qcr", Paths_Paths[10], GameSetup::gData.track);
    else
        sprintf(name_, "%sTr%02d.crv", Paths_Paths[11], GameSetup::gData.track);
}

sint32 AIDataRecord_TrackCurve_t::Get(sint32 slice)
{
    FUNCTION_MARKER(0x8006D3A4, "NFS4.EXE");
    return (uint8)dataBuffer_[slice];
}

AIDataRecord_TrackCurve_t::~AIDataRecord_TrackCurve_t(void)
{
    FUNCTION_MARKER(0x8006D554, "NFS4.EXE");
}

AIDataRecord_CurveSpeedTable_t::AIDataRecord_CurveSpeedTable_t(char *carName, AIDataRecord::WhichRecord_t whichIsThis) : AIDataRecord_t(whichIsThis, 0)
{
    FUNCTION_MARKER(0x8006D3BC, "NFS4.EXE");
    numElements_ = 256;
    bSize_ = 256;
    if (recordMethod_ == AIDataRecord::NORMAL_M)
        sprintf(name_, "%s%s.qcs", Paths_Paths[4], carName);
    else
        sprintf(name_, "%s%s.csp", Paths_Paths[5], carName);
}

sint32 AIDataRecord_CurveSpeedTable_t::Get(sint32 curve)
{
    FUNCTION_MARKER(0x8006D44C, "NFS4.EXE");
    if (curve < 0)
        curve = (-curve);
    if (curve >= 256)
        curve = 255;
    return (uint8)dataBuffer_[curve] * 65536;
}

void AIDataRecord_CurveSpeedTable_t::Upgrade(sint32 handlingUpgrade)
{
    FUNCTION_MARKER(0x8006D480, "NFS4.EXE");
    for (sint32 curveLoop = 0; curveLoop < numElements_; ++curveLoop)
    {
        sint32 value = Get(curveLoop);
        value = fixedmult(value, handlingUpgrade);
        dataBuffer_[curveLoop] = (char)(value / 65536);
    }
}

AIDataRecord_CurveSpeedTable_t::~AIDataRecord_CurveSpeedTable_t(void)
{
    FUNCTION_MARKER(0x8006D534, "NFS4.EXE");
}

sint32 AIDataRecord_CarTracking_t::Get(sint32 slice)
{
    FUNCTION_MARKER(0x8006D50C, "NFS4.EXE");
    return 0;
}

AIDataRecord_CarTracking_t::~AIDataRecord_CarTracking_t(void)
{
    FUNCTION_MARKER(0x8006D514, "NFS4.EXE");
}
