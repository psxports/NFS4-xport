#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include "xport.h"
#include "psx.h"
#include "eaclib/psx/eaclib.h"
#include "game/common/bworldsm.h"
#include "game/common/carobj.h"
#include "game/common/chunk.h"
#include "game/common/gmesetup.h"
#include "game/common/paths.h"
#include "game/common/render.h"
#include "game/common/renderview.h"
#include "game/psx/texture.h"
#include "game/psx/textureprocess.h"
#include "game/psx/trackspec.h"
#include "lib/psx/fixdmult.h"

struct CSVECTOR;

class TP
{
public:
    struct ZPaletteSystem;

    typedef ZPaletteSystem tZPaletteSystem;
    static sint32 gColorMode;
    static CSVECTOR gBlendColor;
    static ZPaletteSystem gZPaletteSystem;
};

struct TP::ZPaletteSystem

{
    sint32 numdepthclut;
};

struct FogKey;

struct FogKey
{
    sint16 distance;
    sint16 slice;

    FogKey *next;
    FogKey *prev;
};

struct CSVECTOR;

struct CSVECTOR

{
    sint32 r;

    sint32 g;

    sint32 b;
};

struct FogKey;

sint32 gZDepth;
sint32 gContrastScale = 65536;
sint16 gClutDepth[256][16];
sint32 openkeys[32];
BWorldSm::Pos *fogslicePos;
sint32 Fog::gNumKeys;
sint32 TP::gColorMode;
CSVECTOR TP::gBlendColor;
TP::ZPaletteSystem TP::gZPaletteSystem;
FogKey *Fog::gHeadKey;
CVECTOR Texture::gWorldColor = { 255, 255, 255, 0 };
CVECTOR Texture::gContrastColor = { 128, 128, 128, 0 };
FogKey *Fog::gCurrentKey[2];
FogKey Fog::gBuf[32];

sint32 TextureProcess::TransColorCheck(char *data, sint32 numentry)
{
    FUNCTION_MARKER(0x800E0884, "NFS4.EXE");
    sint32 translucent_flag = 0;
    uint32 remaining = (uint32)numentry;
    while (remaining != 0)
    {
        uint16 color;
        memcpy(&color, data, 2);
        data += 2;
        translucent_flag |= color > 0x8000U;
        --remaining;
    }
    return translucent_flag;
}

void TextureProcess::ColorClut(sint32 level, sint32 maxlevel, char *data, sint32 numentry, sint32 cx, sint32 cy)
{
    FUNCTION_MARKER(0x800E08B8, "NFS4.EXE");
    sint16 newdata[256];
    char *sourcedata = data;
    sint32 numerator = (sint32)((uint32)TrackSpec::gSpec.fogspec.contrast * (uint32)level);
    sint32 denominator = (sint32)((uint32)maxlevel - 1U);

    sint32 contrasttemp = numerator / denominator;
    for (sint32 j = 0; j < numentry; ++j)
    {
        uint16 color;
        memcpy(&color, sourcedata, 2);
        if (color == 0)
            newdata[j] = 0;
        else
        {
            sint32 b = (color >> 7) & 0xF8;
            sint32 g = (color >> 2) & 0xF8;
            sint32 r = (color & 31) << 3;
            sint32 temp = fixedmult(r - TrackSpec::gSpec.fogspec.color.r, contrasttemp);
            r = (sint32)((uint32)r - (uint32)temp);
            if (r < 0)
                r = 0;
            else if (r > 255)
                r = 255;
            temp = fixedmult(g - TrackSpec::gSpec.fogspec.color.g, contrasttemp);
            g = (sint32)((uint32)g - (uint32)temp);
            if (g < 0)
                g = 0;
            else if (g > 255)
                g = 255;
            temp = fixedmult(b - TrackSpec::gSpec.fogspec.color.b, contrasttemp);
            b = (sint32)((uint32)b - (uint32)temp);
            if (b < 0)
                b = 0;
            else if (b > 255)
                b = 255;
            if (b < 8)
                b = 8;
            newdata[j] = (sint16)((color & 0x8000U) | (((uint32)b >> 3) << 10) | (((uint32)g >> 3) << 5) | ((uint32)r >> 3));
        }
        sourcedata += 2;
    }
    PSX_RECT r;
    r.x = (sint16)cx;
    r.y = (sint16)cy;
    r.w = (sint16)numentry;
    r.h = 1;
    LoadImagePSX(&r, (uint32 *)newdata);
}

sint32 TextureProcess::DepthColorCluts(char *data, sint32 numentry)
{
    FUNCTION_MARKER(0x800E0AC0, "NFS4.EXE");
    DrawSync(0);
    for (sint32 i = 0; i < 16; ++i)
    {
        sint32 cx;
        sint32 cy;
        Texture::GetClutId(numentry >= 17, &cx, &cy);
        uint32 id = ((uint32)(uint16)cy << 6) | ((uint32)(cx >> 4) & 63U);
        gClutDepth[TP::gZPaletteSystem.numdepthclut][i] = (sint16)id;
        ColorClut(i, 16, data, numentry, cx, cy);
    }
    sint32 index = TP::gZPaletteSystem.numdepthclut;
    TP::gZPaletteSystem.numdepthclut = (sint32)((uint32)index + 1U);
    return index;
}

FogKey * Fog::CheckRange(sint32 currentslice, FogKey *fkey)
{
    FUNCTION_MARKER(0x800E0B98, "NFS4.EXE");
    FogKey *keynext = fkey->next;
    sint32 slice = fkey->slice;
    sint32 nextslice = keynext->slice;
    if (nextslice < slice)
        return currentslice >= slice || currentslice < nextslice ? fkey : 0;
    return currentslice >= slice && currentslice < nextslice ? fkey : 0;
}

FogKey * Fog::FindKey(sint32 currentslice, FogKey *fkey)
{
    FUNCTION_MARKER(0x800E0BEC, "NFS4.EXE");
    if (fkey == 0)
        fkey = gHeadKey;
    if (CheckRange(currentslice, fkey) != 0)
        return fkey;
    if (currentslice < fkey->slice)
    {
        do
        {
            fkey = fkey->prev;
        } while (CheckRange(currentslice, fkey) == 0);
    }
    else
    {
        do
        {
            fkey = fkey->next;
        } while (CheckRange(currentslice, fkey) == 0);
    }
    return fkey;
}

FogKey * Fog::AllocKey(void)
{
    FUNCTION_MARKER(0x800E0C80, "NFS4.EXE");
    for (sint32 i = 0; i < 32; ++i)
    {
        if (openkeys[i] == 1)
        {
            openkeys[i] = 0;
            return &gBuf[i];
        }
    }
    return 0;
}

void Fog::AddKey(sint32 slice, sint32 distance)
{
    FUNCTION_MARKER(0x800E0CCC, "NFS4.EXE");
    FogKey *newkey = AllocKey();
    FogKey *insertkey = gHeadKey;
    newkey->prev = 0;
    newkey->next = 0;
    newkey->slice = (sint16)slice;
    newkey->distance = (sint16)distance;
    if (insertkey == 0)
    {
        gHeadKey = newkey;
        newkey->next = newkey;
        newkey->prev = newkey;
    }
    else
    {
        if (insertkey->next != insertkey)
            insertkey = FindKey(slice, 0);
        if (insertkey->slice == slice)
            return;
        FogKey *savenextkey = insertkey->next;
        insertkey->next = newkey;
        newkey->prev = insertkey;
        newkey->next = savenextkey;
        savenextkey->prev = newkey;
    }
    gNumKeys = (sint32)((uint32)gNumKeys + 1U);
}

void Fog::Update(sint32 player)
{
    FUNCTION_MARKER(0x800E0D80, "NFS4.EXE");
    if (gNumKeys == 1)
        return;
    BWorldSm::FindClosestQuadRez(&gCView.cview.translation, &fogslicePos[player], 1);
    sint32 currentslice = fogslicePos[player].slice;
    FogKey *key = FindKey(currentslice, gCurrentKey[player]);
    gCurrentKey[player] = key;
    FogKey *nextkey = key->next;
    sint32 final_dist = key->distance;
    sint32 nextslice = nextkey->slice;
    if (final_dist != nextkey->distance)
    {
        if (nextslice < key->slice)
        {
            sint32 numslices = gNumSlices;
            nextslice = (sint32)((uint32)nextslice + (uint32)numslices);
            if (currentslice >= 0 && currentslice < key->slice)
                currentslice = (sint32)((uint32)currentslice + (uint32)numslices);
        }
        nextkey = key->next;
        sint32 distance = key->distance;
        sint32 diffdistance = nextkey->distance - distance;
        sint32 delta = (sint32)((uint32)currentslice - (uint32)key->slice);
        sint32 numerator = (sint32)((uint32)delta * (uint32)diffdistance);
        sint32 diffslice = (sint32)((uint32)nextslice - (uint32)key->slice);
        final_dist = (sint32)((uint32)distance + (uint32)(numerator / diffslice));
    }
    TrackSpec::gSpec.fogspec.start = final_dist;
}

char * Fog::MakeTrackPathName(char *ext)
{
    FUNCTION_MARKER(0x800E0EC4, "NFS4.EXE");
    static char fogstrspc[64];
    sprintf(fogstrspc, "%sTr%02d%s", Paths_Paths[6], GameSetup::gData.track & 15, ext);
    return fogstrspc;
}

sint32 Fog::ReadFogKeys(void)
{
    FUNCTION_MARKER(0x800E0F14, "NFS4.EXE");
    static char snow[] = "S.fog";
    static char night[] = "N.fog";
    static char weather[] = "W.fog";
    static char normal[] = ".fog";
    char *ext;
    if (GameSetup::gData.Time != 0)
        ext = GameSetup::gData.Weather != 0 ? snow : night;
    else
        ext = GameSetup::gData.Weather != 0 ? weather : normal;
    char *strspc = MakeTrackPathName(ext);
    sint32 *readmem = static_cast<sint32 *>(loadfileadr(strspc, 0));
    if (readmem == 0)
        return 0;
    sint32 numkeys = readmem[0];
    if ((uint32)numkeys >= 32U)
        return 0;
    for (sint32 i = 0; i < numkeys; ++i)
    {
        sint32 slice = readmem[1 + i * 2];
        sint32 distance = readmem[2 + i * 2];
        AddKey(slice, distance);
    }
    purgememadr(readmem);
    return 1;
}

void Fog::InitFogTriggers(void)
{
    FUNCTION_MARKER(0x800E102C, "NFS4.EXE");
    gNumKeys = 0;
    gHeadKey = 0;
    for (sint32 i = 31; i >= 0; --i)
        openkeys[i] = 1;
    if (ReadFogKeys() == 0)
        AddKey(0, TrackSpec::gSpec.fogspec.start);
    FogKey *head = gHeadKey;
    sint32 num_player = GameSetup::gData.commMode == 1 ? 2 : 1;
    gCurrentKey[0] = head;
    gCurrentKey[1] = head;
    static char label[] = "fog pos";
    fogslicePos = static_cast<BWorldSm::Pos *>(reservememadr(label, num_player * 132, 0));
}

void Fog::InitFogPositions(void)
{
    sint32 num_player = GameSetup::gData.commMode == 1 ? 2 : 1;
    for (sint32 k = 0; k < num_player; ++k)
        BWorldSm::SetSlice(0, &fogslicePos[k]);
}

void Fog::ReleasePositions(void)
{
    purgememadr(fogslicePos);
}

void TextureProcess::Init(void)
{
    FUNCTION_MARKER(0x800E1134, "NFS4.EXE");
    gZDepth = 0x10;
    TP::gZPaletteSystem.numdepthclut = 0;
}

void CV::ProcessWorldColors_FINAL(sint32 constrast, CVECTOR *color, sint16 brightness)
{
    FUNCTION_MARKER(0x800E1148, "NFS4.EXE");
    for (sint32 i = 0; i < Chunk::numLight; ++i)
    {
        CVECTOR *source = &Chunk::lightTable[i];
        sint32 r = source->r;
        sint32 g = source->g;
        sint32 b = source->b;
        sint32 temp = fixedmult(r - color->r, constrast);
        r -= temp;
        if (r < 0)
            r = 0;
        else if (r > 255)
            r = 255;
        temp = fixedmult(g - color->g, constrast);
        g -= temp;
        if (g < 0)
            g = 0;
        else if (g > 255)
            g = 255;
        temp = fixedmult(b - color->b, constrast);
        b -= temp;
        if (b < 0)
            b = 0;
        else if (b > 255)
            b = 255;
        r += brightness;
        g += brightness;
        b += brightness;
        if (r > 255)
            r = 255;
        if (r < 0)
            r = 0;
        if (g > 255)
            g = 255;
        if (g < 0)
            g = 0;
        if (b > 255)
            b = 255;
        if (b < 0)
            b = 0;
        Chunk::lightTable[i].r = (uint8)r;
        Chunk::lightTable[i].g = (uint8)g;
        Chunk::lightTable[i].b = (uint8)b;
    }
}

void CV::ColorTracks(sint32 track, sint32 weather, sint32 night)
{
    FUNCTION_MARKER(0x800E1308, "NFS4.EXE");
    sint32 contrast = 0;
    CVECTOR color = { 0, 0, 0, 0 };
    sint16 brightness = 0;
    if (track == 8 && weather == 0 && night == 1)
    {
        contrast = 0x6800;
        brightness = 0x16;
    }
    else if (GameSetup::gData.commMode == 1)
    {
        if (track == 2 && weather == 1 && night == 1)
        {
            contrast = -0x9C80;
            color.g = 0x10;
            brightness = 0x10;
        }
        else if (track == 3 && weather == 0 && night == 1)
            contrast = -0xF400;
        else if (track == 3 && weather == 1 && night == 1)
        {
            contrast = -0x7400;
            color.r = 0x10;
            brightness = 0x10;
        }
        else if (track == 4 && weather == 0 && night == 1)
            contrast = -0x7800;
        else if (track == 4 && weather == 1 && night == 1)
        {
            contrast = -0x10000;
            color.g = 0x18;
            brightness = 0x10;
        }
        else if (track == 6 && weather == 0 && night == 1)
        {
            contrast = -0x9610;
            brightness = 0x31;
        }
        else if (track == 7 && weather == 0 && night == 1)
        {
            contrast = -0xB400;
            brightness = 0x14;
        }
        else if (track == 10 && weather == 1 && night == 1)
            contrast = -0x5400;
    }
    ProcessWorldColors_FINAL(contrast, &color, brightness);
}

void CV::InitColorVertices(void)
{
    FUNCTION_MARKER(0x800E1510, "NFS4.EXE");
    ColorTracks(GameSetup::gData.track & 15, GameSetup::gData.Weather, GameSetup::gData.Time);
}
