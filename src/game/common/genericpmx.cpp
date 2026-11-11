#include <stddef.h>
#include <stdio.h>
#include "xport.h"
#include "eaclib/psx/eaclib.h"
#include "game/common/genericpmx.h"
#include "game/common/gmesetup.h"
#include "game/common/paths.h"
#include "game/psx/draw.h"
#include "game/psx/drawc.h"
#include "game/psx/texture.h"
#include "game/psx/trackspec.h"

Draw::tPixMap *gDPixmap;
Draw::tPixMap *gSpikeBeltPixmap;
Draw::tPixMap *gSMokePalette;
Draw::tPixMap *gDirtPalette;
Draw::tPixMap *gGravelPalette;
Draw::tPixMap *gGrassPalette;
Draw::tPixMap *gSnowPalette;
Draw::tPixMap *gLeafPixmap;
Draw::tPixMap *gDamagePixmap;
Draw::tPixMap *gDLPixmap[10];
Draw::tPixMap *gSparkHPixmap[6];
Draw::tPixMap *gStartUpPixmap[4];
Draw::tPixMap *gWeatherPixmap[3];
Draw::tPixMap *gFlarePixmap[3];
Draw::tPixMap *gLightningPixmap[16];
Draw::tPixMap *gShadowPixmap[2];
Draw::tPixMap *gSkidMarkPixmap[2];
Draw::tPixMap *gSMokePixmap[2];
Draw::tPixMap *gGravelPixmap[2];
Draw::tPixMap gPixmaps[71];

void GenericPMX::LoadTexture(void)
{
    FUNCTION_MARKER(0x8009B458, "NFS4.EXE");
    char name[255];
    const char *format = "%sSfx.psh";
    if ((GameSetup::gData.track & 15) == 4)
        format = GameSetup::gData.Weather == 0 ? "%sSfx4.psh" : "%sSfx4w.psh";
    sprintf(name, format, Paths_Paths[25]);
    char *shpfile = static_cast<char *>(loadfileadr(name, 0));
    Texture::ResetPaletteSharing();
    sint32 np = 0;
    sint32 i;
    Draw::tPixMap *pmx;
    for (i = 0; i < 10; ++i)
    {
        sprintf(name, "LIN%d", i);
        pmx = &gPixmaps[np++];
        Texture::LoadPmx(shpfile, name, 0x40, 0, 160, -1, -1, pmx);
        gDLPixmap[i] = pmx;
    }
    pmx = &gPixmaps[np++];
    Texture::LoadPmx(shpfile, "spik", 0x40, 0, 160, -1, -1, pmx);
    gSpikeBeltPixmap = pmx;
    pmx = &gPixmaps[np++];
    Texture::LoadPmx(shpfile, "DEBG", 0x40, 0, 160, -1, -1, pmx);
    gDPixmap = pmx;
    pmx = &gPixmaps[np++];
    Texture::LoadPmx(shpfile, "SHAD", 0x40, 0, 160, -1, -1, pmx);
    gShadowPixmap[0] = pmx;
    pmx = &gPixmaps[np++];
    Texture::LoadPmx(shpfile, "SKD0", 0x40, 0, 160, -1, -1, pmx);
    gSkidMarkPixmap[0] = pmx;
    pmx = &gPixmaps[np++];
    Texture::LoadPmx(shpfile, "SKD1", 0x40, 0, 160, -1, -1, pmx);
    gSkidMarkPixmap[1] = pmx;
    ChangeTPage(&gSkidMarkPixmap[0]->tpage, 2);
    ChangeTPage(&gSkidMarkPixmap[1]->tpage, 2);
    if (GameSetup::gData.Weather != 0)
    {
        pmx = &gPixmaps[np++];
        Texture::LoadPmx(shpfile, "FLAK", 0x40, 0, 160, -1, -1, pmx);
        gWeatherPixmap[0] = pmx;
        pmx = &gPixmaps[np++];
        Texture::LoadPmx(shpfile, "FLA1", 0x40, 0, 160, -1, -1, pmx);
        gWeatherPixmap[1] = pmx;
        pmx = &gPixmaps[np++];
        Texture::LoadPmx(shpfile, "SPLT", 0x40, 0, 160, -1, -1, pmx);
        gWeatherPixmap[2] = pmx;
        ChangeTPage(&gWeatherPixmap[0]->tpage, 1);
        ChangeTPage(&gWeatherPixmap[1]->tpage, 1);
        ChangeTPage(&gWeatherPixmap[2]->tpage, 1);
    }
    pmx = &gPixmaps[np++];
    Texture::LoadPmx(shpfile, "SMX0", 0x40, 0, 160, -1, -1, pmx);
    gSMokePixmap[0] = pmx;
    pmx = &gPixmaps[np++];
    Texture::LoadPmx(shpfile, "SMX1", 0x40, 0, 160, -1, -1, pmx);
    gSMokePixmap[1] = pmx;
    pmx = &gPixmaps[np++];
    Texture::LoadPmx(shpfile, "SMOK", 0, -1, -1, -1, -1, pmx);
    gSMokePalette = pmx;
    pmx = &gPixmaps[np++];
    Texture::LoadPmx(shpfile, "DIRT", 0, -1, -1, -1, -1, pmx);
    gDirtPalette = pmx;
    pmx = &gPixmaps[np++];
    Texture::LoadPmx(shpfile, "GRX0", 0x40, 0, 160, -1, -1, pmx);
    gGravelPixmap[0] = pmx;
    pmx = &gPixmaps[np++];
    Texture::LoadPmx(shpfile, "GRX1", 0x40, 0, 160, -1, -1, pmx);
    gGravelPixmap[1] = pmx;
    pmx = &gPixmaps[np++];
    Texture::LoadPmx(shpfile, "GRAV", 0, -1, -1, -1, -1, pmx);
    gGravelPalette = pmx;
    pmx = &gPixmaps[np++];
    Texture::LoadPmx(shpfile, "GRAS", 0, -1, -1, -1, -1, pmx);
    gGrassPalette = pmx;
    pmx = &gPixmaps[np++];
    Texture::LoadPmx(shpfile, "SNOW", 0, -1, -1, -1, -1, pmx);
    gSnowPalette = pmx;
    for (i = 0; i < 6; ++i)
    {
        sprintf(name, "SHX%d", i);
        pmx = &gPixmaps[np++];
        Texture::LoadPmx(shpfile, name, 0x40, 0, 160, -1, -1, pmx);
        gSparkHPixmap[i] = pmx;
    }
    if (GameSetup::gData.Time == 0 && GameSetup::gData.commMode != 1)
    {
        char shpname[5];
        sprintf(shpname, "LF%02d", GameSetup::gData.track);
        shapetbl *shape = static_cast<shapetbl *>(locateshape(shpfile, shpname));
        if (shape != 0)
        {
            pmx = &gPixmaps[np++];
            Texture::LoadPmx(0, reinterpret_cast<char *>(shape), 0x40, 0, 160, -1, -1, pmx);
            gLeafPixmap = pmx;
        }
        else
            gLeafPixmap = 0;
    }
    else
        gLeafPixmap = 0;
    pmx = &gPixmaps[np++];
    Texture::LoadPmx(shpfile, "DAMG", 0x40, 0, 160, -1, -1, pmx);
    gDamagePixmap = pmx;
    if (GameSetup::gData.Weather != 0)
    {
        for (i = 0; i < 8; ++i)
        {
            sprintf(name, "LNG%d", i);
            pmx = &gPixmaps[np++];
            Texture::LoadPmx(shpfile, name, 0x40, 0, 160, -1, -1, pmx);
            gLightningPixmap[i] = pmx;
            gLightningPixmap[i + 8] = &gPixmaps[np++];
            ChangeTPage(&gLightningPixmap[i]->tpage, 1);
            Texture::CloneUVPmx(gLightningPixmap[i], 0, gLightningPixmap[i + 8]);
        }
    }
    else
    {
        if ((TrackSpec::gSpec.skyspec.flags & 8) != 0)
        {
            pmx = &gPixmaps[np++];
            Texture::LoadPmx(shpfile, "MONF", 0x40, 0, 160, -1, -1, pmx);
            gFlarePixmap[0] = pmx;
        }
        if ((TrackSpec::gSpec.skyspec.flags & 4) != 0)
        {
            pmx = &gPixmaps[np++];
            Texture::LoadPmx(shpfile, "FLR0", 0x40, 0, 160, -1, -1, pmx);
            gFlarePixmap[0] = pmx;
            pmx = &gPixmaps[np++];
            Texture::LoadPmx(shpfile, "FLR1", 0x40, 0, 160, -1, -1, pmx);
            gFlarePixmap[1] = pmx;
            pmx = &gPixmaps[np++];
            Texture::LoadPmx(shpfile, "RBOW", 0x40, 0, 160, -1, -1, pmx);
            gFlarePixmap[2] = pmx;
            ChangeTPage(&gFlarePixmap[0]->tpage, 1);
            ChangeTPage(&gFlarePixmap[1]->tpage, 1);
            ChangeTPage(&gFlarePixmap[2]->tpage, 1);
        }
    }
    purgememadr(shpfile);
}
