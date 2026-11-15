#include <stdlib.h>
#include <string.h>
#include "xport.h"
#include "hd.h"
#include "hd_sampling.h"

struct HdTextureAllocation
{
    const Draw::tPixMap *source;
    HdTextureRegion region;
    uint16 page;
    uint16 corners[4];
    sint32 bounds[4];
};

static HdTextureAllocation *textureAllocations;
static uint32 textureAllocationCount;
static uint32 textureAllocationCapacity;
uint32 hd_texture_filtering_enabled = HD_FILTER_ALL;

static void hd_texture_corners(const Draw::tPixMap *pixmap, uint16 corners[4])
{
    corners[0] = pixmap->u0 | (pixmap->v0 << 8);
    corners[1] = pixmap->u1 | (pixmap->v1 << 8);
    corners[2] = pixmap->u2 | (pixmap->v2 << 8);
    corners[3] = pixmap->u3 | (pixmap->v3 << 8);
    for (sint32 i = 1; i < 4; ++i)
        for (sint32 j = i; j > 0 && corners[j] < corners[j - 1]; --j)
        {
            uint16 value = corners[j];
            corners[j] = corners[j - 1];
            corners[j - 1] = value;
        }
}

static void hd_texture_store(const Draw::tPixMap *pixmap, const HdTextureRegion *region, const sint32 bounds[4])
{
    for (uint32 index = 0; index < textureAllocationCount;)
        if (textureAllocations[index].source == pixmap)
            textureAllocations[index] = textureAllocations[--textureAllocationCount];
        else
            ++index;
    if (textureAllocationCount == textureAllocationCapacity)
    {
        uint32 capacity = textureAllocationCapacity != 0 ? textureAllocationCapacity * 2 : 128;
        if (capacity < textureAllocationCapacity || capacity > 0xffffffff / sizeof(*textureAllocations))
            return;
        HdTextureAllocation *allocations = (HdTextureAllocation *)realloc(textureAllocations, capacity * sizeof(*textureAllocations));
        if (allocations == 0)
            return;
        textureAllocations = allocations;
        textureAllocationCapacity = capacity;
    }
    HdTextureAllocation *allocation = textureAllocations + textureAllocationCount++;
    allocation->source = pixmap;
    allocation->region = *region;
    allocation->page = pixmap->tpage & 0x19f;
    hd_texture_corners(pixmap, allocation->corners);
    memcpy(allocation->bounds, bounds, sizeof(allocation->bounds));
}

void hd_texture_loaded(const Draw::tPixMap *pixmap, sint32 x, sint32 y, sint32 width, sint32 height, sint32 storageX, sint32 storageY, sint32 storageDepth)
{
    hd_register_cleanup();
    if (pixmap == 0 || storageDepth < 0 || storageDepth > 2 || storageX < 0 || storageY < 0 || width <= 0 || height <= 0 || storageX >= 1024 || storageY >= 512)
    {
        textureAllocationCount = 0;
        return;
    }
    sint32 pixelsPerWord = 4 >> storageDepth;
    if (width > (1024 - storageX) * pixelsPerWord || height > 512 - storageY)
    {
        textureAllocationCount = 0;
        return;
    }
    sint32 bounds[4] = {storageX, storageY, storageX + (width + pixelsPerWord - 1) / pixelsPerWord, storageY + height};
    for (uint32 index = 0; index < textureAllocationCount;)
    {
        const sint32 *previous = textureAllocations[index].bounds;
        if (textureAllocations[index].source == pixmap || (bounds[0] < previous[2] && bounds[2] > previous[0] && bounds[1] < previous[3] && bounds[3] > previous[1]))
            textureAllocations[index] = textureAllocations[--textureAllocationCount];
        else
            ++index;
    }
    if (((pixmap->tpage >> 7) & 3) > 2 || x < 0 || y < 0 || x > 255 || y > 255 || width > 256 - x || height > 256 - y)
        return;
    HdTextureRegion region = {(uint16)x, (uint16)y, (uint16)width, (uint16)height};
    hd_texture_store(pixmap, &region, bounds);
}

static sint32 hd_texture_find(const Draw::tPixMap *pixmap)
{
    if (pixmap == 0)
        return -1;
    uint16 page = pixmap->tpage & 0x19f;
    for (uint32 index = 0; index < textureAllocationCount; ++index)
        if (textureAllocations[index].source == pixmap && textureAllocations[index].page == page)
        {
            return (sint32)index;
        }
    uint16 corners[4];
    hd_texture_corners(pixmap, corners);
    sint32 found = -1;
    for (uint32 index = 0; index < textureAllocationCount; ++index)
    {
        const HdTextureAllocation *allocation = textureAllocations + index;
        if (allocation->page != page || memcmp(allocation->corners, corners, sizeof(corners)) != 0)
            continue;
        if (found >= 0 && memcmp(&textureAllocations[found].region, &allocation->region, sizeof(allocation->region)) != 0)
            return -1;
        found = (sint32)index;
    }
    if (found >= 0)
        return found;
    sint32 minU = 255, minV = 255, maxU = 0, maxV = 0;
    for (sint32 corner = 0; corner < 4; ++corner)
    {
        sint32 u = corners[corner] & 255, v = corners[corner] >> 8;
        if (u < minU)
            minU = u;
        if (u > maxU)
            maxU = u;
        if (v < minV)
            minV = v;
        if (v > maxV)
            maxV = v;
    }
    if (minU == maxU || minV == maxV)
        return -1;
    for (uint32 index = 0; index < textureAllocationCount; ++index)
    {
        const HdTextureAllocation *allocation = textureAllocations + index;
        const HdTextureRegion *region = &allocation->region;
        if (allocation->page != page || minU < region->x || minV < region->y || maxU >= region->x + region->width || maxV >= region->y + region->height)
            continue;
        if (found >= 0 && memcmp(&textureAllocations[found].region, region, sizeof(*region)) != 0)
            return -1;
        found = (sint32)index;
    }
    return found;
}

bool hd_texture_region(const Draw::tPixMap *pixmap, HdTextureRegion *region, bool respectPolicy)
{
    memset(region, 0, sizeof(*region));
    if (respectPolicy && hd_texture_filtering_enabled == 0)
        return false;
    sint32 index = hd_texture_find(pixmap);
    if (index < 0)
        return false;
    *region = textureAllocations[index].region;
    return true;
}

void hd_texture_alias(const Draw::tPixMap *source, const Draw::tPixMap *destination)
{
    sint32 index = hd_texture_find(source);
    if (index >= 0)
    {
        HdTextureAllocation allocation = textureAllocations[index];
        hd_texture_store(destination, &allocation.region, allocation.bounds);
    }
}

void hd_texture_regions_free(void)
{
    free(textureAllocations);
    textureAllocations = 0;
    textureAllocationCount = textureAllocationCapacity = 0;
}
