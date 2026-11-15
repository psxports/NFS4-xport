#include <stdlib.h>
#include <string.h>
#include "xport.h"
#include "psx_gpu.h"
#include "vid.h"
#include "hd.h"
#include "hd_ui.h"
#include "game/common/frame.h"
#include "game/common/render.h"
#include "game/psx/draw.h"

struct OriginalGlyph
{
    void *marker;
    uint32 position;
    uint32 texture;
    uint32 color;
    uint32 size;
};

struct OriginalQuad
{
    void *marker;
    uint32 position[4];
    uint32 color[4];
    uint32 texture[4];
    uint8 code;
};

static OriginalQuad *originalQuads;
static uint32 originalQuadCount;
static uint32 originalQuadCapacity;
uint32 hd_quad_deferred;
uint32 og_quad_builds;

static OriginalGlyph *originalGlyphs;
static uint32 originalGlyphCount;
static uint32 originalGlyphCapacity;
uint32 hd_glyph_deferred;
uint32 og_glyph_builds;

static FramePresentation *activeFrame;
static bool previousRequest;
static GpuRenderList *originalFrame;
static bool originalPending;
static bool originalPagePending;
static bool originalRegionValid;
static PSX_RECT originalSource;
static PSX_RECT originalOutput;
uint32 hd_frame_count;
uint32 og_frame_count;
uint32 hd_fallback_count;
uint32 hd_handoff_count;
float hd_frame_phase;
uint32 og_deferred_frames;
uint32 og_reconstructed_frames;
uint32 og_capture_failures;
uint32 og_reconstruction_failures;

FramePresentationHold::FramePresentationHold(void)
{
    active = xport_hd != 0 && !xport_is_headless();
    if (active)
        gpu_frame_begin();
}

FramePresentationHold::~FramePresentationHold(void)
{
    if (active)
        gpu_frame_end();
}

void FramePresentation::BeginOriginal(void)
{
    originalGlyphCount = originalQuadCount = 0;
}

bool FramePresentation::DeferGlyph(void *marker, uint32 position, uint32 texture, uint32 color, uint32 size)
{
    if (!WantsHd() || xport_is_headless() || (color >> 24 & 0xFE) != 0x66 || (sint16)size <= 0 || (sint16)(size >> 16) <= 0)
        return false;
    if (originalGlyphCount == originalGlyphCapacity)
    {
        uint32 capacity = originalGlyphCapacity == 0 ? 256 : originalGlyphCapacity * 2;
        OriginalGlyph *glyphs = (OriginalGlyph *)realloc(originalGlyphs, capacity * sizeof(*glyphs));
        if (glyphs == 0)
            return false;
        originalGlyphs = glyphs;
        originalGlyphCapacity = capacity;
    }
    if (!hd_ui_glyph(position, texture, color, size))
        return false;
    OriginalGlyph *glyph = originalGlyphs + originalGlyphCount++;
    glyph->marker = marker;
    glyph->position = position;
    glyph->texture = texture;
    glyph->color = color;
    glyph->size = size;
    ++hd_glyph_deferred;
    return true;
}

static SPRT original_glyph(const OriginalGlyph *source)
{
    SPRT sprite;
    sprite.tag = 0x04000000;
    memcpy(&sprite.rgbc, &source->color, sizeof(source->color));
    memcpy(&sprite.p[0].xy, &source->position, sizeof(source->position));
    sprite.p[0].uv_attr = source->texture;
    memcpy(&sprite.w, &source->size, sizeof(source->size));
    return sprite;
}

void FramePresentation::BuildOriginalGlyphs(void)
{
    for (uint32 i = 0; i < originalGlyphCount; ++i)
    {
        SPRT *marker = (SPRT *)originalGlyphs[i].marker;
        if ((marker->tag >> 24) != 0)
            continue;
        SPRT sprite = original_glyph(originalGlyphs + i);
        sprite.tag |= marker->tag & 0xFFFFFF;
        *marker = sprite;
        ++og_glyph_builds;
    }
}

static bool defer_original_shape(void *marker, const uint32 position[4], const uint32 color[4], uint8 code, const uint32 texture[4], bool ui = true)
{
    uint8 kind = code & 0xFC;
    if (!FramePresentation::WantsHd())
        return false;
    if (originalQuadCount == originalQuadCapacity)
    {
        uint32 capacity = originalQuadCapacity == 0 ? 64 : originalQuadCapacity * 2;
        OriginalQuad *quads = (OriginalQuad *)realloc(originalQuads, capacity * sizeof(*quads));
        if (quads == 0)
            return false;
        originalQuads = quads;
        originalQuadCapacity = capacity;
    }
    bool colored = kind == 0x20 || kind == 0x30 || kind == 0x40 || kind == 0x50;
    if (ui && !(colored ? hd_ui_colored(position, color, code) : hd_ui_quad(position, color, code, texture)))
        return false;
    OriginalQuad *quad = originalQuads + originalQuadCount++;
    quad->marker = marker;
    memcpy(quad->position, position, sizeof(quad->position));
    memcpy(quad->color, color, sizeof(quad->color));
    quad->code = code;
    if (texture != 0)
        memcpy(quad->texture, texture, sizeof(quad->texture));
    else
        memset(quad->texture, 0, sizeof(quad->texture));
    ++hd_quad_deferred;
    return true;
}

bool FramePresentation::DeferOriginalLine(void *marker, const uint32 position[4], const uint32 color[4], uint8 code)
{
    if ((code & 0xEC) != 0x40)
        return false;
    return defer_original_shape(marker, position, color, code, 0, false);
}

bool FramePresentation::DeferOriginalPolygon(void *marker, const uint32 position[4], const uint32 color[4], uint8 code, const uint32 texture[4])
{
    if ((code & 0xE0) != 0x20 || (((code & 4) != 0) != (texture != 0)))
        return false;
    return defer_original_shape(marker, position, color, code, texture, false);
}

bool FramePresentation::DeferQuad(void *marker, const uint32 position[4], const uint32 color[4], uint8 code, const uint32 texture[4])
{
    uint8 kind = code & 0xFC;
    if ((kind != 0x28 && kind != 0x38 && kind != 0x2C && kind != 0x3C) || (((code & 4) != 0) != (texture != 0)))
        return false;
    return defer_original_shape(marker, position, color, code, texture);
}

bool FramePresentation::DeferColored(void *marker, const uint32 position[4], const uint32 color[4], uint8 code)
{
    uint8 kind = code & 0xFC;
    if (kind != 0x20 && kind != 0x30 && kind != 0x40 && kind != 0x50)
        return false;
    return defer_original_shape(marker, position, color, code, 0);
}

bool FramePresentation::DeferFlat(void *marker, const uint32 position[4], uint32 color, uint8 code)
{
    if ((code & 0xFC) != 0x20 && (code & 0xFC) != 0x40)
        return false;
    uint32 colors[4] = {color, color, color, color};
    return DeferColored(marker, position, colors, code);
}

static size_t original_quad(const OriginalQuad *source, void *packet)
{
    if ((source->code & 0xFC) == 0x24)
    {
        POLY_FT3 triangle;
        triangle.tag = 0x07000000;
        memcpy(&triangle.rgbc, source->color, sizeof(uint32));
        for (sint32 i = 0; i < 3; ++i)
        {
            memcpy(&triangle.p[i].xy, source->position + i, sizeof(uint32));
            triangle.p[i].uv_attr = source->texture[i];
        }
        triangle.rgbc.code = source->code;
        memcpy(packet, &triangle, sizeof(triangle));
        return sizeof(triangle);
    }
    if ((source->code & 0xFC) == 0x34)
    {
        POLY_GT3 triangle;
        triangle.tag = 0x09000000;
        for (sint32 i = 0; i < 3; ++i)
        {
            memcpy(&triangle.p[i].rgbc, source->color + i, sizeof(uint32));
            memcpy(&triangle.p[i].xy, source->position + i, sizeof(uint32));
            triangle.p[i].uv_attr = source->texture[i];
        }
        triangle.p[0].rgbc.code = source->code;
        memcpy(packet, &triangle, sizeof(triangle));
        return sizeof(triangle);
    }
    if ((source->code & 0xFC) == 0x30)
    {
        POLY_G3 triangle;
        triangle.tag = 0x06000000;
        for (sint32 i = 0; i < 3; ++i)
        {
            memcpy(&triangle.p[i].rgbc, source->color + i, sizeof(uint32));
            memcpy(&triangle.p[i].xy, source->position + i, sizeof(uint32));
        }
        triangle.p[0].rgbc.code = source->code;
        memcpy(packet, &triangle, sizeof(triangle));
        return sizeof(triangle);
    }
    if ((source->code & 0xFC) == 0x50)
    {
        LINE_G2 line;
        line.tag = 0x04000000;
        for (sint32 i = 0; i < 2; ++i)
        {
            memcpy(&line.p[i].rgbc, source->color + i, sizeof(uint32));
            memcpy(&line.p[i].xy, source->position + i, sizeof(uint32));
        }
        line.p[0].rgbc.code = source->code;
        memcpy(packet, &line, sizeof(line));
        return sizeof(line);
    }
    if ((source->code & 0xFC) == 0x20)
    {
        POLY_F3 triangle;
        triangle.tag = 0x04000000;
        memcpy(&triangle.rgbc, source->color, sizeof(uint32));
        memcpy(triangle.p, source->position, sizeof(triangle.p));
        triangle.rgbc.code = source->code;
        memcpy(packet, &triangle, sizeof(triangle));
        return sizeof(triangle);
    }
    if ((source->code & 0xFC) == 0x40)
    {
        LINE_F2 line;
        line.tag = 0x03000000;
        memcpy(&line.rgbc, source->color, sizeof(uint32));
        memcpy(line.p, source->position, sizeof(line.p));
        line.rgbc.code = source->code;
        memcpy(packet, &line, sizeof(line));
        return sizeof(line);
    }
    if ((source->code & 4) != 0)
    {
        if ((source->code & 16) != 0)
        {
            POLY_GT4 quad;
            quad.tag = 0x0C000000;
            for (sint32 i = 0; i < 4; ++i)
            {
                memcpy(&quad.p[i].rgbc, source->color + i, sizeof(uint32));
                memcpy(&quad.p[i].xy, source->position + i, sizeof(uint32));
                quad.p[i].uv_attr = source->texture[i];
            }
            quad.p[0].rgbc.code = source->code;
            memcpy(packet, &quad, sizeof(quad));
            return sizeof(quad);
        }
        POLY_FT4 quad;
        quad.tag = 0x09000000;
        memcpy(&quad.rgbc, source->color, sizeof(uint32));
        for (sint32 i = 0; i < 4; ++i)
        {
            memcpy(&quad.p[i].xy, source->position + i, sizeof(uint32));
            quad.p[i].uv_attr = source->texture[i];
        }
        quad.rgbc.code = source->code;
        memcpy(packet, &quad, sizeof(quad));
        return sizeof(quad);
    }
    if ((source->code & 16) != 0)
    {
        POLY_G4 native;
        POLY_G4 *quad = &native;
        quad->tag = 0x08000000;
        for (sint32 i = 0; i < 4; ++i)
        {
            memcpy(&quad->p[i].xy, source->position + i, sizeof(uint32));
            memcpy(&quad->p[i].rgbc, source->color + i, sizeof(uint32));
        }
        quad->p[0].rgbc.code = source->code;
        memcpy(packet, quad, sizeof(*quad));
        return sizeof(*quad);
    }
    POLY_F4 native;
    POLY_F4 *quad = &native;
    quad->tag = 0x05000000;
    memcpy(&quad->rgbc, source->color, sizeof(uint32));
    for (sint32 i = 0; i < 4; ++i)
        memcpy(quad->p + i, source->position + i, sizeof(uint32));
    quad->rgbc.code = source->code;
    memcpy(packet, quad, sizeof(*quad));
    return sizeof(*quad);
}

void FramePresentation::BuildOriginalQuads(void)
{
    for (uint32 i = 0; i < originalQuadCount; ++i)
    {
        uint32 *marker = (uint32 *)originalQuads[i].marker;
        if ((*marker >> 24) != 0)
            continue;
        uint32 link = *marker;
        original_quad(originalQuads + i, marker);
        *marker |= link & 0xFFFFFF;
        ++og_quad_builds;
    }
}

static bool register_original_draws(GpuRenderList *list)
{
    for (uint32 i = 0; i < originalGlyphCount; ++i)
        if (!gpu_render_list_defer_draw(list, originalGlyphs[i].marker, originalGlyphs + i, sizeof(*originalGlyphs)))
            return false;
    for (uint32 i = 0; i < originalQuadCount; ++i)
    {
        const OriginalQuad *source = originalQuads + i;
        bool captured = (source->code & 4) != 0 ? gpu_render_list_defer_polygon(list, source->marker, (uint16)(source->texture[1] >> 16), source, sizeof(*source)) != 0 : gpu_render_list_defer_draw(list, source->marker, source, sizeof(*source)) != 0;
        if (!captured)
            return false;
    }
    return true;
}

static bool resolve_original_draws(GpuRenderList *list)
{
    for (uint32 i = 0; i < gpu_render_list_deferred_count(list); ++i)
    {
        size_t bytes;
        const OriginalGlyph *source = (const OriginalGlyph *)gpu_render_list_deferred_source(list, i, &bytes);
        if (source == 0)
            continue;
        if (bytes == sizeof(OriginalQuad))
        {
            POLY_GT4 packet;
            const OriginalQuad *quad = (const OriginalQuad *)source;
            size_t length = original_quad(quad, &packet);
            bool resolved = (quad->code & 4) != 0 ? gpu_render_list_resolve_polygon(list, i, &packet, length) != 0 : gpu_render_list_resolve_draw(list, i, &packet, length) != 0;
            if (!resolved)
                return false;
            ++og_quad_builds;
            continue;
        }
        if (bytes != sizeof(*source))
            return false;
        SPRT sprite = original_glyph(source);
        if (!gpu_render_list_resolve_draw(list, i, &sprite, sizeof(sprite)))
            return false;
        ++og_glyph_builds;
    }
    return true;
}

bool FramePresentation::RestoreOriginal(void)
{
    gpu_set_raster(1);
    if (originalPending && !resolve_original_draws(originalFrame))
    {
        ++og_reconstruction_failures;
        return false;
    }
    if (Render::DeferredWorldViews() != 0)
    {
        if (!Render::RestoreWorld())
        {
            ++og_reconstruction_failures;
            return false;
        }
        originalPagePending = true;
    }
    if (originalPending)
    {
        if (!gpu_render_list_replay(originalFrame))
        {
            ++og_reconstruction_failures;
            return false;
        }
        originalPending = false;
        originalPagePending = true;
    }
    if (originalPagePending)
    {
        if (!originalRegionValid || ((originalSource.x != originalOutput.x || originalSource.y != originalOutput.y) && MoveImage(&originalSource, originalOutput.x, originalOutput.y) != 0))
        {
            ++og_reconstruction_failures;
            return false;
        }
        originalPagePending = false;
        ++og_reconstructed_frames;
    }
    if (!Render::RestoreOriginalObservation())
    {
        ++og_reconstruction_failures;
        return false;
    }
    return true;
}

void FramePresentation::ReleaseOriginal(void)
{
    RestoreOriginal();
    gpu_render_list_free(originalFrame);
    originalFrame = 0;
    originalPending = false;
    originalPagePending = false;
    originalRegionValid = false;
    free(originalGlyphs);
    originalGlyphs = 0;
    originalGlyphCount = originalGlyphCapacity = 0;
    free(originalQuads);
    originalQuads = 0;
    originalQuadCount = originalQuadCapacity = 0;
}

void FramePresentation::SubmitOriginal(sint32 frame, sint32 view, sint32 environmentFrame)
{
    sint32 first = view < 0 ? 0 : view;
    sint32 last = view < 0 ? Draw::gNumView : view + 1;
    sint32 root = view < 0 ? Draw::gPlayer1View : view;
    if (environmentFrame < 0)
        environmentFrame = frame;
    uint32 worldViews = Render::DeferredWorldViews();
    bool missingHistory = originalPending || originalPagePending || !gpu_get_raster();
    bool deferred = WantsHd() && !xport_is_headless() && (worldViews != 0 || hd_ui_frontend_ready() || hd_world_raster_ready() || hd_ui_pause_ready());
    PSX_RECT source;
    DISPENV output;
    originalRegionValid = gpu_get_display_env(&output) != 0 && output.isrgb24 == 0;
    if (originalRegionValid)
    {
        source = Draw::gView[root].drawenv[environmentFrame].clip;
        source.w = output.disp.w;
        source.h = output.disp.h;
        originalRegionValid = source.w > 0 && source.h > 0 && source.x >= 0 && source.y >= 0 && source.x + source.w <= GPU_VRAM_WIDTH && source.y + source.h <= GPU_VRAM_HEIGHT && output.disp.x >= 0 && output.disp.y >= 0 && output.disp.x + source.w <= GPU_VRAM_WIDTH && output.disp.y + source.h <= GPU_VRAM_HEIGHT;
    }
    if (originalRegionValid)
    {
        originalSource = source;
        originalOutput = output.disp;
    }
    deferred = deferred && originalRegionValid;
    originalPending = false;
    originalPagePending = missingHistory;
    if (worldViews != 0)
    {
        gpu_set_raster(0);
        for (sint32 i = first; i < last; ++i)
            if ((worldViews & (1 << i)) != 0)
                DrawOTag(Draw::gView[i].ot[frame] + Draw::gView[i].otsize - 1);
    }
    if (deferred)
    {
        if (originalFrame == 0)
            originalFrame = gpu_render_list_create();
        DRAWENV environment = Draw::gView[first].drawenv[environmentFrame];
        if (worldViews != 0)
            environment.isbg = 0;
        gpu_set_raster(0);
        deferred = gpu_render_list_begin(originalFrame, &environment) != 0;
        if (deferred && !register_original_draws(originalFrame))
        {
            gpu_render_list_end(originalFrame);
            deferred = false;
        }
        if (!deferred)
            ++og_capture_failures;
    }
    gpu_set_raster(!deferred);
    if (!deferred)
    {
        BuildOriginalGlyphs();
        BuildOriginalQuads();
    }
    if (!deferred && worldViews != 0)
    {
        Render::RestoreWorld();
        originalPagePending = true;
    }
    for (sint32 i = first; i < last; ++i)
        if ((worldViews & (1 << i)) == 0)
            DrawOTag(Draw::gView[i].ot[frame] + Draw::gView[i].otsize - 1);
    if (!deferred)
        return;
    if (gpu_render_list_end(originalFrame))
    {
        originalPending = true;
        originalPagePending = false;
        ++og_deferred_frames;
        return;
    }
    ++og_capture_failures;
    BuildOriginalGlyphs();
    BuildOriginalQuads();
    originalPagePending = true;
    gpu_set_raster(1);
    if (worldViews != 0)
        Render::RestoreWorld();
    for (sint32 i = first; i < last; ++i)
        if ((worldViews & (1 << i)) == 0)
            DrawOTag(Draw::gView[i].ot[frame] + Draw::gView[i].otsize - 1);
}

FramePresentation::FramePresentation(void)
{
    requestedHd = changed = false;
    phase = 1;
    owner = activeFrame == 0;
    if (owner)
    {
        activeFrame = this;
        gpu_frame_begin();
        xport_poll();
        requestedHd = xport_hd != 0;
        if (!requestedHd)
            RestoreOriginal();
        changed = requestedHd != previousRequest;
        if (changed)
        {
            hd_reset_history();
            ++hd_handoff_count;
        }
        previousRequest = requestedHd;
    }
}

FramePresentation::~FramePresentation(void)
{
    Finish();
}

void FramePresentation::Finish(void)
{
    if (!owner)
        return;
    owner = false;
    gpu_frame_end();
    activeFrame = 0;
    if (xport_is_headless())
        return;
    sint32 width;
    sint32 height;
    vid_resize(&width, &height);
    if (width <= 0 || height <= 0)
        return;
    bool hdPresented = requestedHd && hd_present(phase);
    bool presented = hdPresented;
    if (presented)
    {
        ++hd_frame_count;
        hd_frame_phase = phase;
    }
    else
    {
        if (requestedHd)
            ++hd_fallback_count;
        presented = RestoreOriginal() && gpu_present() != 0;
        if (presented)
            hd_og_presented();
        if (presented)
            ++og_frame_count;
    }
#if defined(AP_WEB)
    if (presented && !hdPresented)
#else
    if (presented)
#endif
        xport_present_complete();
}

bool FramePresentation::Handoff(void)
{
    return activeFrame != 0 && activeFrame->changed;
}

void FramePresentation::SetPhase(float value)
{
    if (activeFrame != 0)
        activeFrame->phase = value < 0 ? 0 : value > 1 ? 1 : value;
}

bool FramePresentation::Active(void)
{
    return activeFrame != 0;
}

bool FramePresentation::WantsHd(void)
{
    return activeFrame != 0 && activeFrame->requestedHd && !xport_is_headless();
}

void FramePresentation::FinishActive(void)
{
    if (activeFrame != 0)
        activeFrame->Finish();
}
