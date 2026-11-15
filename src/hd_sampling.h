#ifndef HD_SAMPLING_H
#define HD_SAMPLING_H

#include "vid_psx_sampling.h"
#include "game/psx/draw.h"

struct HdTextureRegion
{
    uint16 x;
    uint16 y;
    uint16 width;
    uint16 height;
};

enum HdTextureCategory
{
    HD_FILTER_WORLD = 1,
    HD_FILTER_CAR = 2,
    HD_FILTER_SKY = 4,
    HD_FILTER_FRONTEND_CAR = 8,
    HD_FILTER_BACKDROP = 16,
    HD_FILTER_SMOKE = 32,
    HD_FILTER_ALL = 63
};

extern uint32 hd_texture_filtering_enabled;

void hd_texture_loaded(const Draw::tPixMap *pixmap, sint32 x, sint32 y, sint32 width, sint32 height, sint32 storageX, sint32 storageY, sint32 storageDepth);
void hd_texture_alias(const Draw::tPixMap *source, const Draw::tPixMap *destination);
bool hd_texture_region(const Draw::tPixMap *pixmap, HdTextureRegion *region, bool respectPolicy = true);
void hd_texture_regions_free(void);

#define HD_VRAM_WORD_GLSL "uint vidPsxWord(ivec2 address){uvec4 v=uvec4(round(texelFetch(uTex,address,0)*vec4(31.0,31.0,31.0,1.0)));return (v.r<<11)|(v.g<<6)|(v.b<<1)|v.a;}\n"

#define HD_BOUNDED_WINDOW_GLSL \
    "ivec2 hdWindowAddress(ivec2 local,ivec4 region,uint repeatAxes){uvec2 p=uvec2(local+region.xy);" \
    "p=(p&(~vWindow))|(vWindowOrigin&vWindow);return clamp(ivec2(p&uvec2(255u)),region.xy,region.xy+region.zw-1);}\n" \
    "#define VID_PSX_ADDRESS hdWindowAddress\n"

#define HD_PSX_COLOR_GLSL \
    "vec4 hdPsxColor(vec2 uv,uint page,uint clut,ivec4 region,uint stp,uint word){" \
    "vec4 color=vidPsxColor(word,stp,false);if(color.a>0.0&&region.z>0&&region.w>0){" \
    "vec4 filtered=vidPsxFilter(uv-vec2(region.xy),page,clut,region,0u,stp,false);" \
    "if(filtered.a>0.0)color=filtered;}return color;}\n"

#endif
