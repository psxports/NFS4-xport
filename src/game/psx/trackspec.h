#ifndef TRACKSPEC_H
#define TRACKSPEC_H

#include "xport.h"
#include "psx.h"
#include "game/common/car.h"

struct CFogSpec
{
    sint32 contrast;
    CVECTOR color;
    sint32 start;
    sint32 dist2base;
};

struct CWeatherSpec
{
    sint32 type;
    sint32 intensity_limit;
};

struct CHorizonSpec
{
    sint32 mirror;
    sint32 angle;
    sint32 yoffset;
    sint32 height;
    CVECTOR frontColor[2];
    CVECTOR backColor[2];
    char ringPMX[16];
};

struct CSkySpec
{
    sint32 type;
    sint32 flags;
    CVECTOR frontcolors[5];
    CVECTOR backcolors[5];
    CVECTOR clearcolor;
    sint32 sunAngleInSky;
    sint32 sunHeightInSky;
    sint32 moonAngleInSky;
    sint32 moonHeightInSky;
    sint32 numStars;
    sint32 starAngleLow;
    sint32 starAngleHigh;
    sint32 starBrightMin;
    sint32 starBrightMax;
    CVECTOR starBaseColor;
    sint32 starRandomSeed;
    CVECTOR sunBeamColor;
    CVECTOR sunHaloColor;
    sint32 yoffset;
    char cloudIndices[5][4];
    sint32 ringAngles[5];
};

struct CNightSpec
{
    CVECTOR nightcolor;
};

struct CDepthCueSpec
{
    CVECTOR color;
    sint32 distance;
};

struct CWorldColor
{
    sint32 contrast;
    CVECTOR contrast_color;
    sint16 worldR;
    sint16 worldG;
    sint16 worldB;
    sint16 type;
};

struct CTrackSpec
{
    sint16 fogstate;
    sint16 weatherstate;
    sint16 horizonstate;
    sint16 skystate;
    sint16 nightstate;
    sint16 depthcuestate;
    sint16 worldcolorstate;
    sint16 pad0;
    CFogSpec fogspec;
    CWeatherSpec weatherspec;
    CHorizonSpec horizonspec;
    CSkySpec skyspec;
    CNightSpec nightspec;
    CDepthCueSpec depthcuespec;
    CWorldColor worldcolorspec;
};

class TrackSpec
{
public:
    static CTrackSpec gSpec;

    static sint32 gPrevSpec;
    static sint32 gCurrentSpec;
    static sint32 gMaxSpec;

    static void SetDefault(CTrackSpec *spec);
    static void SetUp(void);
    static void Read(sint32 spec_num);
    static void Load(sint32 weather, sint32 night);
};

#endif
