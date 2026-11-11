#ifndef CHUNK_H
#define CHUNK_H

#include "xport.h"
#include "psx.h"
#include "game/common/coorddef.h"
#include "game/common/group.h"
#include "game/common/renderview.h"
#include "game/common/simplemem.h"
#include "game/common/trk.h"

struct RelCoord16
{
    sint16 x;
    sint16 z;
};

class Chunk
{
public:
    static coorddef *chunkCenters;
    static CVECTOR *lightTable;
    static sint32 numLight;

    void InstanceGroup(SerializedGroup *chunkGroup, SimpleMem *mem);
    RelCoord16 boundPts[4];
    RelCoord16 chunkboundPts[4];
    uint8 quadCounts[6];
    uint8 pad[2];
    Trk::Quad *renderQuads[4];
    Group *stripBuf;
    Group *lorezstripBuf;
    Group *objInstanceBuf;
    Group *objSpecialInstanceBuf;
    Group *simSliceBuf;
    Group *simQuadBuf;
    Group *simObjBuf;
    Group *sfxBuf;
    Group *lineBuf;
    Group *objVertexBuf;
    Group *objQuadBuf;
    Group *objQuadInstanceBuf;
    sint16 firstSimSliceInd;
    sint16 chunkInd;
    Group *vertexBuf;
};

#endif
