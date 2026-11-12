#include "xport.h"
#include "game/common/gmesetup.h"
#include "game/common/simqueue.h"

enum VALIDITY
{
    kINVALID = 0,
    kVALID = 1,
    kPREDICTED = 2,
    kRESIM = 3,
};

typedef struct sim_queue SIM_QUEUE;

struct sim_queue
{
    Input::tResults Buffer[2][32];
    VALIDITY Validity[2][32];
    sint32 HeadTime;
    sint32 TailTime[2];
};

sint32 gSimQueue_Ticker = -1;
sint32 gSimQueue_BlockSelf = 1;
sint32 gSimQueue_BlockOther = 1;
static sint32 maxTicksPerFrame;
static sim_queue inputQueue;
static Input::tResults output[2];

void SimQueue::StartUp(void)
{
    FUNCTION_MARKER(0x800B72EC, "NFS4.EXE");

    gSimQueue_BlockSelf = 1;
    gSimQueue_BlockOther = 1;
    SetLag();
}

void SimQueue::Reset(void)
{
    FUNCTION_MARKER(0x800B7318, "NFS4.EXE");

    gSimQueue_BlockSelf = 1;
    gSimQueue_BlockOther = 1;
    for (sint32 i = 0; i < 2; ++i)
    {
        for (sint32 j = 0; j < 32; ++j)
        {
            inputQueue.Validity[i][j] = j < 4 ? kVALID : kINVALID;
            inputQueue.Buffer[i][j].steering = 0;
            inputQueue.Buffer[i][j].gas = 0;
            inputQueue.Buffer[i][j].brake = 0;
            inputQueue.Buffer[i][j].flags = 0;
        }
        inputQueue.TailTime[i] = 4;
    }
    inputQueue.HeadTime = 0;
    gSimQueue_Ticker = 3;
    gSimQueue_BlockOther = 0;
    gSimQueue_BlockSelf = 0;
}

void SimQueue::CleanUp(void)
{
    FUNCTION_MARKER(0x800B73B8, "NFS4.EXE");

    gSimQueue_BlockSelf = 1;
    gSimQueue_BlockOther = 1;
}

sint32 SimQueue::Put(sint32 pIndex, Input::tResults *val)
{
    FUNCTION_MARKER(0x800B73CC, "NFS4.EXE");

    sint32 tail = inputQueue.TailTime[pIndex] & 31;
    if (IsBlocking(pIndex) != 0 || inputQueue.Validity[pIndex][tail] != kINVALID)
        return 0;
    Input::tResults value = *val;
    inputQueue.Buffer[pIndex][tail] = value;
    inputQueue.Validity[pIndex][tail] = kVALID;
    inputQueue.TailTime[pIndex] = (inputQueue.TailTime[pIndex] + 1);
    if (GameSetup::gData.commMode == 0)
    {
        if (GameSetup::gData.numPlayerRaceCars >= 2 && pIndex == 0)
            return 1;
    }
    else if (GameSetup::gData.commMode != 1 || pIndex == 0)
        return 1;
    gSimQueue_Ticker = (gSimQueue_Ticker + 1);
    return 1;
}

void SimQueue::SetCurrentInput(sint32 time)
{
    FUNCTION_MARKER(0x800B74E4, "NFS4.EXE");

    sint32 i = time & 31;
    for (sint32 pIndex = 0; pIndex < GameSetup::gData.numPlayerRaceCars; ++pIndex)
    {
        Input::tResults value = inputQueue.Buffer[pIndex][i];
        output[pIndex] = value;
    }
    for (sint32 pIndex = 0; pIndex < GameSetup::gData.numPlayerRaceCars; ++pIndex)
        inputQueue.Validity[pIndex][i] = kINVALID;
    inputQueue.HeadTime = (inputQueue.HeadTime + 1);
}

void SimQueue::GetCurrentInput(sint32 pIndex, Input::tResults *out)
{
    FUNCTION_MARKER(0x800B75A4, "NFS4.EXE");

    Input::tResults value = output[pIndex];
    *out = value;
}

void SimQueue::SetLag(void)
{
    FUNCTION_MARKER(0x800B75CC, "NFS4.EXE");

    if (GameSetup::gData.commMode == 0 || GameSetup::gData.commMode == 1)
        maxTicksPerFrame = 4;
}

sint32 SimQueue::IsBlocking(sint32 pIndex)
{
    FUNCTION_MARKER(0x800B75F8, "NFS4.EXE");

    if (pIndex != 0)
        return 0;
    if (GameSetup::gData.commMode != 0 && GameSetup::gData.commMode != 1)
        return 0;
    sint32 limit = (inputQueue.HeadTime + maxTicksPerFrame);
    return inputQueue.TailTime[0] >= limit;
}
