#include <stddef.h>
#include <string.h>
#include "xport.h"
#include "eaclib/psx/eaclib.h"
#include "game/common/car.h"
#include "game/common/carobj.h"
#include "game/common/gmesetup.h"
#include "game/common/r3dcar.h"
#include "game/psx/cario.h"
#include "game/psx/draw.h"
#include "game/psx/drawc.h"
#include "game/psx/texture.h"

struct shapetbl;

sint32 inFrontEnd;
static const char CarIO_emptyTextureName[4] = { 0, 0, 0, 0 };
sint32 CarIO::carPixMapCount;
sint32 CarIO::carVRamCount;
sint32 CarIO::licenseSFX_Count;
Draw::tPixMap *CarIO::carPixMap;
sint16 CarIO::carVRamSlots[18][2] = { { 640, 256 }, { 640, 341 }, { 640, 426 }, { 704, 256 }, { 704, 341 }, { 704, 426 }, { 768, 256 }, { 768, 341 }, { 768, 426 }, { 832, 256 }, { 832, 341 }, { 832, 426 }, { 896, 256 }, { 896, 341 }, { 896, 426 }, { 960, 256 }, { 960, 341 }, { 960, 426 } };
sint16 CarIO::carVRamSlotsMenu[6][2] = { { 512, 0 }, { 576, 0 }, { 640, 0 }, { 704, 0 }, { 640, 0 }, { 704, 0 } };
sint16 CarIO::carVRamAdd[6] = { 2, 1, 2, 1, 2, 1 };
sint16 CarIO::carVRamOffset[6] = { 64, 0, -32, 64, 0, -32 };
sint16 CarIO::licensePlate[22][6] = { { 27, 58, 22, 33, 58, 110 }, { 33, 58, 110, 26, 58, 0 }, { 27, 58, 22, 33, 58, 110 }, { 33, 58, 110, 18, 58, 234 }, { 27, 58, 22, 33, 58, 110 }, { 27, 58, 22, 33, 58, 110 }, { 18, 58, 234, 33, 58, 110 }, { 27, 58, 22, 33, 58, 110 }, { 27, 58, 22, 33, 58, 110 }, { 26, 58, 0, 33, 58, 110 }, { 27, 58, 22, 33, 58, 110 }, { 27, 58, 22, 33, 58, 110 }, { 33, 58, 110, 17, 58, 212 }, { 33, 58, 110, 18, 58, 234 }, { 27, 58, 22, 33, 58, 110 }, { 33, 58, 110, 26, 58, 0 }, { 27, 58, 22, 33, 58, 110 }, { 27, 58, 22, 33, 58, 110 }, { 26, 58, 0, 33, 58, 110 }, { 27, 58, 22, 33, 58, 110 }, { 33, 58, 110, 26, 58, 0 }, { 0, 0, 0, 0, 0, 0 } };
sint16 CarIO::licenseSFX_Vram[12][2] = { { 960, 426 }, { 966, 426 }, { 972, 426 }, { 978, 426 }, { 984, 426 }, { 990, 426 }, { 996, 426 }, { 1002, 426 }, { 1008, 426 }, { 1014, 426 }, { 960, 448 }, { 966, 448 } };
shapetbl *CarIO::Plate1[2];
shapetbl *CarIO::Plate2[2];

CarIO::textureInfo CarIO::textureName[51] =
{
    { "frnt", "Pfrn", 0 },
    { "rear", "Prea", 0 },
    { "side", "Psid", 0 },
    { "top1", "Ptp1", 0 },
    { "top2", "Ptp2", 0 },
    { "dHiF", CarIO_emptyTextureName, 1 },
    { "dHiR", CarIO_emptyTextureName, 2 },
    { "dHiS", CarIO_emptyTextureName, 3 },
    { "dTp1", CarIO_emptyTextureName, 4 },
    { "dTop", CarIO_emptyTextureName, 5 },
    { "dLoF", CarIO_emptyTextureName, 1 },
    { "dLoR", CarIO_emptyTextureName, 2 },
    { "dLoS", CarIO_emptyTextureName, 3 },
    { "wndw", "Pwnd", 0 },
    { "ftdm", "Pftd", 0 },
    { "splr", "Pspl", 0 },
    { "ext2", "Pex2", 0 },
    { "ext3", "Pex3", 0 },
    { "ext4", "Pex4", 0 },
    { "whl ", "Pwhl", 0 },
    { "ext1", CarIO_emptyTextureName, 0 },
    { "botm", CarIO_emptyTextureName, 0 },
    { "whlI", CarIO_emptyTextureName, 0 },
    { "tred", CarIO_emptyTextureName, 0 },
    { "bROf", CarIO_emptyTextureName, 0 },
    { "bSOf", CarIO_emptyTextureName, 0 },
    { "rvOf", CarIO_emptyTextureName, 0 },
    { "sFOf", CarIO_emptyTextureName, 0 },
    { "sROf", CarIO_emptyTextureName, 0 },
    { "ltOf", CarIO_emptyTextureName, 0 },
    { "bROn", CarIO_emptyTextureName, 0 },
    { "bSOn", CarIO_emptyTextureName, 0 },
    { "shdw", CarIO_emptyTextureName, 0 },
    { "sFOn", CarIO_emptyTextureName, 0 },
    { "sROn", CarIO_emptyTextureName, 0 },
    { "ltOn", CarIO_emptyTextureName, 0 },
    { "dcl1", CarIO_emptyTextureName, 0 },
    { "dcl2", CarIO_emptyTextureName, 0 },
    { "dcl3", CarIO_emptyTextureName, 0 },
    { "dcl4", CarIO_emptyTextureName, 0 },
    { "dcl5", CarIO_emptyTextureName, 0 },
    { "dcl6", CarIO_emptyTextureName, 0 },
    { "ins0", CarIO_emptyTextureName, 0 },
    { "ins1", CarIO_emptyTextureName, 0 },
    { "ins2", CarIO_emptyTextureName, 0 },
    { "ins3", CarIO_emptyTextureName, 0 },
    { "ins4", CarIO_emptyTextureName, 0 },
    { "ins5", CarIO_emptyTextureName, 0 },
    { "ins6", CarIO_emptyTextureName, 0 },
    { "ins7", CarIO_emptyTextureName, 0 },
    { "ins8", CarIO_emptyTextureName, 0 },
};

void CarIO::StartUp(void)
{
    FUNCTION_MARKER(0x800BBF10, "NFS4.EXE");
    if (carPixMap == 0)
        carPixMap = (Draw::tPixMap *)reservememadr("carPixMap", 0x2640, 0);
    for (sint32 i = 611; i >= 0; --i)
        carPixMap[i].flag = 0;
    carPixMapCount = 0;
    carVRamCount = 0;
    licenseSFX_Count = 0;
    Plate2[0] = 0;
    Plate1[0] = 0;
    Plate2[1] = 0;
    Plate1[1] = 0;
}

void CarIO::CleanUp(void)
{
    FUNCTION_MARKER(0x800BBF7C, "NFS4.EXE");
    if (carPixMap != 0)
        purgememadr(carPixMap);
    carPixMap = 0;
}

void CarIO::ReStart(void)
{
    FUNCTION_MARKER(0x800BBFA4, "NFS4.EXE");
    for (sint32 i = 611; i >= 0; --i)
        carPixMap[i].flag = 0;
    carPixMapCount = 0;
    carVRamCount = 0;
    CleanUpLicense(0);
    CleanUpLicense(1);
}

void CarIO::CopyFromShape(sint16 *source, sint16 *dest, sint32 w, sint32 h, sint32 x, sint32 y)
{
    FUNCTION_MARKER(0x800BBFF0, "NFS4.EXE");
    sint32 columns = ((uint32)(w >> 2) + ((w & 3) != 0));
    sint32 mask = w & 3;
    uint32 lastMask = mask != 0 ? 0xFFFFU : 0;
    uint32 lastLastMask = 0xFFFF;
    uint32 firstMask = 0;
    for (sint32 i = 0; i < mask; ++i)
        lastMask <<= 4;
    mask = x & 3;
    for (sint32 i = 0; i < mask; ++i)
    {
        uint16 rollOver = (uint16)((lastMask & 0xF000) >> 12);
        firstMask = (firstMask << 4) | 0xF;
        lastMask <<= 4;
        lastLastMask = (lastLastMask << 4) | rollOver;
    }
    if ((lastLastMask & 0xFFFFU) != 0xFFFFU)
    {
        lastMask = lastLastMask;
        columns = (columns + 1);
    }
    sint32 destOffset = (((x >> 2)) + (y * 12));
    dest += destOffset;
    uint32 rows = h;
    while (--rows != 0xFFFFFFFF)
    {
        uint32 current = (uint16)*source++;
        uint32 next = 0;
        for (sint32 shift = 0; shift < mask; ++shift)
        {
            uint16 rollOver = (uint16)((current & 0xF000) >> 12);
            current <<= 4;
            next = (next << 4) | rollOver;
        }
        dest[0] = (sint16)(((uint16)dest[0] & firstMask) | current);
        sint32 i = 1;
        sint32 lastColumn = (columns - 1);
        for (; i < lastColumn; ++i)
        {
            dest[i] = (sint16)next;
            current = (uint16)*source++;
            next = 0;
            for (sint32 shift = 0; shift < mask; ++shift)
            {
                uint16 rollOver = (uint16)((current & 0xF000) >> 12);
                current <<= 4;
                next = (next << 4) | rollOver;
            }
            dest[i] = (sint16)((uint16)dest[i] | current);
        }
        dest[i] = (sint16)(((uint16)dest[i] & lastMask) | next);
        if ((lastLastMask & 0xFFFFU) == 0xFFFFU)
        {
            current = (uint16)*source++;
            for (sint32 shift = 0; shift < mask; ++shift)
                current <<= 4;
            dest[i] = (sint16)((uint16)dest[i] | current);
        }
        dest += 12;
    }
}

void CarIO::CopyToShape(sint16 *source, sint16 *dest, sint32 mirror)
{
    FUNCTION_MARKER(0x800BC1B4, "NFS4.EXE");
    for (sint32 h = 21; h >= 0; --h)
    {
        if (mirror == 0)
        {
            for (sint32 i = 0; i < 6; ++i) *dest++ = source[i];
        }
        else
        {
            for (sint32 i = 5; i >= 0; --i)
            {
                uint16 pixel3 = source[i];
                *dest++ = (sint16)(((pixel3 & 0xFU) << 12) | ((pixel3 & 0xF0U) << 4) | ((pixel3 & 0xF00U) >> 4) | (pixel3 >> 12));
            }
        }
        source += 12;
    }
}

void CarIO::CreateLicense(char *text, sint32 carType, sint32 player)
{
    FUNCTION_MARKER(0x800BC25C, "NFS4.EXE");
    if (carType >= 22)
    {
        Plate2[player] = 0;
        Plate1[player] = 0;
        return;
    }
    Plate1[player] = (shapetbl *)reservememadr("plate1", 0x148, 0);
    Plate2[player] = (shapetbl *)reservememadr("plate2", 0x148, 0);
    shapetbl *clutPlate2 = (shapetbl *)((uint8 *)Plate2[player] + 0x118);
    shapetbl *clutPlate1 = (shapetbl *)((uint8 *)Plate1[player] + 0x118);
    sint16 *thePlate = (sint16 *)reservememadr("theplate", 0x210, 0x10);
    shapetbl *shape = (shapetbl *)locateshape(R3DCar::LicenseShapeFile, "blnk");
    shapetbl *clutptr = (shapetbl *)((uint8 *)shape + shape->next);
    for (sint32 i = 0; i < 4; ++i)
    {
        uint32 word;
        memcpy(&word, (uint8 *)shape + i * 4, 4);
        shapetbl *plate2 = Plate2[player];
        shapetbl *plate1 = Plate1[player];
        memcpy((uint8 *)plate2 + i * 4, &word, 4);
        memcpy((uint8 *)plate1 + i * 4, &word, 4);
    }
    for (sint32 i = 0; i < 12; ++i)
    {
        uint32 word;
        memcpy(&word, (uint8 *)clutptr + i * 4, 4);
        memcpy((uint8 *)clutPlate2 + i * 4, &word, 4);
        memcpy((uint8 *)clutPlate1 + i * 4, &word, 4);
    }
    Plate2[player]->next = 0x118;
    Plate1[player]->next = 0x118;
    Plate2[player]->width = 24;
    Plate1[player]->width = 24;
    CopyFromShape((sint16 *)((uint8 *)shape + 16), thePlate, 48, 22, 0, 0);
    sint32 length = ((uint32)strlen(text));
    sint32 start = (24 - (length * 3));
    for (sint32 i = 0; i < length; ++i)
    {
        uint8 ascii = (uint8)text[i];
        if (ascii != ' ')
        {
            switch (ascii)
            {
            case 0xC0: case 0xC4: case 0xC5: ascii = 'a'; break;
            case 0xC8: ascii = 'e'; break;
            case 0xCC: ascii = 'i'; break;
            case 0xD1: ascii = 'n'; break;
            case 0xD2: case 0xD6: ascii = 'o'; break;
            case 0xD9: case 0xDC: ascii = 'u'; break;
            }
            char letter[5];
            letter[0] = (char)ascii;
            letter[1] = 0;
            strcat(letter, "   ");
            shapetbl *glyph = (shapetbl *)locateshape(R3DCar::LicenseShapeFile, letter);
            CopyFromShape((sint16 *)((uint8 *)glyph + 16), thePlate, 7, 12, start, 5);
        }
        start = (start + 6);
    }
    if (R3DCar::InMenu == 0 && GameSetup::gData.mirrorTrack != 0)
    {
        CopyToShape(thePlate + 6, (sint16 *)((uint8 *)Plate1[player] + 16), 1);
        CopyToShape(thePlate, (sint16 *)((uint8 *)Plate2[player] + 16), 1);
    }
    else
    {
        CopyToShape(thePlate, (sint16 *)((uint8 *)Plate1[player] + 16), 0);
        CopyToShape(thePlate + 6, (sint16 *)((uint8 *)Plate2[player] + 16), 0);
    }
    purgememadr(thePlate);
}

void CarIO::CleanUpLicense(sint32 player)
{
    FUNCTION_MARKER(0x800BC5F0, "NFS4.EXE");
    if (Plate1[player] != 0)
        purgememadr(Plate1[player]);
    Plate1[player] = 0;
    if (Plate2[player] != 0)
        purgememadr(Plate2[player]);
    Plate2[player] = 0;
}

void CarIO::LicenseCheck(sint32 reload, sint32 *license_vx, sint32 *license_vy, Car::tObj *carObj, sint32 plate)
{
    FUNCTION_MARKER(0x800BC668, "NFS4.EXE");
    if ((reload & 2) != 0 && licenseSFX_Count < 12)
    {
        sint32 sfx_vx = licenseSFX_Vram[licenseSFX_Count][0];
        sint32 sfx_vy = licenseSFX_Vram[licenseSFX_Count][1];
        carObj->render.licenseOffsetU[plate] = (uint8)(((sfx_vx & 63) - (*license_vx & 63)) * 4);
        carObj->render.licenseOffsetV[plate] = (uint8)(sfx_vy - ((*license_vy) & 0xFFU));
        *license_vx = sfx_vx;
        *license_vy = sfx_vy;
        licenseSFX_Count = (licenseSFX_Count + 1);
    }
    else
    {
        carObj->render.licenseOffsetV[plate] = 0;
        carObj->render.licenseOffsetU[plate] = 0;
    }
}

void CarIO::ReadInCarTextureData(char *shpfile, Car::tObj *carObj, sint32 reload, sint32 player)
{
    FUNCTION_MARKER(0x800BC704, "NFS4.EXE");
    sint32 carType = carObj->render.currentCarType;
    sint32 vx;
    sint32 vy;
    if ((reload & 1) == 0)
    {
        sint32 slot = carVRamCount;
        if (R3DCar::InMenu == 0)
        {
            carObj->render.VRamX = carVRamSlots[slot][0];
            vx = carVRamSlots[slot][0];
            vy = carVRamSlots[slot][1];
            carObj->render.VRamY = (sint16)vy;
            if (carType < 28)
            {
                if ((carObj->render.inside & 1) != 0)
                    carVRamCount = (slot + (carVRamAdd[slot / 3] * 3));
                else
                    carVRamCount = (slot + 3);
            }
            else
                carVRamCount = (slot + 1);
        }
        else
        {
            carObj->render.VRamX = carVRamSlotsMenu[slot][0];
            vx = carVRamSlotsMenu[slot][0];
            vy = carVRamSlotsMenu[slot][1];
            carObj->render.VRamY = (sint16)vy;
            carVRamCount = (slot + carVRamAdd[slot]);
        }
    }
    else
    {
        vx = carObj->render.VRamX;
        vy = carObj->render.VRamY;
    }
    sint32 carPixMapCount;
    if ((reload & 0x10) == 0)
    {
        carPixMapCount = CarIO::carPixMapCount;
        carObj->render.textureStartIndex = carPixMapCount;
    }
    else
        carPixMapCount = carObj->render.textureStartIndex;
    if ((reload & 8) != 0)
    {
        if ((carObj->render.inside & 1) != 0)
        {
            sint32 index = vx - (R3DCar::InMenu != 0 ? 512 : 640);
            vx += carVRamOffset[index >> 6];
        }
        carObj->render.textureOffsetU = (sint16)((vx & 63) << 2);
        carObj->render.textureOffsetV = (sint16)(vy & 255);
    }
    sint32 recolor_flag = R3DCar::InMenu != 0 ? 8 : 0x18;
    Texture::palCopy = (Texture::pal8bit *)carObj->render.palCopy;
    Texture::ResetPaletteSharing();
    for (sint32 i = 0; i < 51; ++i)
    {
        bool palette = true;
        shapetbl *shape = (shapetbl *)locateshape(shpfile, textureName[i].pal);
        sint32 palShare = textureName[i].palShare;
        if (shape == 0 && palShare == 0)
        {
            palette = false;
            shape = (shapetbl *)locateshape(shpfile, textureName[i].tex);
        }
        if (i == 20)
            recolor_flag = 0;
        if (shape != 0)
        {
            if (recolor_flag != 0)
                carObj->render.palCopyNum[i] = (sint16)Texture::palNum;
            bool license = false;
            if (carType < 22)
            {
                if (i == licensePlate[carType][0])
                {
                    sint32 license_vx = vx + licensePlate[carType][1];
                    sint32 license_vy = vy + licensePlate[carType][2];
                    LicenseCheck(reload, &license_vx, &license_vy, carObj, 0);
                    Texture::LoadPmx(0, (char *)Plate1[player], recolor_flag, license_vx, license_vy, -1, -1, &CarIO::carPixMap[carPixMapCount]);
                    license = true;
                    CarIO::carPixMap[carPixMapCount].flag = 1;
                }
                else if (i == licensePlate[carType][3])
                {
                    sint32 license_vx = vx + licensePlate[carType][4];
                    sint32 license_vy = vy + licensePlate[carType][5];
                    LicenseCheck(reload, &license_vx, &license_vy, carObj, 1);
                    Texture::LoadPmx(0, (char *)Plate2[player], recolor_flag, license_vx, license_vy, -1, -1, &CarIO::carPixMap[carPixMapCount]);
                    license = true;
                    CarIO::carPixMap[carPixMapCount].flag = 2;
                }
            }
            if (!license)
                Texture::LoadPmx(0, (char *)shape, recolor_flag, vx, vy, -1, -1, &CarIO::carPixMap[carPixMapCount]);
            if (i == 32)
                ChangeTPage(&CarIO::carPixMap[carPixMapCount].tpage, 2);
            CarIO::carPixMap[carPixMapCount].flag |= 0x80;
        }
        if (palette)
        {
            sint32 palIndex = carPixMapCount;
            if (palShare != 0)
            {
                sint32 sharedIndex = (palShare - 1);
                if (recolor_flag != 0)
                    carObj->render.palCopyNum[i] = carObj->render.palCopyNum[sharedIndex];
                palIndex = (sharedIndex + carObj->render.textureStartIndex);
            }
            shape = (shapetbl *)locateshape(shpfile, textureName[i].tex);
            if (shape != 0)
            {
                uint16 clut = CarIO::carPixMap[palIndex].clut;
                sint32 cx = (clut & 63) << 4;
                sint32 cy = clut >> 6;
                bool license = false;
                if (carType < 22)
                {
                    if (i == licensePlate[carType][0])
                    {
                        sint32 license_vx = vx + licensePlate[carType][1];
                        sint32 license_vy = vy + licensePlate[carType][2];
                        LicenseCheck(reload, &license_vx, &license_vy, carObj, 0);
                        Texture::LoadPmx(0, (char *)Plate1[player], 0x20, license_vx, license_vy, cx, cy, &CarIO::carPixMap[carPixMapCount]);
                        license = true;
                        CarIO::carPixMap[carPixMapCount].flag = 1;
                    }
                    else if (i == licensePlate[carType][3])
                    {
                        sint32 license_vx = vx + licensePlate[carType][4];
                        sint32 license_vy = vy + licensePlate[carType][5];
                        LicenseCheck(reload, &license_vx, &license_vy, carObj, 1);
                        Texture::LoadPmx(0, (char *)Plate2[player], 0x20, license_vx, license_vy, cx, cy, &CarIO::carPixMap[carPixMapCount]);
                        license = true;
                        CarIO::carPixMap[carPixMapCount].flag = 2;
                    }
                }
                if (!license)
                    Texture::LoadPmx(0, (char *)shape, 0x20, vx, vy, cx, cy, &CarIO::carPixMap[carPixMapCount]);
                if (i == 32)
                    ChangeTPage(&CarIO::carPixMap[carPixMapCount].tpage, 2);
                if (palShare == 0)
                    CarIO::carPixMap[carPixMapCount].flag |= 0x80;
            }
        }
        carPixMapCount = (carPixMapCount + 1);
    }
    if ((reload & 0x80) != 0)
        CarIO::carPixMapCount = carPixMapCount;
}

void CarIO::UpdateCarTextureData(char *shpfile, Car::tObj *carObj, sint32 player)
{
    FUNCTION_MARKER(0x800BCEB0, "NFS4.EXE");
    sint32 carPixMapCount = carObj->render.textureStartIndex;
    sint32 carType = carObj->render.currentCarType;
    sint32 vx = carObj->render.VRamX;
    sint32 vy = carObj->render.VRamY;
    sint32 recolor_flag = R3DCar::InMenu != 0 ? 8 : 0x18;
    Texture::palCopy = (Texture::pal8bit *)carObj->render.palCopy;
    Texture::ResetPaletteSharing();
    for (sint32 i = 0; i < 51; ++i)
    {
        bool palette = true;
        shapetbl *shape = (shapetbl *)locateshape(shpfile, textureName[i].pal);
        sint32 palShare = textureName[i].palShare;
        if (shape == 0 && palShare == 0)
        {
            palette = false;
            shape = (shapetbl *)locateshape(shpfile, textureName[i].tex);
        }
        if (i == 20)
            recolor_flag = 0;
        if (shape != 0)
        {
            Draw::tPixMap *pmx = &CarIO::carPixMap[carPixMapCount];
            if ((pmx->flag & 0x80) == 0)
            {
                carPixMapCount = (carPixMapCount + 1);
                continue;
            }
            sint32 cx = (pmx->clut & 63) << 4;
            sint32 cy = pmx->clut >> 6;
            if (recolor_flag != 0)
                Texture::palNum = carObj->render.palCopyNum[i];
            bool license = false;
            if (carType < 22)
            {
                if (i == licensePlate[carType][0])
                {
                    license = true;
                    Texture::LoadPmx(0, (char *)Plate1[player], recolor_flag, vx + licensePlate[carType][1], vy + licensePlate[carType][2], cx, cy, pmx);
                }
                else if (i == licensePlate[carType][3])
                {
                    license = true;
                    Texture::LoadPmx(0, (char *)Plate2[player], recolor_flag, vx + licensePlate[carType][4], vy + licensePlate[carType][5], cx, cy, pmx);
                }
            }
            if (!license)
                Texture::LoadPmx(0, (char *)shape, recolor_flag, vx, vy, cx, cy, &CarIO::carPixMap[carPixMapCount]);
            if (i == 32)
                ChangeTPage(&CarIO::carPixMap[carPixMapCount].tpage, 2);
            CarIO::carPixMap[carPixMapCount].flag |= 0x80;
        }
        if (palette)
        {
            sint32 palIndex = carPixMapCount;
            if (palShare != 0)
            {
                sint32 sharedIndex = (palShare - 1);
                if (recolor_flag != 0)
                    carObj->render.palCopyNum[i] = carObj->render.palCopyNum[sharedIndex];
                palIndex = (sharedIndex + carObj->render.textureStartIndex);
            }
            shape = (shapetbl *)locateshape(shpfile, textureName[i].tex);
            if (shape != 0)
            {
                uint16 clut = CarIO::carPixMap[palIndex].clut;
                sint32 cx = (clut & 63) << 4;
                sint32 cy = clut >> 6;
                bool license = false;
                if (carType < 22)
                {
                    if (i == licensePlate[carType][0])
                    {
                        license = true;
                        Texture::LoadPmx(0, (char *)Plate1[player], 0x20, vx + licensePlate[carType][1], vy + licensePlate[carType][2], cx, cy, &CarIO::carPixMap[carPixMapCount]);
                    }
                    else if (i == licensePlate[carType][3])
                    {
                        license = true;
                        Texture::LoadPmx(0, (char *)Plate2[player], 0x20, vx + licensePlate[carType][4], vy + licensePlate[carType][5], cx, cy, &CarIO::carPixMap[carPixMapCount]);
                    }
                }
                if (!license)
                    Texture::LoadPmx(0, (char *)shape, 0x20, vx, vy, cx, cy, &CarIO::carPixMap[carPixMapCount]);
                if (i == 32)
                    ChangeTPage(&CarIO::carPixMap[carPixMapCount].tpage, 2);
                if (palShare == 0)
                    CarIO::carPixMap[carPixMapCount].flag |= 0x80;
            }
        }
        carPixMapCount = (carPixMapCount + 1);
    }
}

void CarIO::ReleaseCarCluts(Car::tObj *carObj)
{
    FUNCTION_MARKER(0x800BD358, "NFS4.EXE");
    sint32 carPixMapCount = carObj->render.textureStartIndex;
    for (sint32 i = 0; i < 51; ++i)
    {
        Draw::tPixMap *pixMap = &CarIO::carPixMap[carPixMapCount];
        if ((pixMap->flag & 0x80) != 0)
        {
            sint16 clut = pixMap->clut;
            pixMap->flag = 0;
            Texture::MenuReleaseClutId(clut);
        }
        carPixMapCount = (carPixMapCount + 1);
    }
}
