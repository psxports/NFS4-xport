#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "xport.h"
#include "vid.h"
#include "hd.h"
#include "hd_frontend.h"
#include "hd_sampling.h"
#include "hd_ui.h"
#include "game/common/frame.h"
#include "game/psx/draw.h"

struct HdUiView
{
    VID_RECT clip;
    sint32 offsetX;
    sint32 offsetY;
    uint16 page;
};

struct HdUiCommand
{
    float x[4];
    float y[4];
    float u[4];
    float v[4];
    VID_RECT clip;
    uint32 color[4];
    uint32 order;
    uint16 page;
    uint16 clut;
    HdTextureRegion image;
    float offset[2];
    uint16 imagePage;
    sint32 view;
    uint8 kind;
    uint8 vertices;
    uint8 sourceKind;
    uint8 anchor;
    uint8 tachPart;
    uint8 tachPlayer;
    const void *source;
    bool textured;
    bool explicitPage;
    bool semi;
    bool raw;
};

struct HdUiTach
{
    float previous;
    float current;
    float phaseOffset;
    float phaseScale;
    sint32 offsetX;
    sint32 offsetY;
    const void *source;
    bool valid;
};

struct HdUiTachSample
{
    float previous;
    float current;
    float velocity;
    const void *source;
    uint32 tick;
    uint32 renderTick;
    bool valid;
};

struct HdUiFrame
{
    HdUiCommand *commands;
    uint32 count;
    uint32 capacity;
    HdUiView views[10];
    HdUiTach tach[2];
    sint32 width;
    sint32 height;
    sint32 originX;
    sint32 originY;
    sint32 viewCount;
    uint32 worldViews;
    bool paused;
    bool frontend;
    bool frontend3d;
    bool valid;
};

struct HdUiCanvas
{
    float x;
    float y;
    float scaleX;
    float scaleY;
};

struct HdUiVertexCache
{
    HdUiCommand command;
    VID_RECT viewport;
    bool valid;
};

static HdUiFrame uiBuild;
static HdUiFrame uiComplete;
static sint32 uiView;
static HdUiTachSample uiTachSamples[2];
static bool uiActive;
static uint8 uiAnchor;
static HdTextureRegion uiImage;
static uint16 uiImagePage;
static VID_GEOMETRY *uiMesh;
static VID_INDEX *uiIndices;
static HdUiVertexCache *uiVertexCache;
static uint32 uiCapacity;
static VID_RECT *uiViewports;
static VID_MATERIAL *uiMaterials[6];
static VID_SHADER *uiShader;
static VID_MATERIAL *uiSurfaceMaterial;
static VID_MATERIAL *uiCompositeMaterials[2];
static sint32 uiPreparedWidth;
static sint32 uiPreparedHeight;
uint32 hd_ui_frontend_models = 1;
uint32 hd_ui_unknown_pages;
uint32 hd_ui_enabled = 1;
uint32 hd_ui_view_mask = 1023;
uint32 hd_ui_draws_by_view[10];
uint32 hd_ui_command_count;
uint32 hd_ui_draws;
uint32 hd_ui_invalid;
uint32 hd_ui_preview_hash;
uint32 hd_ui_preview_pixels;

HdUiLayout::HdUiLayout(Anchor anchor)
{
    previous = uiAnchor;
    uiAnchor = (uint8)anchor;
}

HdUiLayout::~HdUiLayout(void)
{
    uiAnchor = previous;
}

HdUiImage::HdUiImage(uint16 page, sint32 x, sint32 y, sint32 width, sint32 height)
{
    memcpy(previous, &uiImage, sizeof(uiImage));
    previous[4] = uiImagePage;
    memset(&uiImage, 0, sizeof(uiImage));
    uiImagePage = page & 0x19f;
    if (((page >> 7) & 3) <= 2 && x >= 0 && y >= 0 && width > 0 && height > 0 && x < 256 && y < 256 && width <= 256 - x && height <= 256 - y)
    {
        uiImage.x = (uint16)x;
        uiImage.y = (uint16)y;
        uiImage.width = (uint16)width;
        uiImage.height = (uint16)height;
    }
}

HdUiImage::~HdUiImage(void)
{
    memcpy(&uiImage, previous, sizeof(uiImage));
    uiImagePage = previous[4];
}

void hd_ui_invalidate(void)
{
    FramePresentation::RestoreOriginal();
    hd_frontend_invalidate();
    hd_ui_tach_reset();
    uiActive = false;
    hd_ui_command_count = 0;
    uiBuild.count = uiComplete.count = 0;
    uiBuild.valid = uiComplete.valid = false;
}

void hd_ui_begin_frame(sint32 frame)
{
    FramePresentation::BeginOriginal();
    uiAnchor = HdUiLayout::Center;
    memset(&uiImage, 0, sizeof(uiImage));
    uiImagePage = 0;
    hd_frontend_begin_frame();
    hd_register_cleanup();
    if (frame < 0)
        frame = gFlip;
    uiBuild.count = 0;
    uiBuild.worldViews = 0;
    uiBuild.tach[0].valid = uiBuild.tach[1].valid = false;
    uiBuild.paused = false;
    uiBuild.frontend = uiBuild.frontend3d = false;
    uiBuild.valid = false;
    uiActive = false;
    if (frame < 0 || frame > 1 || Draw::gPlayer1View < 0 || Draw::gPlayer1View >= Draw::gNumView || Draw::gNumView > 10)
        return;
    const DRAWENV *root = &Draw::gView[Draw::gPlayer1View].drawenv[frame];
    uiBuild.width = gEnviro[frame].disp.disp.w;
    uiBuild.height = gEnviro[frame].disp.disp.h;
    if (uiBuild.width <= 0 || uiBuild.height <= 0)
        return;
    uiBuild.originX = root->clip.x;
    uiBuild.originY = root->clip.y;
    uiBuild.viewCount = Draw::gNumView;
    for (sint32 i = 0; i < Draw::gNumView; ++i)
    {
        const DRAWENV *env = &Draw::gView[i].drawenv[frame];
        HdUiView *view = uiBuild.views + i;
        view->clip.x = env->clip.x - uiBuild.originX;
        view->clip.y = env->clip.y - uiBuild.originY;
        view->clip.w = env->clip.w;
        view->clip.h = env->clip.h;
        view->offsetX = env->ofs[0] - uiBuild.originX;
        view->offsetY = env->ofs[1] - uiBuild.originY;
        view->page = env->tpage;
    }
    uiView = Draw::gPlayer1View;
    uiBuild.valid = true;
    uiActive = true;
}

void hd_ui_frontend_frame(bool supported)
{
    if (uiActive)
        uiBuild.frontend = supported;
}

void hd_ui_frontend_3d(void)
{
    if (uiActive)
        uiBuild.frontend3d = true;
}

bool hd_ui_frontend_ready(void)
{
    return uiComplete.valid && !uiComplete.paused && uiComplete.worldViews == 0 && ((uiComplete.frontend && !uiComplete.frontend3d) || (hd_ui_frontend_models != 0 && uiComplete.frontend3d && hd_frontend_ready_ui()));
}

void hd_ui_pause_frame(void)
{
    if (uiActive)
        uiBuild.paused = true;
}

bool hd_ui_pause_ready(void)
{
    return uiComplete.valid && uiComplete.paused;
}

void hd_ui_world_view(sint32 player)
{
    if (uiActive && player >= 0 && player < 2)
        uiBuild.worldViews |= 1 << player;
}

bool hd_ui_world_ready(sint32 views)
{
    return views >= 1 && views <= 2 && uiComplete.valid && uiComplete.worldViews == (uint32)((1 << views) - 1);
}

void hd_ui_set_view(sint32 view)
{
    uiView = view;
}

static HdUiCommand *hd_ui_append(uint8 kind)
{
    if (!uiActive || uiView < 0 || uiView >= uiBuild.viewCount)
        return 0;
    if (uiBuild.count == uiBuild.capacity)
    {
        uint32 capacity = uiBuild.capacity == 0 ? 256 : uiBuild.capacity * 2;
        HdUiCommand *commands = (HdUiCommand *)realloc(uiBuild.commands, capacity * sizeof(*commands));
        if (commands == 0)
        {
            ++hd_ui_invalid;
            uiBuild.valid = false;
            return 0;
        }
        uiBuild.commands = commands;
        uiBuild.capacity = capacity;
    }
    HdUiCommand *command = uiBuild.commands + uiBuild.count;
    memset(command, 0, sizeof(*command));
    command->order = uiBuild.count++;
    command->view = uiView;
    command->kind = kind;
    command->anchor = uiAnchor;
    command->vertices = 4;
    command->image = uiImage;
    command->imagePage = uiImagePage;
    return command;
}

void hd_ui_frontend_marker(uint32 token)
{
    HdUiCommand *command = hd_ui_append(3);
    if (command != 0)
    {
        command->clut = (uint16)token;
        command->vertices = 0;
    }
}

void hd_ui_page(uint16 page)
{
    HdUiCommand *command = hd_ui_append(1);
    if (command != 0)
        command->page = page;
}

void hd_ui_area(const PSX_RECT *area)
{
    HdUiCommand *command = hd_ui_append(2);
    if (command != 0)
    {
        command->clip.x = area->x - uiBuild.originX;
        command->clip.y = area->y - uiBuild.originY;
        command->clip.w = area->w;
        command->clip.h = area->h;
    }
}

static void hd_ui_read(HdUiCommand *command, const SPRT *sprite)
{
    if (sprite->w <= 0 || sprite->h <= 0)
    {
        command->vertices = 0;
        return;
    }
    for (sint32 i = 0; i < 4; ++i)
    {
        command->x[i] = (float)sprite->p[0].xy.x + ((i & 1) != 0 ? sprite->w : 0);
        command->y[i] = (float)sprite->p[0].xy.y + (i >= 2 ? sprite->h : 0);
        command->u[i] = (float)sprite->p[0].uv.u + ((i & 1) != 0 ? sprite->w : 0);
        command->v[i] = (float)sprite->p[0].uv.v + (i >= 2 ? sprite->h : 0);
    }
    command->clut = sprite->p[0].attr;
    for (sint32 i = 0; i < 4; ++i)
        command->color[i] = sprite->rgbc.r | ((uint32)sprite->rgbc.g << 8) | ((uint32)sprite->rgbc.b << 16);
    command->textured = true;
    command->semi = (sprite->rgbc.code & 2) != 0;
    command->raw = (sprite->rgbc.code & 1) != 0;
}

static void hd_ui_read_line(HdUiCommand *command, const uint32 position[4], uint32 color, uint8 code)
{
    float x = (float)(sint16)position[0], y = (float)(sint16)(position[0] >> 16);
    float dx = (float)(sint16)position[1] - x, dy = (float)(sint16)(position[1] >> 16) - y;
    float length = sqrtf(dx * dx + dy * dy);
    if (length > 0)
    {
        dx /= length;
        dy /= length;
    }
    else
    {
        dx = 1;
        dy = 0;
    }
    for (sint32 i = 0; i < 4; ++i)
    {
        float side = (i & 1) != 0 ? 0.5f : -0.5f;
        float end = i >= 2 ? length + 0.5f : -0.5f;
        command->x[i] = x + 0.5f + end * dx - side * dy;
        command->y[i] = y + 0.5f + end * dy + side * dx;
        command->color[i] = color & 0xFFFFFF;
    }
    command->semi = (code & 2) != 0;
}

bool hd_ui_colored(const uint32 position[4], const uint32 color[4], uint8 code)
{
    uint8 kind = code & 0xFC;
    if (kind != 0x20 && kind != 0x30 && kind != 0x40 && kind != 0x50)
        return false;
    HdUiCommand *command = hd_ui_append(0);
    if (command == 0)
        return false;
    if ((code & 0xE0) == 0x40)
    {
        hd_ui_read_line(command, position, color[0], code);
        if ((code & 16) != 0 && position[0] != position[1])
            command->color[2] = command->color[3] = color[1] & 0xFFFFFF;
    }
    else
    {
        command->vertices = 3;
        for (sint32 i = 0; i < 3; ++i)
        {
            command->x[i] = (float)(sint16)position[i];
            command->y[i] = (float)(sint16)(position[i] >> 16);
            command->color[i] = color[(code & 16) != 0 ? i : 0] & 0xFFFFFF;
        }
        command->semi = (code & 2) != 0;
    }
    return true;
}

bool hd_ui_flat(const uint32 position[4], uint32 color, uint8 code)
{
    if ((code & 0xFC) != 0x20 && (code & 0xFC) != 0x40)
        return false;
    uint32 colors[4] = {color, color, color, color};
    return hd_ui_colored(position, colors, code);
}

bool hd_ui_quad(const uint32 position[4], const uint32 color[4], uint8 code, const uint32 texture[4])
{
    HdUiCommand *command = hd_ui_append(0);
    if (command == 0)
        return false;
    for (sint32 i = 0; i < 4; ++i)
    {
        command->x[i] = (float)(sint16)position[i];
        command->y[i] = (float)(sint16)(position[i] >> 16);
        command->color[i] = color[(code & 16) != 0 ? i : 0] & 0xFFFFFF;
    }
    command->semi = (code & 2) != 0;
    if (texture != 0)
    {
        for (sint32 i = 0; i < 4; ++i)
        {
            command->u[i] = (float)(texture[i] & 255);
            command->v[i] = (float)((texture[i] >> 8) & 255);
        }
        command->clut = (uint16)(texture[0] >> 16);
        command->page = (uint16)(texture[1] >> 16);
        command->textured = command->explicitPage = true;
        command->raw = (code & 1) != 0;
    }
    return true;
}

bool hd_ui_glyph(uint32 position, uint32 texture, uint32 color, uint32 size)
{
    HdUiCommand *command = hd_ui_append(0);
    if (command == 0)
        return false;
    sint16 x = (sint16)position;
    sint16 y = (sint16)(position >> 16);
    sint16 width = (sint16)size;
    sint16 height = (sint16)(size >> 16);
    for (sint32 i = 0; i < 4; ++i)
    {
        command->x[i] = (float)x + ((i & 1) != 0 ? width : 0);
        command->y[i] = (float)y + (i >= 2 ? height : 0);
        command->u[i] = (float)(texture & 255) + ((i & 1) != 0 ? width : 0);
        command->v[i] = (float)((texture >> 8) & 255) + (i >= 2 ? height : 0);
        command->color[i] = color & 0xFFFFFF;
    }
    command->clut = (uint16)(texture >> 16);
    command->textured = true;
    command->semi = (color & 0x02000000) != 0;
    command->raw = (color & 0x01000000) != 0;
    return true;
}

void hd_ui_sprite(const SPRT *sprite)
{
    if (sprite->w <= 0 || sprite->h <= 0)
        return;
    HdUiCommand *command = hd_ui_append(0);
    if (command != 0)
        hd_ui_read(command, sprite);
}

static void hd_ui_read(HdUiCommand *command, const POLY_FT4 *quad)
{
    for (sint32 i = 0; i < 4; ++i)
    {
        command->x[i] = (float)quad->p[i].xy.x;
        command->y[i] = (float)quad->p[i].xy.y;
        command->u[i] = (float)quad->p[i].uv.u;
        command->v[i] = (float)quad->p[i].uv.v;
    }
    command->page = quad->p[1].attr;
    command->clut = quad->p[0].attr;
    for (sint32 i = 0; i < 4; ++i)
        command->color[i] = quad->rgbc.r | ((uint32)quad->rgbc.g << 8) | ((uint32)quad->rgbc.b << 16);
    command->textured = true;
    command->semi = (quad->rgbc.code & 2) != 0;
    command->raw = (quad->rgbc.code & 1) != 0;
    command->explicitPage = true;
}

void hd_ui_ft4(const POLY_FT4 *quad)
{
    HdUiCommand *command = hd_ui_append(0);
    if (command != 0)
        hd_ui_read(command, quad);
}

static void hd_ui_read(HdUiCommand *command, const POLY_GT4 *quad)
{
    for (sint32 i = 0; i < 4; ++i)
    {
        command->x[i] = (float)quad->p[i].xy.x;
        command->y[i] = (float)quad->p[i].xy.y;
        command->u[i] = (float)quad->p[i].uv.u;
        command->v[i] = (float)quad->p[i].uv.v;
        command->color[i] = quad->p[i].rgbc.r | ((uint32)quad->p[i].rgbc.g << 8) | ((uint32)quad->p[i].rgbc.b << 16);
    }
    command->page = quad->p[1].attr;
    command->clut = quad->p[0].attr;
    command->semi = (quad->p[0].rgbc.code & 2) != 0;
    command->raw = (quad->p[0].rgbc.code & 1) != 0;
    command->textured = command->explicitPage = true;
}

void hd_ui_gt4(const POLY_GT4 *quad)
{
    HdUiCommand *command = hd_ui_append(0);
    if (command != 0)
        hd_ui_read(command, quad);
}

static void hd_ui_read(HdUiCommand *command, const POLY_F4 *quad)
{
    for (sint32 i = 0; i < 4; ++i)
    {
        command->x[i] = (float)quad->p[i].x;
        command->y[i] = (float)quad->p[i].y;
        command->color[i] = quad->rgbc.r | ((uint32)quad->rgbc.g << 8) | ((uint32)quad->rgbc.b << 16);
    }
    command->semi = (quad->rgbc.code & 2) != 0;
}

void hd_ui_f4(const POLY_F4 *quad)
{
    HdUiCommand *command = hd_ui_append(0);
    if (command != 0)
        hd_ui_read(command, quad);
}

static void hd_ui_read(HdUiCommand *command, const POLY_G4 *quad)
{
    for (sint32 i = 0; i < 4; ++i)
    {
        command->x[i] = (float)quad->p[i].xy.x;
        command->y[i] = (float)quad->p[i].xy.y;
        command->color[i] = quad->p[i].rgbc.r | ((uint32)quad->p[i].rgbc.g << 8) | ((uint32)quad->p[i].rgbc.b << 16);
    }
    command->semi = (quad->p[0].rgbc.code & 2) != 0;
}

void hd_ui_g4(const POLY_G4 *quad)
{
    HdUiCommand *command = hd_ui_append(0);
    if (command != 0)
        hd_ui_read(command, quad);
}

static void hd_ui_read(HdUiCommand *command, const POLY_F3 *triangle)
{
    command->vertices = 3;
    for (sint32 i = 0; i < 3; ++i)
    {
        command->x[i] = (float)triangle->p[i].x;
        command->y[i] = (float)triangle->p[i].y;
        command->color[i] = triangle->rgbc.r | ((uint32)triangle->rgbc.g << 8) | ((uint32)triangle->rgbc.b << 16);
    }
    command->semi = (triangle->rgbc.code & 2) != 0;
}

static void hd_ui_read(HdUiCommand *command, const POLY_G3 *triangle)
{
    command->vertices = 3;
    for (sint32 i = 0; i < 3; ++i)
    {
        command->x[i] = (float)triangle->p[i].xy.x;
        command->y[i] = (float)triangle->p[i].xy.y;
        command->color[i] = triangle->p[i].rgbc.r | ((uint32)triangle->p[i].rgbc.g << 8) | ((uint32)triangle->p[i].rgbc.b << 16);
    }
    command->semi = (triangle->p[0].rgbc.code & 2) != 0;
}

static void hd_ui_read(HdUiCommand *command, const LINE_F2 *line)
{
    uint32 position[4] = {0, 0, 0, 0};
    uint32 color;
    memcpy(position, line->p, sizeof(line->p));
    memcpy(&color, &line->rgbc, sizeof(color));
    hd_ui_read_line(command, position, color, line->rgbc.code);
}

static void hd_ui_read(HdUiCommand *command, const LINE_G2 *line)
{
    LINE_F2 flat;
    flat.rgbc = line->p[0].rgbc;
    for (sint32 i = 0; i < 2; ++i)
    {
        flat.p[i].x = line->p[i].xy.x;
        flat.p[i].y = line->p[i].xy.y;
    }
    hd_ui_read(command, &flat);
    if (flat.p[0].x != flat.p[1].x || flat.p[0].y != flat.p[1].y)
        command->color[2] = command->color[3] = line->p[1].rgbc.r | ((uint32)line->p[1].rgbc.g << 8) | ((uint32)line->p[1].rgbc.b << 16);
}

void hd_ui_queue(const SPRT *source)
{
    HdUiCommand *command = hd_ui_append(0);
    if (command != 0)
    {
        command->sourceKind = 1;
        command->source = source;
    }
}

void hd_ui_queue(const POLY_FT4 *source)
{
    HdUiCommand *command = hd_ui_append(0);
    if (command != 0)
    {
        command->sourceKind = 2;
        command->source = source;
    }
}

void hd_ui_queue(const POLY_GT4 *source)
{
    HdUiCommand *command = hd_ui_append(0);
    if (command != 0)
    {
        command->sourceKind = 3;
        command->source = source;
    }
}

void hd_ui_queue(const POLY_F4 *source)
{
    HdUiCommand *command = hd_ui_append(0);
    if (command != 0)
    {
        command->sourceKind = 4;
        command->source = source;
    }
}

void hd_ui_queue(const POLY_G4 *source)
{
    HdUiCommand *command = hd_ui_append(0);
    if (command != 0)
    {
        command->sourceKind = 5;
        command->source = source;
    }
}

void hd_ui_queue(const POLY_F3 *source)
{
    HdUiCommand *command = hd_ui_append(0);
    if (command != 0)
    {
        command->sourceKind = 6;
        command->source = source;
    }
}

void hd_ui_queue(const POLY_G3 *source)
{
    HdUiCommand *command = hd_ui_append(0);
    if (command != 0)
    {
        command->sourceKind = 9;
        command->source = source;
    }
}

void hd_ui_queue(const LINE_G2 *source)
{
    HdUiCommand *command = hd_ui_append(0);
    if (command != 0)
    {
        command->sourceKind = 10;
        command->source = source;
    }
}

void hd_ui_queue(const LINE_F2 *source)
{
    HdUiCommand *command = hd_ui_append(0);
    if (command != 0)
    {
        command->sourceKind = 7;
        command->source = source;
    }
}

void hd_ui_queue(const DR_MODE *source)
{
    HdUiCommand *command = hd_ui_append(1);
    if (command != 0)
    {
        command->sourceKind = 8;
        command->source = source;
    }
}

void hd_ui_tach_reset(bool hold)
{
    for (sint32 player = 0; player < 2; ++player)
    {
        if (hold)
        {
            uiComplete.tach[player].previous = uiComplete.tach[player].current;
            uiTachSamples[player].previous = uiTachSamples[player].current;
            uiTachSamples[player].velocity = 0;
        }
        else
        {
            uiComplete.tach[player].valid = uiBuild.tach[player].valid = false;
            uiTachSamples[player].valid = false;
        }
    }
}

void hd_ui_offset_last(float x, float y)
{
    if (!uiActive || uiBuild.count == 0)
        return;
    HdUiCommand *command = uiBuild.commands + uiBuild.count - 1;
    command->offset[0] += x;
    command->offset[1] += y;
}

void hd_ui_tach_sample(sint32 player, float rpm, const void *source, uint32 tick)
{
    if (player < 0 || player >= 2)
        return;
    HdUiTachSample *sample = uiTachSamples + player;
    if (!sample->valid || sample->source != source || tick < sample->renderTick || tick - sample->tick > 16)
    {
        sample->previous = sample->current = rpm;
        sample->velocity = 0;
        sample->source = source;
        sample->tick = tick;
        sample->valid = source != 0;
    }
    else if (tick != sample->tick)
    {
        for (uint32 step = sample->tick; step < tick; ++step)
        {
            sample->previous = sample->current;
            float difference = sample->current - rpm;
            float change = (sample->velocity + 48 * difference) / 64;
            sample->current = rpm + (difference + change) * 0.472366553f;
            sample->velocity = (sample->velocity - 48 * change) * 0.472366553f;
        }
        sample->tick = tick;
    }
    sample->renderTick = tick;
}

static float hd_ui_tach_angle(float rpm)
{
    float angle = rpm * (65536.0f / 10800) + 0x5999;
    if (angle < 0x5999)
        angle = 0x5999;
    if (angle > 0x13334)
        angle = 0x13334;
    return angle * (6.28318530718f / 65536);
}

void hd_ui_tach(sint32 player, float rpm, const void *source, const Draw::tPixMap *pixmap)
{
    if (!uiActive || player < 0 || player >= 2 || uiBuild.count < 4)
        return;
    HdUiCommand *commands = uiBuild.commands + uiBuild.count - 4;
    for (sint32 i = 0; i < 4; ++i)
        if (commands[i].kind != 0 || commands[i].view != uiView)
            return;
    HdUiTach *tach = uiBuild.tach + player;
    const HdUiTach *previous = uiComplete.tach + player;
    tach->current = hd_ui_tach_angle(rpm);
    tach->previous = uiComplete.valid && !uiComplete.paused && previous->valid && previous->source == source ? previous->current : tach->current;
    tach->phaseOffset = 0;
    tach->phaseScale = 1;
    const HdUiTachSample *sample = uiTachSamples + player;
    if (sample->valid && sample->source == source)
    {
        tach->previous = hd_ui_tach_angle(sample->previous);
        tach->current = hd_ui_tach_angle(sample->current);
        tach->phaseOffset = (float)(sample->renderTick - sample->tick);
        tach->phaseScale = 1;
    }
    tach->source = source;
    tach->offsetX = uiBuild.views[uiView].offsetX;
    tach->offsetY = uiBuild.views[uiView].offsetY;
    tach->valid = true;
    for (sint32 i = 0; i < 4; ++i)
    {
        commands[i].tachPart = (uint8)(i + 1);
        commands[i].tachPlayer = (uint8)player;
    }
    hd_texture_region(pixmap, &commands[3].image);
    commands[3].imagePage = pixmap->tpage & 0x19f;
}

static void hd_ui_tach_command(HdUiCommand *command, const HdUiTach *tach, float phase)
{
    float previous = tach->previous;
    float current = tach->current;
    float phaseOffset = tach->phaseOffset;
    float phaseScale = tach->phaseScale;
    const HdUiTachSample *sample = uiTachSamples + command->tachPlayer;
    if (!uiComplete.paused && sample->valid && sample->source == tach->source)
    {
        previous = hd_ui_tach_angle(sample->previous);
        current = hd_ui_tach_angle(sample->current);
        phaseOffset = (float)(sample->renderTick - sample->tick);
        phaseScale = 1;
    }
    phase = (phase + phaseOffset) * phaseScale;
    if (phase < 0)
        phase = 0;
    if (phase > 1)
        phase = 1;
    float angle = previous + (current - previous) * phase;
    float cosine = cosf(angle), sine = sinf(angle);
    if (command->tachPart == 4)
    {
        for (sint32 i = 0; i < 4; ++i)
        {
            command->u[i] = cosine * 29 + (command->tachPlayer == 0 ? 29 : 117) + ((i & 1) != 0 ? 28 : 0);
            command->v[i] = sine * 29 + 157 + (i >= 2 ? 28 : 0);
        }
    }
    else if (command->tachPart == 2)
    {
        for (sint32 i = 0; i < 4; ++i)
        {
            float side = (i & 1) != 0 ? 0.5f : -0.5f;
            float end = i >= 2 ? 39.5f : -0.5f;
            command->x[i] = tach->offsetX + 14 - cosine * 29 + 0.5f + end * cosine - side * sine;
            command->y[i] = tach->offsetY + 14 - sine * 29 + 0.5f + end * sine + side * cosine;
        }
    }
    else
    {
        float shadow = command->tachPart == 3 ? 2.0f : 0.0f;
        command->x[0] = tach->offsetX + 14 + cosine * 10;
        command->y[0] = tach->offsetY + 14 + sine * 10 + shadow;
        for (sint32 i = 1; i < 3; ++i)
        {
            float tip = angle + (i == 1 ? -1 : 1) * (6.28318530718f / 128);
            command->x[i] = tach->offsetX + 14 - cosf(tip) * 32;
            command->y[i] = tach->offsetY + 14 - sinf(tip) * 32 + shadow;
        }
    }
}

static int hd_ui_order(const void *left, const void *right)
{
    const HdUiCommand *a = (const HdUiCommand *)left;
    const HdUiCommand *b = (const HdUiCommand *)right;
    if (a->view != b->view)
        return a->view < b->view ? -1 : 1;
    return a->order > b->order ? -1 : a->order < b->order ? 1 : 0;
}

void hd_ui_end_frame(void)
{
    if (!uiActive)
        return;
    uiActive = false;
    hd_frontend_end_frame();
    for (uint32 i = 0; i < uiBuild.count; ++i)
    {
        HdUiCommand *command = uiBuild.commands + i;
        switch (command->sourceKind)
        {
            case 1:
                hd_ui_read(command, (const SPRT *)command->source);
                break;
            case 2:
                hd_ui_read(command, (const POLY_FT4 *)command->source);
                break;
            case 3:
                hd_ui_read(command, (const POLY_GT4 *)command->source);
                break;
            case 4:
                hd_ui_read(command, (const POLY_F4 *)command->source);
                break;
            case 5:
                hd_ui_read(command, (const POLY_G4 *)command->source);
                break;
            case 6:
                hd_ui_read(command, (const POLY_F3 *)command->source);
                break;
            case 7:
                hd_ui_read(command, (const LINE_F2 *)command->source);
                break;
            case 8:
                command->page = (uint16)(((const DR_MODE *)command->source)->code[0] & 0x1ff);
                break;
            case 9:
                hd_ui_read(command, (const POLY_G3 *)command->source);
                break;
            case 10:
                hd_ui_read(command, (const LINE_G2 *)command->source);
                break;
        }
        command->source = 0;
        command->sourceKind = 0;
        for (sint32 vertex = 0; vertex < 4; ++vertex)
        {
            command->x[vertex] += command->offset[0];
            command->y[vertex] += command->offset[1];
        }
    }
    qsortPSX(uiBuild.commands, uiBuild.count, sizeof(*uiBuild.commands), hd_ui_order);
    sint32 currentView = -1;
    VID_RECT clip = {0, 0, 0, 0};
    uint16 page = 0;
    bool pageKnown = true;
    uint32 used = 0;
    for (uint32 i = 0; i < uiBuild.count; ++i)
    {
        HdUiCommand command = uiBuild.commands[i];
        const HdUiView *view = uiBuild.views + command.view;
        if (currentView != command.view)
        {
            currentView = command.view;
            clip = view->clip;
            page = view->page;
            pageKnown = true;
        }
        if (command.kind == 1)
        {
            page = command.page;
            pageKnown = true;
            continue;
        }
        if (command.kind == 2)
        {
            clip = command.clip;
            continue;
        }
        if (command.kind == 3)
        {
            pageKnown = hd_frontend_page_ui(command.clut, &page);
            command.clip = clip;
            command.x[0] = (float)view->offsetX;
            command.y[0] = (float)view->offsetY;
            uiBuild.commands[used++] = command;
            continue;
        }
        if (command.explicitPage)
        {
            page = command.page;
            pageKnown = true;
        }
        else
        {
            command.page = page;
            if (!pageKnown && (command.textured || command.semi))
            {
                uiBuild.valid = false;
                ++hd_ui_unknown_pages;
            }
        }
        command.clip = clip;
        for (sint32 point = 0; point < 4; ++point)
        {
            command.x[point] += view->offsetX;
            command.y[point] += view->offsetY;
        }
        uiBuild.commands[used++] = command;
    }
    uiBuild.count = used;
    HdUiFrame saved = uiComplete;
    uiComplete = uiBuild;
    uiBuild = saved;
    hd_ui_command_count = uiComplete.count;
}

static HdUiCanvas hd_ui_canvas(sint32 width, sint32 height, uint8 anchor)
{
    float fitWidth = (float)width, fitHeight = width * 0.75f;
    if (fitHeight > height)
    {
        fitHeight = (float)height;
        fitWidth = height * (4.0f / 3);
    }
    HdUiCanvas canvas;
    canvas.x = anchor == HdUiLayout::Left || anchor == HdUiLayout::Stretch ? 0 : anchor == HdUiLayout::Right ? width - fitWidth : (width - fitWidth) * 0.5f;
    canvas.y = (height - fitHeight) * 0.5f;
    canvas.scaleX = (anchor == HdUiLayout::Stretch ? width : fitWidth) / uiComplete.width;
    canvas.scaleY = fitHeight / uiComplete.height;
    return canvas;
}

static sint32 hd_ui_pixel_edge(float position)
{
    sint32 edge = (sint32)position;
    return edge + (position > edge);
}

static VID_RECT hd_ui_clip(const VID_RECT *clip, const HdUiCanvas *canvas)
{
    float left = clip->x > 0 ? (float)clip->x : 0;
    float top = clip->y > 0 ? (float)clip->y : 0;
    float right = (float)(clip->x + clip->w);
    float bottom = (float)(clip->y + clip->h);
    if (right > uiComplete.width)
        right = (float)uiComplete.width;
    if (bottom > uiComplete.height)
        bottom = (float)uiComplete.height;
    VID_RECT viewport;
    viewport.x = hd_ui_pixel_edge(canvas->x + left * canvas->scaleX - 0.5f);
    viewport.y = hd_ui_pixel_edge(canvas->y + top * canvas->scaleY - 0.5f);
    viewport.w = hd_ui_pixel_edge(canvas->x + right * canvas->scaleX - 0.5f) - viewport.x;
    viewport.h = hd_ui_pixel_edge(canvas->y + bottom * canvas->scaleY - 0.5f) - viewport.y;
    return viewport;
}

static void hd_ui_frontend_mapping(sint32 width, sint32 height, const VID_RECT *viewport, float mapping[4], float offsetX = 0, float offsetY = 0)
{
    HdUiCanvas canvas = hd_ui_canvas(width, height, HdUiLayout::Center);
    float fitWidth = canvas.scaleX * uiComplete.width, fitHeight = canvas.scaleY * uiComplete.height;
    float originX = canvas.x + offsetX * canvas.scaleX, originY = canvas.y + offsetY * canvas.scaleY;
    mapping[0] = fitWidth / viewport->w;
    mapping[1] = fitHeight / viewport->h;
    mapping[2] = (2 * (originX - viewport->x) + fitWidth) / viewport->w - 1;
    mapping[3] = 1 - (2 * (originY - viewport->y) + fitHeight) / viewport->h;
}

static const char uiVertex[] = "layout(location=0)in vec4 aPos;layout(location=1)in vec2 aUV;layout(location=2)in vec4 aColor;layout(location=4)in float aAlpha;flat out float vUiAlpha;\n"
                               "layout(location=5)in uvec4 aTex;layout(location=6)in uvec2 aRect;out vec2 vUV;out vec4 vColor;out float vDepth;out vec2 vFog;flat out uvec4 vUiTex;flat out ivec4 vRegion;\n"
                               "void main(){gl_Position=aPos;vUV=aUV;vColor=aColor/255.0;vUiAlpha=aAlpha;vDepth=0.0;vFog=vec2(0.0);vUiTex=aTex;vRegion=ivec4(ivec2((aTex.z>>1)&255u,(aTex.w>>1)&255u),ivec2(aRect));}\n";

static const char uiFragment[] = "flat in float vUiAlpha;flat in uvec4 vUiTex;flat in ivec4 vRegion;layout(location=1)out vec4 outSurface;\n" VID_PSX_SAMPLING_GLSL HD_VRAM_WORD_GLSL HD_PSX_COLOR_GLSL HD_PSX_PREMULTIPLIED_GLSL "void main(){bool maskOnly=uParams[0].z>0.5;bool composite=uParams[0].w>0.5;outSurface=vec4(0.0,0.0,0.0,1.0);"
                                 "if((vUiTex.w&1u)!=0u){outColor=composite?hdPsxPremultiplied(vColor.rgb,(vUiTex.x>>5)&3u,vUiAlpha<0.75):vec4(vColor.rgb,maskOnly?0.0:uParams[0].y);return;}"
                                 "uint word=vidPsxDecode(vUiTex.x,vUiTex.y,ivec2(floor(vUV)));"
                                 "vec4 color=hdPsxColor(vUV,vUiTex.x,vUiTex.y,vRegion,composite?(vUiAlpha<0.75?((word&32768u)!=0u?2u:1u):0u):uint(uParams[0].x),word);if(color.a==0.0)discard;"
                                 "outSurface.r=(word&32768u)!=0u?1.0:0.0;vec3 rgb=(vUiTex.z&1u)!=0u?color.rgb:color.rgb*vColor.rgb*(255.0/128.0);"
                                 "outColor=composite?hdPsxPremultiplied(clamp(rgb,0.0,1.0),(vUiTex.x>>5)&3u,vUiAlpha<0.75&&(word&32768u)!=0u):vec4(clamp(rgb,0.0,1.0),maskOnly?0.0:uParams[0].y);}";

bool hd_ui_prepare(sint32 width, sint32 height, float phase)
{
    if (!uiComplete.valid || width <= 0 || height <= 0)
        return false;
    bool layoutChanged = uiPreparedWidth != width || uiPreparedHeight != height;
    uiPreparedWidth = width;
    uiPreparedHeight = height;
    if (uiComplete.count == 0)
        return true;
    if (hd_ui_frontend_models != 0 && uiComplete.frontend3d && !hd_frontend_prepare_ui())
        return false;
    VID_TEXTURE *vram = hd_vram_texture();
    if (vram == 0)
        return false;
    if (uiShader == 0)
        uiShader = vid_shader_create(uiVertex, uiFragment);
    if (uiShader == 0)
        return false;
    for (sint32 pass = 0; pass < 6; ++pass)
        if (uiMaterials[pass] == 0)
        {
            uiMaterials[pass] = vid_mat_create();
            if (uiMaterials[pass] == 0)
                return false;
            VID_MATERIAL_DATA *material = vid_mat_data(uiMaterials[pass]);
            material->texture = vram;
            material->shader = uiShader;
            material->params[0] = pass == 0 ? 0.0f : pass == 1 ? 1.0f : 2.0f;
            material->params[1] = pass == 2 ? 0.5f : pass == 5 ? 0.25f : 1.0f;
            material->blend = pass < 2 ? VID_BLEND_NONE : pass == 2 ? VID_BLEND_ALPHA : pass == 4 ? VID_BLEND_SUBTRACT : VID_BLEND_ADD;
            material->filter = VID_FILTER_NEAREST;
            material->depth_test = 0;
            material->depth_write = 0;
            vid_mat_commit(uiMaterials[pass]);
        }
    if (uiSurfaceMaterial == 0)
    {
        uiSurfaceMaterial = vid_mat_create();
        if (uiSurfaceMaterial == 0)
            return false;
        VID_MATERIAL_DATA *material = vid_mat_data(uiSurfaceMaterial);
        material->texture = vram;
        material->shader = uiShader;
        material->params[2] = 1;
        material->blend = VID_BLEND_ALPHA;
        material->surface_write = 1;
        material->filter = VID_FILTER_NEAREST;
        material->depth_test = material->depth_write = 0;
        vid_mat_commit(uiSurfaceMaterial);
    }
    for (sint32 surface = 0; surface < 2; ++surface)
        if (uiCompositeMaterials[surface] == 0)
        {
            uiCompositeMaterials[surface] = vid_mat_create();
            if (uiCompositeMaterials[surface] == 0)
                return false;
            VID_MATERIAL_DATA *material = vid_mat_data(uiCompositeMaterials[surface]);
            *material = *vid_mat_read(uiMaterials[2]);
            material->params[3] = 1;
            material->blend = VID_BLEND_PREMULTIPLIED;
            material->surface_write = surface;
            vid_mat_commit(uiCompositeMaterials[surface]);
        }
    if (uiCapacity < uiComplete.count)
    {
        VID_RECT *viewports = (VID_RECT *)realloc(uiViewports, uiComplete.count * sizeof(*viewports));
        if (viewports == 0)
            return false;
        uiViewports = viewports;
        vid_mesh_free(uiMesh);
        vid_index_free(uiIndices);
        uiIndices = 0;
        free(uiVertexCache);
        uiVertexCache = (HdUiVertexCache *)calloc(uiComplete.count, sizeof(*uiVertexCache));
        uiMesh = vid_mesh_create_2d(uiComplete.count * 4);
        uiIndices = vid_index_create(uiComplete.count * 6);
        uiCapacity = uiMesh != 0 ? uiComplete.count : 0;
        if (uiMesh == 0 || uiIndices == 0 || uiVertexCache == 0)
            return false;
        uint32 *indices = vid_index_data(uiIndices);
        const uint32 order[6] = {0, 1, 2, 2, 1, 3};
        for (uint32 i = 0; i < uiCapacity; ++i)
            for (sint32 corner = 0; corner < 6; ++corner)
                indices[i * 6 + corner] = i * 4 + order[corner];
        vid_index_commit(uiIndices, uiCapacity * 6);
        vid_index_static(uiIndices);
    }
    VID_VERTEX2D *vertices = vid_mesh_data_2d(uiMesh);
    HdUiCanvas canvases[4];
    for (sint32 i = 0; i < 4; ++i)
        canvases[i] = hd_ui_canvas(width, height, (uint8)i);
    for (uint32 i = 0; i < uiComplete.count; ++i)
    {
        const HdUiCommand *command = uiComplete.commands + i;
        HdUiCommand interpolated;
        if (command->tachPart != 0 && command->tachPlayer < 2 && uiComplete.tach[command->tachPlayer].valid)
        {
            interpolated = *command;
            hd_ui_tach_command(&interpolated, uiComplete.tach + command->tachPlayer, phase);
            command = &interpolated;
        }
        const HdUiCanvas *canvas = canvases + (command->anchor <= HdUiLayout::Stretch ? command->anchor : HdUiLayout::Center);
        VID_RECT *viewport = uiViewports + i;
        *viewport = hd_ui_clip(&command->clip, canvas);
        if (command->kind == 3)
        {
            uiVertexCache[i].valid = false;
            continue;
        }
        HdUiVertexCache *cached = uiVertexCache + i;
        if (!layoutChanged && cached->valid && memcmp(&cached->command, command, sizeof(*command)) == 0 && memcmp(&cached->viewport, viewport, sizeof(*viewport)) == 0)
            continue;
        cached->command = *command;
        cached->viewport = *viewport;
        cached->valid = true;
        VID_VERTEX2D updated[4];
        for (sint32 vertex = 0; vertex < 4; ++vertex)
        {
            sint32 corner = command->vertices == 3 && vertex == 3 ? 1 : vertex;
            VID_VERTEX2D *out = updated + vertex;
            memset(out, 0, sizeof(*out));
            if (viewport->w > 0 && viewport->h > 0)
            {
                out->pos[0] = (canvas->x + command->x[corner] * canvas->scaleX - viewport->x) * 2 / viewport->w - 1;
                out->pos[1] = 1 - (canvas->y + command->y[corner] * canvas->scaleY - viewport->y) * 2 / viewport->h;
            }
            out->pos[3] = 1;
            out->uv[0] = command->u[corner];
            out->uv[1] = command->v[corner];
            out->color[0] = (uint8)command->color[corner];
            out->color[1] = (uint8)(command->color[corner] >> 8);
            out->color[2] = (uint8)(command->color[corner] >> 16);
            out->color[3] = 255;
            out->alpha = command->semi ? 0.5f : 1.0f;
            out->tex[0] = command->page;
            out->tex[1] = command->clut;
            out->tex[2] = command->raw;
            out->tex[3] = !command->textured;
            if (command->textured && (hd_texture_filtering_enabled & HD_FILTER_BACKDROP) != 0 && (command->page & 0x19f) == command->imagePage)
            {
                out->tex[2] |= command->image.x << 1;
                out->tex[3] |= command->image.y << 1;
                out->rect[0] = command->image.width;
                out->rect[1] = command->image.height;
            }
        }
        if (memcmp(vertices + i * 4, updated, sizeof(updated)) != 0)
        {
            memcpy(vertices + i * 4, updated, sizeof(updated));
            vid_mesh_commit_range(uiMesh, i * 4, 4);
        }
    }
    return true;
}

bool hd_ui_render(bool surface)
{
    if (!uiComplete.valid)
        return false;
    bool success = true;
    hd_ui_draws = 0;
    memset(hd_ui_draws_by_view, 0, sizeof(hd_ui_draws_by_view));
    if (hd_ui_enabled == 0)
        return true;
    if (hd_ui_frontend_models != 0 && uiComplete.frontend3d)
        hd_frontend_begin_ui();
    for (uint32 i = 0; i < uiComplete.count; ++i)
    {
        const HdUiCommand *command = uiComplete.commands + i;
        if ((hd_ui_view_mask & (1 << command->view)) == 0 || uiViewports[i].w <= 0 || uiViewports[i].h <= 0)
            continue;
        vid_viewport(uiViewports[i]);
        if (command->kind == 3)
        {
            if (hd_ui_frontend_models != 0)
            {
                if (surface)
                    return false;
                float mapping[4];
                hd_ui_frontend_mapping(uiPreparedWidth, uiPreparedHeight, uiViewports + i, mapping, command->x[0], command->y[0]);
                success = hd_frontend_draw_ui(command->clut, mapping) && success;
                ++hd_ui_draws;
                ++hd_ui_draws_by_view[command->view];
            }
            continue;
        }
        if (command->vertices == 0)
            continue;
        vid_uniforms(0, 0);
        if (!command->semi || ((command->page >> 5) & 3) != 2)
        {
            uint32 last = i;
            uint32 views = 1 << command->view;
            while (last + 1 < uiComplete.count)
            {
                const HdUiCommand *next = uiComplete.commands + last + 1;
                if (next->kind == 3 || next->vertices == 0 || (hd_ui_view_mask & (1 << next->view)) == 0 || uiViewports[last + 1].w <= 0 || uiViewports[last + 1].h <= 0 || (next->semi && ((next->page >> 5) & 3) == 2) || memcmp(uiViewports + i, uiViewports + last + 1, sizeof(VID_RECT)) != 0)
                    break;
                ++last;
                views |= 1 << next->view;
            }
            vid_pass(uiCompositeMaterials[surface ? 1 : 0]);
            success = vid_submit(uiMesh, uiIndices, i * 6, (last - i + 1) * 6) != 0 && success;
            ++hd_ui_draws;
            for (sint32 view = 0; view < 10; ++view)
                if ((views & (1 << view)) != 0)
                    ++hd_ui_draws_by_view[view];
            i = last;
            continue;
        }
        vid_pass(uiMaterials[!command->semi ? 0 : command->textured ? 1 : 2 + ((command->page >> 5) & 3)]);
        success = vid_submit(uiMesh, uiIndices, i * 6, command->vertices == 3 ? 3 : 6) != 0 && success;
        ++hd_ui_draws;
        ++hd_ui_draws_by_view[command->view];
        if (command->semi && command->textured)
        {
            vid_pass(uiMaterials[2 + ((command->page >> 5) & 3)]);
            success = vid_submit(uiMesh, uiIndices, i * 6, command->vertices == 3 ? 3 : 6) != 0 && success;
            ++hd_ui_draws;
            ++hd_ui_draws_by_view[command->view];
        }
        if (surface)
        {
            vid_pass(uiSurfaceMaterial);
            success = vid_submit(uiMesh, uiIndices, i * 6, command->vertices == 3 ? 3 : 6) != 0 && success;
            ++hd_ui_draws;
            ++hd_ui_draws_by_view[command->view];
        }
    }
    return success;
}

bool hd_ui_present(void)
{
    sint32 width, height;
    vid_resize(&width, &height);
    if (!hd_ui_frontend_ready() || !hd_ui_prepare(width, height) || !vid_begin(0, 0, 0))
        return false;
    if (!hd_ui_render(false))
    {
        vid_cancel();
        return false;
    }
    return vid_end() != 0;
}

bool hd_ui_preview(void)
{
    hd_frontend_sample(1);
    if (!uiComplete.valid || uiComplete.count == 0)
        return false;
    sint32 width, height;
    vid_resize(&width, &height);
    if (width <= 0 || height <= 0 || !hd_ui_prepare(width, height))
        return false;
    VID_SAMPLE *samples[80 * 60];
    for (sint32 i = 0; i < 80 * 60; ++i)
        samples[i] = vid_sample_create();
    if (!vid_begin(0, 0, 0))
    {
        for (sint32 i = 0; i < 80 * 60; ++i)
            vid_sample_free(samples[i]);
        return false;
    }
    if (!hd_ui_render(false))
    {
        vid_cancel();
        for (sint32 i = 0; i < 80 * 60; ++i)
            vid_sample_free(samples[i]);
        return false;
    }
    VID_RECT viewport = {0, 0, width, height};
    vid_viewport(viewport);
    for (sint32 y = 0; y < 60; ++y)
        for (sint32 x = 0; x < 80; ++x)
            vid_sample(samples[y * 80 + x], (x + 0.5f) / 80, (y + 0.5f) / 60);
    bool result = vid_end() != 0;
#if defined(_DEBUG)
    FILE *file = result ? xport_fopen("../status/enhance/ui-preview.ppm", "wb") : 0;
    if (file != 0)
        fprintf(file, "P6\n80 60\n255\n");
#endif
    hd_ui_preview_hash = 2166136261;
    hd_ui_preview_pixels = 0;
    for (sint32 i = 0; i < 80 * 60; ++i)
    {
        uint32 color = vid_sample_color(samples[i]);
        hd_ui_preview_hash = (hd_ui_preview_hash ^ color) * 16777619;
        if (color != 0)
            ++hd_ui_preview_pixels;
#if defined(_DEBUG)
        uint8 rgb[3] = {(uint8)(color >> 16), (uint8)(color >> 8), (uint8)color};
        if (file != 0)
            fwrite(rgb, 1, 3, file);
#endif
        vid_sample_free(samples[i]);
    }
#if defined(_DEBUG)
    if (file != 0)
        fclose(file);
#endif
    return result;
}

void hd_ui_release(void)
{
    hd_ui_invalidate();
    FramePresentation::ReleaseOriginal();
    free(uiBuild.commands);
    free(uiComplete.commands);
    free(uiViewports);
    memset(&uiBuild, 0, sizeof(uiBuild));
    memset(&uiComplete, 0, sizeof(uiComplete));
    uiViewports = 0;
    uiActive = false;
    vid_mesh_free(uiMesh);
    uiMesh = 0;
    vid_index_free(uiIndices);
    uiIndices = 0;
    free(uiVertexCache);
    uiVertexCache = 0;
    uiCapacity = 0;
    for (sint32 i = 0; i < 6; ++i)
    {
        vid_mat_free(uiMaterials[i]);
        uiMaterials[i] = 0;
    }
    vid_mat_free(uiSurfaceMaterial);
    for (sint32 surface = 0; surface < 2; ++surface)
    {
        vid_mat_free(uiCompositeMaterials[surface]);
        uiCompositeMaterials[surface] = 0;
    }
    uiSurfaceMaterial = 0;
    vid_shader_free(uiShader);
    uiShader = 0;
}
