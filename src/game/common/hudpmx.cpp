#include <stdio.h>
#include "xport.h"
#include "eaclib/psx/eaclib.h"
#include "game/common/dashhud.h"
#include "game/common/gmesetup.h"
#include "game/common/hudpmx.h"
#include "game/common/paths.h"
#include "game/psx/draw.h"
#include "game/psx/font.h"
#include "game/psx/texture.h"

static char loadShapeXOff;
static char *gShpfile;
static char *gHudFont;
HudPmx::tUV HudPmx::gHudNumberUV[10];
HudPmx::tShape HudPmx::gShapes[175];

void HudPmx::LoadShape(char *n, HudPmx::tShape *s)
{
    FUNCTION_MARKER(0x80092464, "NFS4.EXE");
    shapetbl *shp = static_cast<shapetbl *>(locateshape(gShpfile, n));
    if (shp == 0)
    {
        *s = tShape();
        return;
    }
    sint16 width = shp->width;
    char *f = gShpfile;
    sint32 xOff = static_cast<uint8>(loadShapeXOff) + 0x80;
    s->width = width;
    s->height = shp->height;
    Texture::LoadPmx(f, n, 0x41, xOff, 0x80, -1, -1, &s->pixmap);
}

void HudPmx::InitTextures(void)
{
    FUNCTION_MARKER(0x800924DC, "NFS4.EXE");
    char *tachs[29] = {
        "tslk", "tbz3", "thsv", "tfor", "tz28", "ttra", "tdb7", "txkr",
        "ttm5", "tvet", "t550", "t911", "tf50", "tdsv", "tclk", "tmcf",
        "t911", "thsv", "tvet", "tbon", "tbon", "tbon", "tcap", "thsv",
        "ttm5", "tvet", "t911", "tdsv", "tbon"
    };
    char *ntachs[29] = {
        "nslk", "nbz3", "nhsv", "nfor", "nz28", "ntra", "ndb7", "nxkr",
        "nnm5", "nvet", "n550", "n911", "nf50", "tdsv", "tclk", "nmcf",
        "n911", "nhsv", "nvet", "nbon", "nbon", "nbon", "ncap", "nhsv",
        "nnm5", "nvet", "n911", "tdsv", "nbon"
    };
    static char defaultTach[] = "tbon";
    static char defaultNightTach[] = "nbon";
    char mapname[5];
    sprintf(mapname, "mp%02d", GameSetup::gData.track);
    char *shapes[132] = {
        "cmra", "cam0", "cam1", "cam2", "cam3", "cam4", "cam5", "cam6",
        "cam7", "cam8", "cam9", "ca10", "ca11", "ca12", "crn1", "crn2",
        "crn3", "crn4", "arrl", "arrr", "mwt1", "mwt2", "mwb1", "mwb2",
        "msid", "msi2", "kmhh", "mphh", "gea1", "gea2", "gea3", "gea4",
        "gea5", "gea6", "smln", "smlr", "pau0", "pau1", "pau2", "pau3",
        "rwid", "rwi2", "rsid", "rsi2", "big0", "big1", "big2", "big3",
        "big4", "big5", "big6", "big7", "big8", "big9", "sml1", "sml2",
        "sml3", "sml4", "sml5", "sml6", "321d", "321l", "mpbt", "smlx",
        "sml+", "sml-", "sml:", "sml\"", "sml'", "sml,", "sml.", "sml/",
        "ahyp", "acol", "a192", "a193", "a194", "wrng", "a196", "a197",
        "wrng", "a199", "a200", "a201", "wrng", "a203", "a204", "a205",
        "a206", "a207", "wrng", "a209", "a210", "a211", "a212", "wrng",
        "a214", "wrng", "wrng", "a217", "a218", "a219", "a220", "A229",
        "wrng", "time", "timb", "laps", "ejct", "play", "paus", "resg",
        "rad0", "rad1", "s1/4", "s1/2", "spd2", "spd1", "plus", "minu",
        mapname, "plr1", "oppn", "barl", "barr", "barb", "baXl", "baXr",
        "tacc", "tslk", "tpno", "tslk"
    };
    char name[256];
    loadShapeXOff = 0;
    sprintf(name, "%sfont.pfn", Paths_Paths[26]);
    gHudFont = static_cast<char *>(loadfileadrz(name, 0));
    Font::LoadFont(gHudFont, 128, 128, 1);
    for (sint32 i = 0; i < 10; ++i)
    {
        sint32 u, v, w, h, yo;
        Font::GetUVWH((char)(i + '0'), &u, &v, &w, &h, &yo);
        gHudNumberUV[i].u0 = (uint8)u;
        gHudNumberUV[i].v0 = (uint8)v;
        gHudNumberUV[i].clut = gFontClut;
    }
    sprintf(name, "%shud.psh", Paths_Paths[26]);
    gShpfile = static_cast<char *>(loadfileadr(name, 0));
    Texture::ResetPaletteSharing();
    sint32 carType0 = GameSetup::gData.carInfo[0].carType;
    sint32 time = GameSetup::gData.Time;
    sint32 carType1 = GameSetup::gData.carInfo[1].carType;
    shapes[129] = time == 0 ? (carType0 < 30 ? tachs[carType0] : defaultTach) :
        (carType0 < 30 ? ntachs[carType0] : defaultNightTach);
    for (sint32 i = 0; i < 131; ++i)
        LoadShape(shapes[i], &gShapes[i]);
    {
        static char alph[5] = "alpX";
        for (sint32 i = 132; i < 158; ++i)
        {
            alph[3] = (char)(i - 67);
            LoadShape(alph, &gShapes[i]);
        }
    }
    {
        static char alph[5] = "alpX";
        for (sint32 i = 158; i < 168; ++i)
        {
            alph[3] = (char)(i - 110);
            LoadShape(alph, &gShapes[i]);
        }
    }
    LoadShape("alTR", &gShapes[168]);
    LoadShape("alCI", &gShapes[169]);
    LoadShape("alSQ", &gShapes[170]);
    LoadShape("negA", &gShapes[171]);
    LoadShape("negB", &gShapes[172]);
    LoadShape("neg2", &gShapes[173]);
    LoadShape("alUP", &gShapes[174]);
    for (sint32 i = 74; i < 102; ++i)
        LoadShape(shapes[i], &gShapes[i]);
    LoadShape("a229", &gShapes[103]);
    LoadShape("ahyp", &gShapes[72]);
    LoadShape("acol", &gShapes[73]);
    if (DashHUD::gInfo.splitscreen != 0)
    {
        shapes[131] = time == 0 ? (carType1 < 30 ? tachs[carType1] : defaultTach) :
            (carType1 < 30 ? ntachs[carType1] : defaultNightTach);
        loadShapeXOff = 22;
        LoadShape(shapes[131], &gShapes[131]);
    }
    purgememadr(gShpfile);
}

void HudPmx::ReleaseFont(void)
{
    purgememadr(gHudFont);
}
