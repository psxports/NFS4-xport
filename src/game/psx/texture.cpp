#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include "xport.h"
#include "psx.h"
#include "eaclib/psx/eaclib.h"
#include "game/common/paths.h"
#include "game/psx/draw.h"
#include "game/psx/platform.h"
#include "game/psx/texture.h"
#include "game/psx/textureprocess.h"
#include "lib/psx/shpdepth.h"

struct SHAPETABLEENTRY
{
    char id[4];
    sint32 offset;
};

sint32 gNbFreePal4;
sint32 gNbFreePal8;
sint16 *gFreePal4;
sint16 *gFreePal8;
Draw::tPixMap *gMenuPixmap[8];
Draw::tPixMap gMenuPixmapAlloc[8];
sint32 Texture::CarColor;
sint32 Texture::palNum;
sint32 Texture::gNum4bitPal;
sint32 Texture::gNum8bitPal;
Texture::pal8bit *Texture::palCopy;
Draw::tPixMap **Texture::gP4bitPmx;
Draw::tPixMap **Texture::gP8bitPmx;
char **Texture::gPalette4bit;
char **Texture::gPalette8bit;

void Texture::InitPaletteSharing(void)
{
    FUNCTION_MARKER(0x800DF5B4, "NFS4.EXE");
    gNum4bitPal = 0;
    gNum8bitPal = 0;
    gP4bitPmx = reinterpret_cast<Draw::tPixMap **>(reservememadr("pshare 4", 0x800, 0x10));
    gP8bitPmx = reinterpret_cast<Draw::tPixMap **>(reservememadr("pshare 8", 0x100, 0x10));
    gPalette4bit = reinterpret_cast<char **>(reservememadr("pal 4bit", 0x800, 0x10));
    gPalette8bit = reinterpret_cast<char **>(reservememadr("pal 8bit", 0x100, 0x10));
}

void Texture::DeInitPaletteSharing(void)
{
    FUNCTION_MARKER(0x800DF630, "NFS4.EXE");
    purgememadr(gP4bitPmx);
    purgememadr(gP8bitPmx);
    purgememadr(gPalette4bit);
    purgememadr(gPalette8bit);
}

void Texture::ResetPaletteSharing(void)
{
    FUNCTION_MARKER(0x800DF678, "NFS4.EXE");
    gNum4bitPal = 0;
    gNum8bitPal = 0;
}

sint32 Texture::CheckForSharedPalette(sint32 test, char *data, Draw::tPixMap *pmx, sint32 bpp)
{
    FUNCTION_MARKER(0x800DF688, "NFS4.EXE");
    if (test == 0)
        return 0;
    sint32 paletteCount = bpp == 0 ? gNum4bitPal : gNum8bitPal;
    char **palettes = bpp == 0 ? gPalette4bit : gPalette8bit;
    sint32 wordCount = bpp == 0 ? 8 : 128;
    for (sint32 i = 0; i < paletteCount; ++i)
    {
        char *indata = data;
        char *checkdata = palettes[i];
        sint32 j = 0;
        do
        {
            uint32 checkWord;
            uint32 inputWord;
            memcpy(&checkWord, checkdata, 4);
            checkdata += 4;
            memcpy(&inputWord, indata, 4);
            indata += 4;
            if (inputWord != checkWord)
                break;
            ++j;
        } while (j < wordCount);
        if (j == wordCount)
        {
            Draw::tPixMap shared = *(bpp == 0 ? gP4bitPmx[i] : gP8bitPmx[i]);
            *pmx = shared;
            return 1;
        }
    }
    return 0;
}

void Texture::AddSharedPalette(char *ptr_to_data, Draw::tPixMap *ptr_to_pmx, sint32 bpp)
{
    FUNCTION_MARKER(0x800DF7BC, "NFS4.EXE");
    if (bpp == 0)
    {
        sint32 index = gNum4bitPal;
        gPalette4bit[index] = ptr_to_data;
        gP4bitPmx[index] = ptr_to_pmx;
        gNum4bitPal = (sint32)((uint32)index + 1U);
    }
    else if (bpp == 1)
    {
        sint32 index = gNum8bitPal;
        gPalette8bit[index] = ptr_to_data;
        gP8bitPmx[index] = ptr_to_pmx;
        gNum8bitPal = (sint32)((uint32)index + 1U);
    }
}

void Texture::InitClut(void)
{
    FUNCTION_MARKER(0x800DF834, "NFS4.EXE");
    sint16 *pal4 = gFreePal4;
    gNbFreePal4 = 0;
    for (sint32 x = 0; x < 0x100; x += 0x10)
        for (sint32 y = 0; y < 0x78; ++y)
            pal4[gNbFreePal4++] = (sint16)((y << 6) | ((x >> 4) & 0x3f));
    pal4 = gFreePal4;
    for (sint32 x = 0; x < 0x80; x += 0x10)
        for (sint32 y = 0; y < 0x20; ++y)
            pal4[gNbFreePal4++] = (sint16)((0x2000 + (y << 6)) | ((x >> 4) & 0x3f));
    sint16 *pal8 = gFreePal8;
    gNbFreePal8 = 0;
    for (sint32 x = 0; x < 0x100; x += 0x100)
        for (sint32 y = 0; y < 8; ++y)
            pal8[gNbFreePal8++] = (sint16)((0x1e00 + (y << 6)) | ((x >> 4) & 0x3f));
}

void Texture::GetClutId(sint32 bpp, sint32 *xclut, sint32 *yclut)
{
    FUNCTION_MARKER(0x800DF938, "NFS4.EXE");
    uint16 id = 0;
    if (bpp == 0)
    {
        sint32 count = gNbFreePal4;
        if (count != 0)
        {
            sint32 index = (sint32)((uint32)count - 1U);
            sint16 *freePal = gFreePal4;
            gNbFreePal4 = index;
            id = (uint16)freePal[index];
        }
    }
    else
    {
        sint32 count = gNbFreePal8;
        if (count != 0)
        {
            sint32 index = (sint32)((uint32)count - 1U);
            sint16 *freePal = gFreePal8;
            gNbFreePal8 = index;
            id = (uint16)freePal[index];
        }
    }
    *xclut = (id & 0x3F) << 4;
    *yclut = (sint32)(sint16)id >> 6;
}

void Texture::MenuReleaseClutId(sint16 id)
{
    FUNCTION_MARKER(0x800DF9A4, "NFS4.EXE");
    sint32 yclut = (sint32)id >> 6;
    if (yclut < 488)
    {
        sint32 index = gNbFreePal8;
        gFreePal8[index] = id;
        gNbFreePal8 = (sint32)((uint32)index + 1U);
    }
    else
    {
        sint32 index = gNbFreePal4;
        gFreePal4[index] = id;
        gNbFreePal4 = (sint32)((uint32)index + 1U);
    }
}

void Texture::ColorCarPalette(char *from, char *to, sint32 numentry)
{
    FUNCTION_MARKER(0x800DFA04, "NFS4.EXE");
    uint32 colorOffset = (uint32)CarColor * 32U;
    uint8 *source = (uint8 *)from + colorOffset;
    uint8 *dest = (uint8 *)to;
    uint32 remaining = (uint32)numentry;
    while (remaining != 0)
    {
        uint8 low = source[0];
        uint8 high = source[1];
        source += 2;
        dest[0] = low;
        dest[1] = high;
        dest += 2;
        --remaining;
    }
}

void Texture::CopyPalette(char *data, sint32 width, sint32 x, sint32 y)
{
    FUNCTION_MARKER(0x800DFA3C, "NFS4.EXE");
    Texture::pal8bit *palette = palCopy + palNum;
    palette->x = x;
    palette->y = y;
    uint8 *source = (uint8 *)data;
    uint8 *dest = (uint8 *)palette->pal;
    sint32 i = 0;
    while (i < width)
    {
        uint8 low = source[0];
        uint8 high = source[1];
        source += 2;
        dest[0] = low;
        dest[1] = high;
        dest += 2;
        i = (sint32)((uint32)i + 1U);
    }
    palNum = (sint32)((uint32)palNum + 1U);
}

void Texture::ProcessPaletteCopy(Texture::pal8bit *palCopy, sint32 palStart, sint32 palNum)
{
    FUNCTION_MARKER(0x800DFA9C, "NFS4.EXE");
    if (palCopy == 0)
        return;
    PSX_RECT r;
    r.w = 16;
    r.h = 1;
    union
    {
        sint16 colors[16];
        uint32 alignment;
    } tmpPal;
    if (palStart >= palNum)
        return;
    Texture::pal8bit *palette = palCopy + palStart;
    sint32 i = palStart;
    do
    {
        ColorCarPalette((char *)palette->pal, (char *)tmpPal.colors, 16);
        r.x = (sint16)palette->x;
        i = (sint32)((uint32)i + 1U);
        r.y = (sint16)palette->y;
        ++palette;
        LoadImagePSX(&r, (uint32 *)tmpPal.colors);
    } while (i < palNum);
}

void Texture::LoadImage(PSX_RECT *imgrect, uint32 *p)
{
    FUNCTION_MARKER(0x800DFB48, "NFS4.EXE");
    if (((uint16)imgrect->w & 1U) != 0)
        imgrect->h = (sint16)((uint16)imgrect->h | 1U);
    LoadImagePSX(imgrect, (uint32 *)p);
}

void Texture::Vramf(shapetbl *shp, sint32 x, sint32 y, sint32 clutx, sint32 cluty)
{
    FUNCTION_MARKER(0x800DFB8C, "NFS4.EXE");
    while (shp != 0)
    {
        uint32 type = shp->type & 0xF7U;
        if (type >= 0x40 && type < 0x44)
        {
            shp->shapex = (sint32)((uint32)x & 0xFFFU);
            shp->shapey = (sint32)((uint32)y & 0xFFFU);
            shp->type |= 8U;
            PSX_RECT r;
            r.x = (sint16)x;
            r.y = (sint16)y;
            sint32 depth = shapedepth(shp);
            sint32 roundedWidth = shp->width * depth + 15;
            r.w = (sint16)(roundedWidth / 16);
            r.h = shp->height;
            LoadImage(&r, (uint32 *)&shp->data);
        }
        else if (type == 0x23 && clutx >= 0)
        {
            shp->shapex = (sint32)((uint32)clutx & 0xFFFU);
            shp->shapey = (sint32)((uint32)cluty & 0xFFFU);
            shp->type |= 8U;
            PSX_RECT r;
            r.x = (sint16)clutx;
            r.y = (sint16)cluty;
            r.w = shp->width;
            r.h = 1;
            LoadImage(&r, (uint32 *)&shp->data);
        }
        sint32 next = shp->next;
        shp = next == 0 ? 0 : (shapetbl *)((char *)shp + next);
    }
}

void Texture::Vramcf(shapetbl *shp, sint32 x, sint32 y, sint32 clutx, sint32 cluty)
{
    FUNCTION_MARKER(0x800DFD38, "NFS4.EXE");
    sint32 depth = shapedepth(shp);
    uint32 roundedBits = ((uint32)shp->width * (uint32)depth + 15U) & ~15U;
    sint32 rowbytes = (sint32)roundedBits >> 3;
    if ((rowbytes & 2) == 0 || ((uint16)shp->height & 1U) != 0)
    {
        Vramf(shp, x, y, clutx, cluty);
        return;
    }
    sint32 height = shp->height;
    sint32 lastRow = (sint32)(((uint32)height - 1U) * (uint32)rowbytes);
    char *s = (char *)&shp->data + lastRow;
    sint32 endY = (sint32)((uint32)y + (uint32)height);
    PSX_RECT r;
    r.x = (sint16)x;
    r.y = (sint16)((uint32)endY - 2U);
    r.w = 1;
    r.h = 2;
    LoadImagePSX(&r, (uint32 *)(s - 2));
    r.x = (sint16)((uint32)x + 1U);
    r.y = (sint16)((uint32)endY - 1U);
    r.w = (sint16)((uint32)((sint32)roundedBits >> 4) - 1U);
    r.h = 1;
    LoadImagePSX(&r, (uint32 *)(s + 2));
    shp->height = (sint16)((uint16)shp->height - 1U);
    Vramf(shp, x, y, clutx, cluty);
    shp->height = (sint16)height;
}

sint32 Texture::GetTranslucencyMode(shapetbl *shp)
{
    FUNCTION_MARKER(0x800DFEA0, "NFS4.EXE");
    while (shp != 0)
    {
        if (shp->type == 0x6B)
        {
            uint32 abr = ((uint16)shp->width >> 5) & 3U;
            return abr == 3 ? 2 : (sint32)abr;
        }
        sint32 next = shp->next;
        shp = next == 0 ? 0 : (shapetbl *)((char *)shp + next);
    }
    return 0;
}

void Texture::LoadPmx(char *f, char *n, sint32 ctrl, sint32 rx, sint32 ry, sint32 cx, sint32 cy, Draw::tPixMap *pmx)
{
    FUNCTION_MARKER(0x800DFF18, "NFS4.EXE");
    shapetbl *shpptr = f != 0 ? static_cast<shapetbl *>(locateshape(f, n)) : reinterpret_cast<shapetbl *>(n);
    if (shpptr == 0)
        return;
    sint32 bshapex = shpptr->shapex;
    sint32 bshapey = shpptr->shapey;
    sint32 bpp_real = shpptr->type & 3U;
    sint32 bpp = bpp_real;
    sint32 bwidth = shpptr->width;
    sint32 bheight = shpptr->height;
    if ((ctrl & 8) != 0 && bpp != 0)
        bpp = 0;
    sint32 rotated = shpptr->transposed != 0;
    sint32 x;
    sint32 y;
    if (rx == -1)
    {
        x = 0;
        y = 160;
        shpptr->width = 1;
        shpptr->height = 1;
    }
    else
    {
        x = (sint32)((uint32)shpptr->shapex + (uint32)rx);
        y = (sint32)((uint32)shpptr->shapey + (uint32)ry);
    }
    sint32 w = shpptr->width;
    sint32 h = shpptr->height;
    if ((ctrl & 1) == 0)
    {
        --w;
        --h;
    }
    if (bpp == 2)
        Vramcf(shpptr, x, y, 0, 0);
    else
    {
        shapetbl *clutptr = (shapetbl *)((char *)shpptr + shpptr->next);
        sint32 bwidthclut = clutptr->width;
        clutptr->width = bpp == 0 ? 16 : 256;
        char *paletteData = &clutptr->data;
        if (CheckForSharedPalette(ctrl & 0x40, paletteData, pmx, bpp) == 0)
        {
            if (cx == -1)
                GetClutId(bpp, &cx, &cy);
            else if (cx == -3)
                pmx->pad2 = (uint16)TextureProcess::DepthColorCluts(paletteData, clutptr->width);
            else if (cx == -2)
            {
                cy = 160;
                cx = 0;
                clutptr->width = 1;
            }
            if ((ctrl & 2) != 0)
            {
                sint32 transparency = TextureProcess::TransColorCheck(&clutptr->data, clutptr->width);
                pmx->flag = (uint16)((uint32)transparency << 1);
            }
            if ((ctrl & 8) != 0 && bpp_real != 0)
            {
                if (palCopy != 0)
                    CopyPalette(&clutptr->data, bwidthclut, cx, cy);
                if ((ctrl & 0x10) != 0)
                    ColorCarPalette(&clutptr->data, &clutptr->data, 16);
            }
            if (cx < 0)
                pmx->clut = 0xFFFF;
            else
                pmx->clut = GetClut(cx, cy);
            if ((ctrl & 0x40) != 0)
                AddSharedPalette(&clutptr->data, pmx, bpp);
        }
        if ((ctrl & 0x20) != 0)
            Vramcf(shpptr, x, y, -1, -1);
        else
            Vramcf(shpptr, x, y, cx, cy);
        clutptr->width = (sint16)bwidthclut;
    }
    sint32 translucency = GetTranslucencyMode(shpptr);
    pmx->tpage = GetTPage(bpp, translucency, x, y);
    sint32 u = x % 64;
    sint32 v = y % 256;
    if (bpp == 0)
        u *= 4;
    else if (bpp == 1)
        u *= 2;
    if (rotated != 0)
    {
        pmx->u0 = (uint8)u;
        pmx->v0 = (uint8)(v + h);
        pmx->u1 = (uint8)u;
        pmx->v1 = (uint8)v;
        pmx->u2 = (uint8)(u + w);
        pmx->v2 = (uint8)(v + h);
        pmx->u3 = (uint8)(u + w);
        pmx->v3 = (uint8)v;
    }
    else
    {
        pmx->u0 = (uint8)u;
        pmx->v0 = (uint8)v;
        pmx->u1 = (uint8)(u + w);
        pmx->v1 = (uint8)v;
        pmx->u2 = (uint8)u;
        pmx->v2 = (uint8)(v + h);
        pmx->u3 = (uint8)(u + w);
        pmx->v3 = (uint8)(v + h);
    }
    shpptr->width = (sint16)bwidth;
    shpptr->height = (sint16)bheight;
    shpptr->shapex = bshapex;
    shpptr->shapey = bshapey;
}

void Texture::CloneUVPmx(Draw::tPixMap *pmx, sint32 mode, Draw::tPixMap *rpmx)
{
    FUNCTION_MARKER(0x800E0358, "NFS4.EXE");
    Draw::tPixMap original = *pmx;
    *rpmx = original;
    uint8 u, v;
    switch (mode) {
    case 0:
        u = pmx->u1; v = pmx->v1;
        rpmx->u0 = u; rpmx->v0 = v;
        u = pmx->u0; v = pmx->v0;
        rpmx->u1 = u; rpmx->v1 = v;
        u = pmx->u3; v = pmx->v3;
        rpmx->u2 = u; rpmx->v2 = v;
        u = pmx->u2; v = pmx->v2;
        rpmx->u3 = u; rpmx->v3 = v;
        break;
    case 1:
        u = pmx->u1; v = pmx->v1;
        rpmx->u3 = u; rpmx->v3 = v;
        u = pmx->u3; v = pmx->v3;
        rpmx->u1 = u; rpmx->v1 = v;
        u = pmx->u0; v = pmx->v0;
        rpmx->u2 = u; rpmx->v2 = v;
        u = pmx->u2; v = pmx->v2;
        rpmx->u0 = u; rpmx->v0 = v;
        break;
    case 2:
        u = pmx->u2; v = pmx->v2;
        rpmx->u0 = u; rpmx->v0 = v;
        u = pmx->u0; v = pmx->v0;
        rpmx->u1 = u; rpmx->v1 = v;
        u = pmx->u1; v = pmx->v1;
        rpmx->u3 = u; rpmx->v3 = v;
        u = pmx->u3; v = pmx->v3;
        rpmx->u2 = u; rpmx->v2 = v;
        break;
    case 3:
        u = pmx->u3; v = pmx->v3;
        rpmx->u0 = u; rpmx->v0 = v;
        u = pmx->u2; v = pmx->v2;
        rpmx->u1 = u; rpmx->v1 = v;
        u = pmx->u0; v = pmx->v0;
        rpmx->u3 = u; rpmx->v3 = v;
        u = pmx->u1; v = pmx->v1;
        rpmx->u2 = u; rpmx->v2 = v;
        break;
    case 4:
        u = pmx->u1; v = pmx->v1;
        rpmx->u0 = u; rpmx->v0 = v;
        u = pmx->u3; v = pmx->v3;
        rpmx->u1 = u; rpmx->v1 = v;
        u = pmx->u2; v = pmx->v2;
        rpmx->u3 = u; rpmx->v3 = v;
        u = pmx->u0; v = pmx->v0;
        rpmx->u2 = u; rpmx->v2 = v;
        break;
    }
}

void Texture::InitTrackTexture(void)
{
    FUNCTION_MARKER(0x800E04B8, "NFS4.EXE");
    static char cluts4[] = "4 bit Cluts";
    static char cluts8[] = "8 bit Cluts";
    gFreePal4 = reinterpret_cast<sint16 *>(Platform::GetDCTBuffer(0x13f0, cluts4));
    gFreePal8 = reinterpret_cast<sint16 *>(Platform::GetDCTBuffer(0x10, cluts8));
    InitPaletteSharing();
    TextureProcess::Init();
    InitClut();
}

void Texture::InitMenuClut(void)
{
    FUNCTION_MARKER(0x800E0510, "NFS4.EXE");
    static char cluts4[] = "4 bits cluts";
    static char cluts8[] = "8 bits cluts";
    gFreePal4 = (sint16 *)reservememadr(cluts4, 0x300, 0);
    gFreePal8 = (sint16 *)reservememadr(cluts8, 0x1180, 0);
    sint16 *pal4 = gFreePal4;
    gNbFreePal4 = 0;
    for (sint32 x = 0; x < 256; x += 16)
        for (sint32 y = 0; y < 24; ++y)
        {
            sint32 index = gNbFreePal4;
            pal4[index] = (sint16)((0x7A00 + y * 64) | (((x + 512) >> 4) & 63));
            gNbFreePal4 = index + 1;
        }
    sint16 *pal8 = gFreePal8;
    gNbFreePal8 = 0;
    for (sint32 x = 0; x < 256; x += 256)
        for (sint32 y = 0; y < 140; ++y)
        {
            sint32 index = gNbFreePal8;
            pal8[index] = (sint16)((0x5700 + y * 64) | (((x + 512) >> 4) & 63));
            gNbFreePal8 = index + 1;
        }
}

void Texture::InitMenuTexture(void)
{
    FUNCTION_MARKER(0x800E0610, "NFS4.EXE");
    for (sint32 i = 7; i >= 0; --i)
        gMenuPixmap[i] = 0;
}

void Texture::CleanupMenuTexture(void)
{
    FUNCTION_MARKER(0x800E0638, "NFS4.EXE");
    for (sint32 i = 0; i < 8; ++i)
    {
        Draw::tPixMap *pmx = gMenuPixmap[i];
        if (pmx != 0)
            MenuReleaseClutId((sint16)pmx->clut);
        gMenuPixmap[i] = 0;
    }
}

void Texture::LoadMenuTexture(void)
{
    FUNCTION_MARKER(0x800E0698, "NFS4.EXE");
    if (gMenuPixmap[0] != 0)
        return;
    char name[255];
    sprintf(name, "%sshow.psh", Paths_Paths[25]);
    char *shpfile = static_cast<char *>(loadfileadr(name, 0));
    ResetPaletteSharing();
    static char shapeNames[6][8] = { "show", "shad", "lgt3", "lgta", "lgtb", "lgtc" };
    static const sint32 slots[6] = { 0, 1, 3, 4, 5, 6 };
    for (sint32 i = 0; i < 6; ++i)
    {
        Draw::tPixMap *pmx = &gMenuPixmapAlloc[i];
        LoadPmx(shpfile, shapeNames[i], 0, 0x380, 0x180, -1, -1, pmx);
        gMenuPixmap[slots[i]] = pmx;
    }
    purgememadr(shpfile);
}

void Texture::KillTrackTexture(void)
{
    FUNCTION_MARKER(0x800E084C, "NFS4.EXE");
    return;
}

void Texture::KillMenuTexture(void)
{
    FUNCTION_MARKER(0x800E0854, "NFS4.EXE");
    purgememadr(gFreePal4);
    purgememadr(gFreePal8);
}
