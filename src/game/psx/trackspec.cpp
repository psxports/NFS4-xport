#include <stddef.h>
#include <stdio.h>
#include "xport.h"
#include "psx.h"
#include "eaclib/psx/eaclib.h"
#include "game/common/gmesetup.h"
#include "game/common/paths.h"
#include "game/psx/trackspec.h"

struct CTrackSpecHeader
{
    sint32 version;
    sint32 num_spec;
};

sint32 TrackSpec::gPrevSpec;
sint32 TrackSpec::gCurrentSpec;
sint32 TrackSpec::gMaxSpec;
CTrackSpec TrackSpec::gSpec;

void read(char **handle, void *buf, sint32 bytes)
{
    FUNCTION_MARKER(0x800E17A0, "NFS4.EXE");
    char *source = *handle;
    char *dest = static_cast<char *>(buf);
    for (sint32 i = 0; i < bytes; ++i) *dest++ = *source++;
    *handle = source;
}

void TrackSpec::SetDefault(CTrackSpec *spec)
{
    FUNCTION_MARKER(0x800E1544, "NFS4.EXE");
    spec->fogstate = 0;
    sint16 weather = (sint16)GameSetup::gData.Weather;
    spec->horizonstate = 1;
    spec->skystate = 1;
    spec->weatherstate = weather;
    sint16 night = (sint16)GameSetup::gData.Time;
    spec->fogspec.contrast = 0x10000;
    spec->depthcuestate = 1;
    spec->fogspec.start = 200;
    spec->fogspec.dist2base = 8;
    spec->weatherspec.intensity_limit = 2;
    spec->horizonspec.mirror = 1;
    spec->horizonspec.yoffset = -0x1080;
    spec->fogspec.color.r = spec->fogspec.color.g = spec->fogspec.color.b = 128;
    spec->weatherspec.type = 0;
    spec->horizonspec.angle = 0;
    spec->horizonspec.height = 0x4b00;
    for (sint32 i = 0; i < 2; ++i) {
        spec->horizonspec.frontColor[i].r = spec->horizonspec.frontColor[i].g = spec->horizonspec.frontColor[i].b = 128;
        spec->horizonspec.backColor[i].r = spec->horizonspec.backColor[i].g = spec->horizonspec.backColor[i].b = 128;
    }
    spec->nightstate = night;
    for (sint32 i = 0; i < 16; ++i)
        spec->horizonspec.ringPMX[i] = (char)(i < 8 ? i : 23 - i);
    spec->skyspec.type = 0;
    spec->skyspec.flags = 4;
    for (sint32 i = 0; i < 5; ++i) {
        spec->skyspec.frontcolors[i].r = spec->skyspec.frontcolors[i].g = 50;
        spec->skyspec.frontcolors[i].b = 70;
        spec->skyspec.backcolors[i].r = spec->skyspec.backcolors[i].g = 50;
        spec->skyspec.backcolors[i].b = 70;
        for (sint32 j = 0; j < 4; ++j)
            spec->skyspec.cloudIndices[i][j] = 0;
        spec->skyspec.ringAngles[i] = i << 12;
    }
    spec->skyspec.sunAngleInSky = -0x1848;
    spec->skyspec.sunHeightInSky = 0xee;
    spec->skyspec.moonHeightInSky = 0xee;
    spec->skyspec.numStars = 60;
    spec->skyspec.starAngleLow = 4000;
    spec->skyspec.starAngleHigh = 10000;
    spec->skyspec.starBrightMin = 64;
    spec->skyspec.starBrightMax = 200;
    spec->skyspec.starBaseColor.r = spec->skyspec.starBaseColor.g = spec->skyspec.starBaseColor.b = 255;
    spec->skyspec.starRandomSeed = 12345;
    spec->skyspec.sunBeamColor.r = spec->skyspec.sunBeamColor.g = 33;
    spec->skyspec.sunHaloColor.r = 25;
    spec->skyspec.clearcolor.g = 16;
    spec->skyspec.sunBeamColor.b = 16;
    spec->nightspec.nightcolor.g = spec->nightspec.nightcolor.b = 16;
    spec->skyspec.sunHaloColor.g = 10;
    spec->depthcuespec.distance = 68;
    spec->skyspec.clearcolor.r = spec->skyspec.clearcolor.b = 8;
    spec->skyspec.moonAngleInSky = 0;
    spec->skyspec.sunHaloColor.b = 0;
    spec->skyspec.yoffset = 0;
    spec->nightspec.nightcolor.r = 8;
    spec->depthcuespec.color.r = spec->depthcuespec.color.g = spec->depthcuespec.color.b = 128;
    spec->worldcolorspec.worldR = spec->worldcolorspec.worldG = spec->worldcolorspec.worldB = spec->worldcolorspec.type = 0;
    spec->worldcolorspec.contrast = 0x4ccc;
    spec->worldcolorspec.contrast_color.r = spec->worldcolorspec.contrast_color.g = spec->worldcolorspec.contrast_color.b = 128;
}

void TrackSpec::SetUp(void)
{
    FUNCTION_MARKER(0x800E177C, "NFS4.EXE");
    GameSetup::gData.Time = gSpec.nightstate;
    GameSetup::gData.Weather = gSpec.weatherstate;
}

void TrackSpec::Read(sint32 spec_num)
{
    FUNCTION_MARKER(0x800E17D0, "NFS4.EXE");
    char str[64];
    sprintf(str, "%sTr%02d.bin", Paths_Paths[6], GameSetup::gData.track);
    char *startpos = static_cast<char *>(loadfileadr(str, 0));
    char *currentpos = startpos;
    if (startpos != 0) {
        gPrevSpec = spec_num;
        gCurrentSpec = spec_num;
        CTrackSpecHeader header;
        read(&currentpos, &header, 8);
        if (spec_num < header.num_spec) {
            gMaxSpec = (header.num_spec + 1);
            currentpos += (spec_num * 0x108);
            read(&currentpos, &gSpec, 0x108);
            SetUp();
        } else {
            SetDefault(&gSpec);
        }
        purgememadr(startpos);
    }
}

void TrackSpec::Load(sint32 weather, sint32 night)
{
    FUNCTION_MARKER(0x800E18CC, "NFS4.EXE");
    sint32 spec[2][2] = { { 0, 1 }, { 2, 3 } };
    Read(spec[weather][night]);
}
