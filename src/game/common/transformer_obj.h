#ifndef TRANSFORMER_OBJ_H
#define TRANSFORMER_OBJ_H

#include "xport.h"
#include "game/common/mathnfs.h"
#include "game/common/transformer.h"

struct Transformer::zObj
{
    uint16 numVertex;
    uint16 numFacet;
    coorddef translation;
    COORD16 *vertex;
    COORD16 *Nvertex;
    Transformer::zFacet *facet;
};

#endif
