#ifndef TRANSFORMER_H
#define TRANSFORMER_H

#include "xport.h"
#include "game/common/coorddef.h"

class Transformer
{
public:
    typedef COORD16 zVertex;

    struct zObj;

    struct zOverlay

    {
        sint16 offset;

        uint8 u;

        uint8 v;
    };

    struct zUV
    {
        uint8 u;
        uint8 v;
    };

    struct zFacet
    {
        sint16 flag;
        uint8 textureIndex;
        uint8 vertexId0;
        uint8 vertexId1;
        uint8 vertexId2;
        zUV uv0;
        zUV uv1;
        zUV uv2;
    };

    struct zScene
    {
        zObj *obj[57];
        zOverlay overlay[90];
    };

};

#endif
