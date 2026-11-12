#include <stddef.h>
#include <string.h>
#include "xport.h"
#include "psx.h"
#include "frontend/common/fetv.h"
#include "frontend/psx/fetexture.h"
#include "frontend/psx/mmeffect.h"
#include "frontend/psx/psxfront.h"
#include "game/psx/drawc.h"

void DrawTVLines(tTVConfig &tv)
{
    FUNCTION_MARKER(0x80021FF8, "FRONT.BIN");
    sint32 span = tv.h * 48;
    if (span == 0)
        abort();
    sint32 wide = (tv.fxWide + 1) % span;
    sint32 thin = (tv.fxThin + 2) % span;
    tv.fxWide = (sint16)wide;
    tv.fxThin = (sint16)thin;
    sint16 y = (sint16)wide;
    sint16 fxHeight = 8;
    if (tv.fxWide < tv.y)
    {
        fxHeight = (sint16)(8 - ((uint16)tv.y - wide));
        y = tv.y;
    }
    else if (tv.fxWide + 8 > tv.y + tv.h)
        fxHeight = (sint16)((uint16)tv.y - wide);
    Draw::PrimStruct *cprim = &DrawC::carCache.head.cprim;
    if (fxHeight > 0)
    {
        POLY_F4 *videoFX = reinterpret_cast<POLY_F4 *>(cprim->PrimPtr);
        uint32 *ot = cprim->LastPrim;
        setaddr(videoFX, getaddr(ot));
        cprim->PrimPtr = reinterpret_cast<char *>(videoFX + 1);
        setaddr(ot, videoFX);
        setcode(videoFX, 0x2A);
        setlen(videoFX, 5);
        videoFX->p[0].x = tv.x;
        videoFX->p[0].y = y;
        videoFX->p[1].x = (sint16)((uint16)tv.x + (uint16)tv.w);
        videoFX->p[1].y = y;
        videoFX->p[2].x = tv.x;
        videoFX->p[2].y = (sint16)((uint16)y + (uint16)fxHeight);
        videoFX->p[3].x = (sint16)((uint16)tv.x + (uint16)tv.w);
        videoFX->p[3].y = (sint16)((uint16)y + (uint16)fxHeight);
        videoFX->rgbc.b = 10;
        videoFX->rgbc.g = 10;
        videoFX->rgbc.r = 10;
    }
    y = tv.fxThin;
    if (tv.y < tv.fxThin && tv.fxThin < tv.y + tv.h)
    {
        POLY_F4 *videoFX = reinterpret_cast<POLY_F4 *>(cprim->PrimPtr);
        uint32 *ot = cprim->LastPrim;
        setaddr(videoFX, getaddr(ot));
        cprim->PrimPtr = reinterpret_cast<char *>(videoFX + 1);
        setaddr(ot, videoFX);
        setcode(videoFX, 0x2A);
        setlen(videoFX, 5);
        videoFX->p[0].x = tv.x;
        videoFX->p[0].y = y;
        videoFX->p[1].x = (sint16)((uint16)tv.x + (uint16)tv.w);
        videoFX->p[1].y = y;
        videoFX->p[2].x = tv.x;
        videoFX->p[2].y = (sint16)((uint16)y + 1);
        videoFX->p[3].x = (sint16)((uint16)tv.x + (uint16)tv.w);
        videoFX->p[3].y = (sint16)((uint16)y + 1);
        videoFX->rgbc.b = 10;
        videoFX->rgbc.g = 10;
        videoFX->rgbc.r = 10;
    }
    FeDraw_SetABRMode(1);
    if ((tv.flags & 0x20) != 0)
    {
        sint16 x = (sint16)(((uint16)tv.x + 4) & ~3u);
        for (; x < tv.x + tv.w; x = (sint16)((uint16)x + 4))
            PSXDrawTransSquare(0xA0A0A, x, tv.y, 1, tv.h, 1);
        FeDraw_SetABRMode(2);
    }
}

static uint8 TVNoiseU(tTexture::ShapeInfo *noise)
{
    sint32 offset = (noise->shapex - (sint16)((uint16)noise->shapex & 0xFFC0)) * 16;
    uint8 depth = (uint8)noise->depth;
    if (depth == 0)
        abort();
    return (uint8)(offset / depth);
}

static sint16 TVReflectionFade(uint16 axis, uint32 position)
{
    sint32 distance = (sint16)(uint32(axis) - position + 1u);
    if (distance < 0)
        distance = -distance;
    sint16 fade = (sint16)(distance * 2);
    if (fade > 128)
        fade = 128;
    return fade;
}

static uint32 TVScaledTint(uint32 tint, sint32 scale)
{
    uint32 blue = (((tint >> 16) & 255u) * uint32(scale)) >> 7;
    uint32 green = (((tint >> 8) & 255u) * uint32(scale)) >> 7;
    uint32 red = ((tint & 255u) * uint32(scale)) >> 7;
    return (blue << 16) | (green << 8) | red;
}

void DrawTV(tTVConfig &tv)
{
    FUNCTION_MARKER(0x8002234C, "FRONT.BIN");
    sint32 noiseIndex = randPSX() & 3;
    sint16 bright = tv.destBrightness;
    uint32 tint = uint32(tv.tint);
    sint16 videoX = tv.x;
    sint16 videoY = tv.y;
    sint16 videoWidth = tv.w;
    sint16 videoHeight = tv.h;
    tTexture::ShapeInfo *noise = &gHelpShapes[34 + noiseIndex];
    bool do_tint = (tv.flags & 2) != 0;
    if (!do_tint)
        tint = 0x808080;
    if (sint32(tv.transition) != sint32(tv.destBrightness))
    {
        if (sint32(tv.transition) < sint32(tv.destBrightness) && uint32(tv.state) - 1u < 2u)
        {
            tv.transition = (sint16)((uint16)tv.transition + 4);
            if (sint32(tv.destBrightness) < sint32(tv.transition))
                tv.transition = tv.destBrightness;
        }
        else if (tv.state == tv_TransitionOff || tv.state == tv_StateOn)
        {
            tv.transition = (sint16)((uint16)tv.transition - 8);
            if (sint32(tv.transition) < sint32(tv.destBrightness))
                tv.transition = tv.destBrightness;
        }
    }
    if (tv.state == tv_StateOn || tv.state == tv_StateOff)
        bright = tv.destBrightness;
    else if (tv.state == tv_TransitionOn)
    {
        if (sint32(tv.transition) == sint32(tv.destBrightness))
            tv.state = tv_StateOn;
        do_tint = true;
        bright = tv.transition;
    }
    else if (tv.state == tv_TransitionOff)
    {
        if (sint32(tv.transition) == sint32(tv.destBrightness))
            tv.state = tv_StateOff;
        do_tint = true;
        bright = tv.transition;
    }
    tint = TVScaledTint(tint, bright);
    if ((tv.flags & 8) == 0)
        DrawTVLines(tv);
    if ((tv.flags & 0x10) != 0)
        return;
    Draw::PrimStruct *cprim = &DrawC::carCache.head.cprim;
    if (tv.state != tv_StateOn)
    {
        POLY_FT4 *texture = reinterpret_cast<POLY_FT4 *>(cprim->PrimPtr);
        uint32 *ot = cprim->LastPrim;
        setaddr(texture, getaddr(ot));
        cprim->PrimPtr = reinterpret_cast<char *>(texture + 1);
        setaddr(ot, texture);
        uint32 gray = uint32(64 - (bright >> 1));
        uint32 color = (gray << 16) | (gray << 8) | gray;
        memcpy(&texture->rgbc, &color, sizeof(color));
        SetPolyFT4(texture);
        SetSemiTrans(texture, 1);
        SetShadeTex(texture, 0);
        texture->p[0].xy.x = videoX;
        texture->p[0].xy.y = videoY;
        texture->p[1].xy.y = videoY;
        texture->p[2].xy.x = videoX;
        texture->p[1].xy.x = (sint16)((uint16)videoX + (uint16)videoWidth);
        texture->p[3].xy.x = texture->p[1].xy.x;
        texture->p[2].xy.y = (sint16)((uint16)videoY + (uint16)videoHeight);
        texture->p[3].xy.y = texture->p[2].xy.y;
        texture->p[0].uv.u = TVNoiseU(noise);
        texture->p[0].uv.v = (uint8)noise->shapey;
        texture->p[1].uv.u = (uint8)(noise->width + TVNoiseU(noise));
        texture->p[1].uv.v = (uint8)noise->shapey;
        texture->p[2].uv.u = TVNoiseU(noise);
        texture->p[2].uv.v = (uint8)(noise->height + noise->shapey);
        texture->p[3].uv.u = (uint8)(noise->width + TVNoiseU(noise));
        texture->p[3].uv.v = (uint8)(noise->height + noise->shapey);
        texture->p[1].attr = (uint16)getTPage(noise->type & 3, 3, (uint16)noise->shapex, (uint16)noise->shapey);
        texture->p[0].attr = GetClut((noise->clutID & 63) << 4, noise->clutID >> 6);
        if ((tv.flags & 4) != 0)
        {
            uint16 axis = tv.flip_axis;
            sint16 fadeTop = TVReflectionFade(axis, (uint16)videoY);
            sint16 fadeBottom = TVReflectionFade(axis, uint32((uint16)videoY) + (uint16)videoHeight);
            POLY_GT4 *reflection = reinterpret_cast<POLY_GT4 *>(cprim->PrimPtr);
            ot = cprim->LastPrim;
            setaddr(reflection, getaddr(ot));
            cprim->PrimPtr = reinterpret_cast<char *>(reflection + 1);
            setaddr(ot, reflection);
            gray = uint32(((128 - sint32(bright)) * (128 - sint32(fadeTop))) / 128);
            color = (gray << 16) | (gray << 8) | gray;
            memcpy(&reflection->p[1].rgbc, &color, sizeof(color));
            memcpy(&reflection->p[0].rgbc, &color, sizeof(color));
            gray = uint32(((128 - sint32(bright)) * (128 - sint32(fadeBottom))) / 128);
            color = (gray << 16) | (gray << 8) | gray;
            memcpy(&reflection->p[3].rgbc, &color, sizeof(color));
            memcpy(&reflection->p[2].rgbc, &color, sizeof(color));
            setcode(reflection, 0x3E);
            setlen(reflection, 12);
            reflection->p[0].xy.x = videoX;
            reflection->p[1].xy.x = (sint16)((uint16)videoX + (uint16)videoWidth);
            reflection->p[0].xy.y = (sint16)(uint32(tv.flip_axis) * 2u - (uint16)videoY);
            reflection->p[2].xy.x = videoX;
            reflection->p[1].xy.y = (sint16)(uint32(tv.flip_axis) * 2u - (uint16)videoY);
            reflection->p[3].xy.x = (sint16)((uint16)videoX + (uint16)videoWidth);
            reflection->p[2].xy.y = (sint16)(uint32(tv.flip_axis) * 2u - (uint16)videoY - (uint16)videoHeight);
            reflection->p[3].xy.y = (sint16)(uint32(tv.flip_axis) * 2u - (uint16)videoY - (uint16)videoHeight);
            reflection->p[0].uv.u = TVNoiseU(noise);
            reflection->p[0].uv.v = (uint8)(noise->shapey - 1);
            reflection->p[1].uv.u = (uint8)(noise->width + TVNoiseU(noise));
            reflection->p[1].uv.v = (uint8)(noise->shapey - 1);
            reflection->p[2].uv.u = TVNoiseU(noise);
            reflection->p[2].uv.v = (uint8)(noise->height + noise->shapey - 1);
            reflection->p[3].uv.u = (uint8)(noise->width + TVNoiseU(noise));
            reflection->p[3].uv.v = (uint8)(noise->height + noise->shapey - 1);
            reflection->p[1].attr = (uint16)getTPage(noise->type & 3, 3, (uint16)noise->shapex, (uint16)noise->shapey);
            reflection->p[0].attr = GetClut((noise->clutID & 63) << 4, noise->clutID >> 6);
        }
    }
    if (tv.state != tv_StateOff)
    {
        POLY_FT4 *texture = reinterpret_cast<POLY_FT4 *>(cprim->PrimPtr);
        uint32 *ot = cprim->LastPrim;
        setaddr(texture, getaddr(ot));
        cprim->PrimPtr = reinterpret_cast<char *>(texture + 1);
        setaddr(ot, texture);
        memcpy(&texture->rgbc, &tint, sizeof(tint));
        SetPolyFT4(texture);
        SetSemiTrans(texture, 0);
        SetShadeTex(texture, do_tint ? 0 : 1);
        texture->p[0].xy.x = videoX;
        texture->p[0].xy.y = videoY;
        texture->p[1].xy.x = (sint16)((uint16)videoX + (uint16)videoWidth);
        texture->p[1].xy.y = videoY;
        texture->p[2].xy.x = videoX;
        texture->p[3].xy.x = texture->p[1].xy.x;
        texture->p[2].xy.y = (sint16)((uint16)videoY + (uint16)videoHeight);
        texture->p[3].xy.y = texture->p[2].xy.y;
        texture->p[0].uv.u = tv.u;
        texture->p[0].uv.v = tv.v;
        texture->p[1].uv.u = (uint8)(tv.u + tv.uw);
        texture->p[1].uv.v = tv.v;
        texture->p[2].uv.u = tv.u;
        texture->p[2].uv.v = (uint8)(tv.v + tv.vh);
        texture->p[3].uv.u = (uint8)(tv.u + tv.uw);
        texture->p[3].uv.v = (uint8)(tv.v + tv.vh);
        texture->p[1].attr = tv.tpage;
        texture->p[0].attr = tv.clut;
        if ((tv.flags & 4) != 0)
        {
            uint16 axis = tv.flip_axis;
            sint16 fadeTop = TVReflectionFade(axis, (uint16)videoY);
            sint16 fadeBottom = TVReflectionFade(axis, uint32((uint16)videoY) + (uint16)videoHeight);
            POLY_GT4 *reflection = reinterpret_cast<POLY_GT4 *>(cprim->PrimPtr);
            ot = cprim->LastPrim;
            setaddr(reflection, getaddr(ot));
            cprim->PrimPtr = reinterpret_cast<char *>(reflection + 1);
            setaddr(ot, reflection);
            setlen(reflection, 12);
            uint32 color = TVScaledTint(tint, (128 - fadeTop));
            reflection->p[0].xy.x = videoX;
            memcpy(&reflection->p[1].rgbc, &color, sizeof(color));
            memcpy(&reflection->p[0].rgbc, &color, sizeof(color));
            color = TVScaledTint(tint, (128 - fadeBottom));
            memcpy(&reflection->p[3].rgbc, &color, sizeof(color));
            memcpy(&reflection->p[2].rgbc, &color, sizeof(color));
            setcode(reflection, 0x3C);
            reflection->p[1].xy.x = (sint16)((uint16)videoX + (uint16)videoWidth);
            reflection->p[0].xy.y = (sint16)(uint32(tv.flip_axis) * 2u - (uint16)videoY);
            reflection->p[2].xy.x = videoX;
            reflection->p[1].xy.y = (sint16)(uint32(tv.flip_axis) * 2u - (uint16)videoY);
            reflection->p[3].xy.x = (sint16)((uint16)videoX + (uint16)videoWidth);
            reflection->p[2].xy.y = (sint16)(uint32(tv.flip_axis) * 2u - (uint16)videoY - (uint16)videoHeight);
            reflection->p[3].xy.y = (sint16)(uint32(tv.flip_axis) * 2u - (uint16)videoY - (uint16)videoHeight);
            reflection->p[0].uv.u = tv.u;
            reflection->p[0].uv.v = (uint8)(tv.v - 1);
            reflection->p[1].uv.u = (uint8)(tv.u + tv.uw);
            reflection->p[1].uv.v = (uint8)(tv.v - 1);
            reflection->p[2].uv.u = tv.u;
            reflection->p[2].uv.v = (uint8)(tv.v + tv.vh - 1);
            reflection->p[3].uv.u = (uint8)(tv.u + tv.uw);
            reflection->p[3].uv.v = (uint8)(tv.v + tv.vh - 1);
            reflection->p[1].attr = tv.tpage;
            reflection->p[0].attr = tv.clut;
        }
    }
}

void InitTV(tTVConfig &tv, tTexture::ShapeInfo *textures, sint16 index)
{
    FUNCTION_MARKER(0x80023008, "FRONT.BIN");
    tTexture::ShapeInfo *shape = &textures[index];
    tv.state = tv_StateOff;
    tv.flags = 0;
    tv.x = (sint16)-((uint16)shape->centerx);
    tv.y = (sint16)-((uint16)shape->centery);
    tv.w = shape->width;
    tv.h = shape->height;
    sint32 offset = (shape->shapex - (sint16)((uint16)shape->shapex & 0xFFC0)) * 16;
    uint8 depth = (uint8)shape->depth;
    if (depth == 0)
        abort();
    tv.u = (uint8)(offset / depth);
    tv.vh = (uint8)tv.h;
    tv.uw = (uint8)tv.w;
    tv.v = (uint8)shape->shapey;
    tv.shapex = shape->shapex;
    tv.shapey = shape->shapey;
    tv.clutID = (uint16)shape->clutID;
    tv.shapeType = (uint8)(shape->type & 3);
    tv.clut = GetClut((tv.clutID & 63) << 4, tv.clutID >> 6);
    tv.tpage = (uint16)(((shape->type & 3) << 7) | (((uint16)shape->shapey & 0x100) >> 4) | (((uint16)shape->shapex & 0x3C0) >> 6) | (((uint16)shape->shapey & 0x200) << 2));
    sint32 random = randPSX();
    sint32 span = tv.h * 48;
    if (span == 0)
        abort();
    tv.fxWide = (sint16)(random % span);
    random = randPSX();
    span = tv.h * 48;
    if (span == 0)
        abort();
    sint16 fxThin = (sint16)(random % span);
    tv.flip_axis = 0;
    tv.transition = 0;
    tv.tint = 0x808080;
    tv.destBrightness = 128;
    tv.fxThin = fxThin;
}

void TurnOffTV(tTVConfig &tv)
{
    FUNCTION_MARKER(0x80023218, "FRONT.BIN");
    tv.state = tv_TransitionOff;
    tv.destBrightness = 0;
}

void TurnOnTV(tTVConfig &tv)
{
    FUNCTION_MARKER(0x80023228, "FRONT.BIN");
    tv.state = tv_TransitionOn;
    tv.destBrightness = 128;
}
