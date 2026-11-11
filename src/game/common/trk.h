#ifndef TRK_H
#define TRK_H

#include "xport.h"

class Trk
{
public:
    struct AnimateBoomInst
    {
        sint16 size;
        uint8 type;
        uint8 objectIndex;
        uint8 zoffset;
        uint8 flags;
        sint16 pad;
        sint16 count;
        sint16 interval;
        uint8 simIndex;
        uint8 boomIndex;
        sint16 pad2;
    };

    struct AnimateInst
    {
        sint16 size;
        uint8 type;
        uint8 objectIndex;
        uint8 zoffset;
        uint8 flags;
        sint16 pad;
        sint16 count;
        sint16 interval;
    };

    struct CollideBoomInst
    {
        sint16 size;
        uint8 type;
        uint8 objectIndex;
        uint8 zoffset;
        uint8 flags;
        sint16 pad;
        sint32 x;
        sint32 y;
        sint32 z;
        sint16 qx;
        sint16 qy;
        sint16 qz;
        sint16 qw;
        sint16 sx;
        sint16 sy;
        sint16 sz;
        uint8 simIndex;
        uint8 boomIndex;
    };

    struct SFX
    {
        sint32 point[3];
        sint16 type;
        sint16 pad;
    };

    struct SimpleInst
    {
        sint16 size;
        uint8 type;
        uint8 objectIndex;
        uint8 zoffset;
        uint8 flags;
        sint16 pad;
        sint32 x;
        sint32 y;
        sint32 z;
    };

    struct SimObject
    {
        sint32 point[3];
        sint16 radius;
        sint16 serialNum;
        uint8 topCRAP;
        uint8 bottomCRAP;
        uint8 instIndex;
        uint8 type;
    };

    struct Material
    {
        sint16 shapeIndex;
        uint8 flag;
        uint8 uvFlag;
        uint8 r;
        uint8 g;
        uint8 b;
        char textureCount;
        uint8 interval;
        char pad;
    };

    struct Line

    {
        uint8 firstPoint;

        uint8 slice;

        uint8 type;

        uint8 quadIndex;
    };

    struct ObjectDef

    {
        sint16 id;

        uint8 vertexCount;

        uint8 quadCount;
    };

    struct Quad

    {
        sint16 material;

        uint8 aPoints[4];
    };

    struct NewStrip
    {
        uint8 topVert;
        uint8 botVert;
        char quadCount;
        char size;
    };

    struct NewSimSlice
    {
        uint8 stripIndex;
        uint8 quadCount;
        uint8 simquadIndex;
        uint8 simquadCount;
        uint8 simquadStartIndex;
    };

    struct NewSlice
    {
        sint32 center[3];
        char normal[3];
        char forward[3];
        char right[3];
        uint8 acousticType;
        sint16 pavedProfile;
        sint16 leftDrive;
        sint16 rightDrive;
        uint8 chunkIndex;
        uint8 laneCount;
        uint8 avgPavedWidthLf;
        uint8 avgPavedWidthRt;
    };

    struct NewSimQuad
    {
        uint8 surface;
    };
};

#endif
