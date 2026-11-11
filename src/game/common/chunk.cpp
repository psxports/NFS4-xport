#include <stddef.h>
#include <string.h>
#include "xport.h"
#include "psx.h"
#include "game/common/chunk.h"
#include "game/common/coorddef.h"
#include "game/common/gmesetup.h"
#include "game/common/group.h"
#include "game/common/renderview.h"
#include "game/common/simplemem.h"
#include "game/common/trk.h"

coorddef *Chunk::chunkCenters;
CVECTOR *Chunk::lightTable;

void Chunk::InstanceGroup(SerializedGroup *chunkGroup, SimpleMem *mem)
{
    FUNCTION_MARKER(0x8008B3FC, "NFS4.EXE");
    SerializedGroup *metadata = chunkGroup->LocateGroupType(0x1c, 0);
    uint8 *chunkInfo = reinterpret_cast<uint8 *>(metadata + 1);
    memcpy(&firstSimSliceInd, chunkInfo + 10, 2);
    for (sint32 i = 0; i < 4; ++i)
        memcpy(&boundPts[i], chunkInfo + 16 + i * 4, 4);
    for (sint32 i = 0; i < 4; ++i)
        memcpy(&chunkboundPts[i], chunkInfo + 32 + i * 4, 4);
    memcpy(&chunkInd, chunkInfo + 12, 2);
    if (GameSetup::gData.commMode != 1)
    {
        objInstanceBuf = chunkGroup->LocateCreateGroupType(3, mem, 0);
        simObjBuf = chunkGroup->LocateCreateGroupType(11, mem, 0);
    }
    else
    {
        SerializedGroup *simGroup = chunkGroup->LocateGroupType(11, 0);
        SerializedGroup *instGroup = chunkGroup->LocateGroupType(3, 0);
        if (simGroup != 0)
        {
            sint32 i = 0;
            sint32 numElements = simGroup->m_num_elements;
            Trk::SimObject *simObjs = reinterpret_cast<Trk::SimObject *>(simGroup + 1);
            for (; i < numElements; ++i)
            {
                sint32 index = simObjs[i].instIndex;
                if (index != 127)
                {
                    if (index >= instGroup->m_num_elements)
                        break;
                    Trk::SimpleInst *inst = reinterpret_cast<Trk::SimpleInst *>(instGroup + 1);
                    for (sint32 j = 0; j < index; ++j)
                        inst = reinterpret_cast<Trk::SimpleInst *>(reinterpret_cast<uint8 *>(inst) + inst->size);
                    if (inst == 0 || inst->type != 5 ||
                        reinterpret_cast<Trk::CollideBoomInst *>(inst)->boomIndex != 0)
                        break;
                }
            }
            if (i != 0)
            {
                simObjBuf = chunkGroup->CreateLiteGroupDataSize(simGroup, mem, (sint32)((uint32)i * 20U));
                simObjBuf->m_num_elements = i;
            }
            else
                simObjBuf = 0;
        }
        else
            simObjBuf = 0;
        if (instGroup != 0)
        {
            sint32 numElements = instGroup->m_num_elements;
            sint32 i = 0;
            sint32 size = 0;
            Trk::SimpleInst *inst = reinterpret_cast<Trk::SimpleInst *>(instGroup + 1);
            for (; i < numElements; ++i)
            {
                if (inst->type == 5)
                {
                    if (reinterpret_cast<Trk::CollideBoomInst *>(inst)->boomIndex != 0)
                        break;
                }
                else if (inst->type != 1)
                    break;
                sint32 length = inst->size;
                size = (sint32)((uint32)size + (uint32)length);
                inst = reinterpret_cast<Trk::SimpleInst *>(reinterpret_cast<uint8 *>(inst) + length);
            }
            objInstanceBuf = chunkGroup->CreateLiteGroupDataSize(instGroup, mem, size);
            objInstanceBuf->m_num_elements = i;
        }
        else
            objInstanceBuf = 0;
        if (simObjBuf != 0 && objInstanceBuf != 0)
        {
            Trk::SimObject *simObjs = reinterpret_cast<Trk::SimObject *>(simGroup + 1);
            sint32 count = simObjBuf->m_num_elements;
            for (sint32 i = count - 1; i >= 0; --i)
                if (simObjs[i].instIndex >= objInstanceBuf->m_num_elements)
                    simObjs[i].instIndex = 127;
        }
    }
    objSpecialInstanceBuf = chunkGroup->LocateCreateGroupType(0x15, mem, 0);
    sfxBuf = chunkGroup->LocateCreateGroupType(10, mem, 0);
    simQuadBuf = chunkGroup->LocateCreateGroupType(5, mem, 0);
    simSliceBuf = chunkGroup->LocateCreateGroupType(6, mem, 0);
    lineBuf = chunkGroup->LocateCreateGroupType(9, mem, 0);
    SerializedGroup *geomGroup = chunkGroup->LocateGroupType(0x17, 0);
    SerializedGroup *geomMetadata = geomGroup->LocateGroupType(0x1b, 0);
    uint8 *geomInfo = reinterpret_cast<uint8 *>(geomMetadata + 1);
    for (sint32 i = 0; i < 6; ++i)
        quadCounts[i] = geomInfo[12 + i * 2];
    Group *quadGroup = geomGroup->LocateCreateGroupType(0x19, mem, 0);

    renderQuads[0] = quadGroup != 0 ? reinterpret_cast<Trk::Quad *>(quadGroup + 1) :
        reinterpret_cast<Trk::Quad *>(sizeof(Group));
    sint16 count;
    memcpy(&count, geomInfo + 12, 2);
    renderQuads[1] = count != 0 ? renderQuads[0] + count : renderQuads[0];
    memcpy(&count, geomInfo + 14, 2);
    renderQuads[2] = count != 0 ? renderQuads[1] + count : renderQuads[1];
    memcpy(&count, geomInfo + 20, 2);
    renderQuads[3] = count != 0 ? renderQuads[2] + count : renderQuads[2];
    stripBuf = geomGroup->LocateCreateGroupType(0x1a, mem, 0);
    lorezstripBuf = geomGroup->LocateCreateGroupType(0x25, mem, 0);
    vertexBuf = geomGroup->LocateCreateGroupType(0x18, mem, 0);
    objVertexBuf = geomGroup->LocateCreateGroupType(0x27, mem, 0);
    objQuadBuf = geomGroup->LocateCreateGroupType(0x28, mem, 0);
    objQuadInstanceBuf = geomGroup->LocateCreateGroupType(0x29, mem, 0);
}
