#ifndef TEXTURE_H
#define TEXTURE_H

#include "xport.h"
#include "psx.h"
#ifdef __cplusplus
#include "game/common/car.h"
#include "game/psx/draw.h"
#endif

typedef struct shapetbl shapetbl;

struct shapetbl
{
    uint32 type : 8;
    sint32 next : 24;
    sint16 width;
    sint16 height;
    sint16 centerx;
    sint16 centery;
    sint32 shapex : 12;
    uint32 reserved : 1;
    uint32 twiddled : 1;
    uint32 transposed : 1;
    uint32 rotated : 1;
    sint32 shapey : 12;
    uint32 mipmaps : 4;
    char data;
};

#ifdef __cplusplus
class Texture
{
public:
    static CVECTOR gWorldColor;
    static CVECTOR gContrastColor;

    struct pal8bit
    {
        sint32 x;
        sint32 y;
        sint16 pal[256];
    };

    static sint32 CarColor;
    static sint32 palNum;
    static sint32 gNum4bitPal;
    static sint32 gNum8bitPal;
    static pal8bit * palCopy;
    static Draw::tPixMap ** gP4bitPmx;
    static Draw::tPixMap ** gP8bitPmx;
    static char ** gPalette4bit;
    static char ** gPalette8bit;

    static void InitPaletteSharing(void);
    static void DeInitPaletteSharing(void);
    static void ResetPaletteSharing(void);
    static sint32 CheckForSharedPalette(sint32 test, char *data, Draw::tPixMap *pmx, sint32 bpp);
    static void AddSharedPalette(char *ptr_to_data, Draw::tPixMap *ptr_to_pmx, sint32 bpp);
    static void InitClut(void);
    static void GetClutId(sint32 bpp, sint32 *xclut, sint32 *yclut);
    static void MenuReleaseClutId(sint16 id);
    static void ColorCarPalette(char *from, char *to, sint32 numentry);
    static void CopyPalette(char *data, sint32 width, sint32 x, sint32 y);
    static void ProcessPaletteCopy(pal8bit *palCopy, sint32 palStart, sint32 palNum);
    static void LoadImage(PSX_RECT *imgrect, uint32 *p);
    static void Vramf(shapetbl *shp, sint32 x, sint32 y, sint32 clutx, sint32 cluty);
    static void Vramcf(shapetbl *shp, sint32 x, sint32 y, sint32 clutx, sint32 cluty);
    static sint32 GetTranslucencyMode(shapetbl *shp);
    static void LoadPmx(char *f, char *n, sint32 ctrl, sint32 rx, sint32 ry, sint32 cx, sint32 cy, Draw::tPixMap *pmx);
    static void CloneUVPmx(Draw::tPixMap *pmx, sint32 mode, Draw::tPixMap *rpmx);
    static void InitTrackTexture(void);
    static void InitMenuClut(void);
    static void InitMenuTexture(void);
    static void CleanupMenuTexture(void);
    static void LoadMenuTexture(void);
    static void KillTrackTexture(void);
    static void KillMenuTexture(void);
};
#endif

#ifdef __cplusplus
extern Draw::tPixMap *gMenuPixmap[8];
#endif

#endif
