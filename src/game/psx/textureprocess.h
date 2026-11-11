#ifndef TEXTUREPROCESS_H
#define TEXTUREPROCESS_H

#include "xport.h"
#include "psx.h"
#include "game/common/car.h"

struct FogKey;

class Fog
{
public:
    static FogKey *gCurrentKey[2];
    static FogKey *gHeadKey;
    static FogKey gBuf[32];

    static sint32 gNumKeys;

    static FogKey * CheckRange(sint32 currentslice, FogKey *fkey);
    static FogKey * FindKey(sint32 currentslice, FogKey *fkey);
    static FogKey * AllocKey(void);
    static void AddKey(sint32 slice, sint32 distance);
    static void Update(sint32 player);
    static char * MakeTrackPathName(char *ext);
    static sint32 ReadFogKeys(void);
    static void InitFogTriggers(void);
    static void InitFogPositions(void);
    static void ReleasePositions(void);
};

class TextureProcess
{
public:
    static sint32 TransColorCheck(char *data, sint32 numentry);
    static void ColorClut(sint32 level, sint32 maxlevel, char *data, sint32 numentry, sint32 cx, sint32 cy);
    static sint32 DepthColorCluts(char *data, sint32 numentry);
    static void Init(void);
};

class CV
{
public:
    static void ProcessWorldColors_FINAL(sint32 constrast, CVECTOR *color, sint16 brightness);
    static void ColorTracks(sint32 track, sint32 weather, sint32 night);
    static void InitColorVertices(void);
};

extern sint16 gClutDepth[256][16];

#endif
