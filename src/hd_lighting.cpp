#include <math.h>
#include <string.h>
#include "xport.h"
#include "psx.h"
#include "vid.h"
#include "hd_lighting.h"

static VID_TARGET *lightingDepth;
static VID_TEXTURE *lightingData;
static HdLightingProjection projections[HD_LIGHTING_LAYERS];
static float records[2][HD_LIGHTING_RECORDS][4];
static bool lightingReady;
static sint32 lightingViews = 1;
uint32 hd_lighting_captures;
uint32 hd_lighting_caster_draws;
float hd_headlight_intensity = 24;
float hd_shadow_strength = 0.475f;
float hd_shadow_vertical_bias[HD_LIGHTING_TRACKS] = {0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f};
float hd_sun_height_bias[HD_LIGHTING_TRACKS] = {0, 0, 0, 0, 0, 0, 0.1f};

static float hd_lighting_dot(const VEC3F *a, const VEC3F *b)
{
    return a->x * b->x + a->y * b->y + a->z * b->z;
}

static bool hd_lighting_normalize(VEC3F *vector)
{
    float length = sqrtf(hd_lighting_dot(vector, vector));
    if (!(length > 0.000001f) || !(length < 1e20f))
        return false;
    for (sint32 axis = 0; axis < 3; ++axis)
        vector->v[axis] /= length;
    return true;
}

VEC3F hd_lighting_shadow_direction(const VEC3F *sun, sint32 track)
{
    VEC3F direction = *sun;
    if (!hd_lighting_normalize(&direction))
    {
        direction.x = direction.z = 0;
        direction.y = 1;
        return direction;
    }
    if (track >= 0 && track < HD_LIGHTING_TRACKS)
        direction.y += hd_shadow_vertical_bias[track];
    if (!hd_lighting_normalize(&direction))
    {
        direction.x = direction.z = 0;
        direction.y = 1;
    }
    return direction;
}

void hd_lighting_raise_sun(SVECTOR *sun, sint32 track)
{
    if (track < 0 || track >= HD_LIGHTING_TRACKS)
        return;
    float radius = sqrtf((float)sun->vx * sun->vx + (float)sun->vz * sun->vz);
    float height = sun->vy + radius * hd_sun_height_bias[track];
    if (height < -32768)
        height = -32768;
    if (height > 32767)
        height = 32767;
    sun->vy = (sint16)height;
}

static VEC3F hd_lighting_cross(const VEC3F *a, const VEC3F *b)
{
    VEC3F result = {a->y * b->z - a->z * b->y, a->z * b->x - a->x * b->z, a->x * b->y - a->y * b->x};
    return result;
}

static bool hd_lighting_basis(const VEC3F *forward, VEC3F basis[3])
{
    basis[2] = *forward;
    if (!hd_lighting_normalize(basis + 2))
        return false;
    VEC3F up = {0, 1, 0};
    if (fabsf(basis[2].y) > 0.99f)
    {
        up.y = 0;
        up.z = 1;
    }
    basis[0] = hd_lighting_cross(&up, basis + 2);
    hd_lighting_normalize(basis);
    basis[1] = hd_lighting_cross(basis + 2, basis);
    return true;
}

void hd_lighting_orthographic(HdLightingProjection *projection, const VEC3F *center, const VEC3F *sun, float radius, bool snap, const VEC3F *right)
{
    projection->active = false;
    VEC3F direction = {-sun->x, -sun->y, -sun->z};
    VEC3F basis[3];
    if (!(radius > 0) || !hd_lighting_basis(&direction, basis))
        return;
    if (right != 0)
    {
        float along = hd_lighting_dot(right, basis + 2);
        for (sint32 axis = 0; axis < 3; ++axis)
            basis[0].v[axis] = right->v[axis] - along * basis[2].v[axis];
        if (!hd_lighting_normalize(basis))
            return;
        basis[1] = hd_lighting_cross(basis + 2, basis);
    }
    memset(projection->rows, 0, sizeof(projection->rows));
    for (sint32 row = 0; row < 3; ++row)
    {
        float extent = row == 2 ? radius * 4 : radius;
        float offset = hd_lighting_dot(basis + row, center);
        if (snap && row < 2)
        {
            float texel = radius * 2 / HD_LIGHTING_MAP_SIZE;
            offset = floorf(offset / texel + 0.5f) * texel;
        }
        for (sint32 axis = 0; axis < 3; ++axis)
            projection->rows[row][axis] = basis[row].v[axis] / extent;
        projection->rows[row][3] = -offset / extent;
    }
    projection->rows[3][3] = 1;
    projection->center = *center;
    projection->radius = radius * 5;
    projection->owner = -1;
    projection->active = true;
}

bool hd_lighting_spot(HdLightingProjection *projection, const HdLightingLamp *lamp)
{
    projection->active = false;
    VEC3F basis[3];
    const float nearPlane = 4;
    if (!lamp->active || !(lamp->range > nearPlane) || !hd_lighting_basis(&lamp->direction, basis))
        return false;
    memset(projection->rows, 0, sizeof(projection->rows));
    for (sint32 row = 0; row < 4; ++row)
    {
        sint32 source = row < 2 ? row : 2;
        float scale = row == 0 ? 1.5f : row == 1 ? 2.5f : row == 2 ? (lamp->range + nearPlane) / (lamp->range - nearPlane) : 1;
        for (sint32 axis = 0; axis < 3; ++axis)
            projection->rows[row][axis] = basis[source].v[axis] * scale;
        projection->rows[row][3] = -hd_lighting_dot(basis + source, &lamp->position) * scale;
    }
    projection->rows[2][3] -= 2 * lamp->range * nearPlane / (lamp->range - nearPlane);
    projection->center = lamp->position;
    projection->radius = lamp->range;
    projection->owner = lamp->owner;
    projection->active = true;
    return true;
}

bool hd_lighting_resources(void)
{
    if (lightingDepth == 0)
        lightingDepth = vid_target_create_depth_array(HD_LIGHTING_MAP_SIZE, HD_LIGHTING_LAYERS);
    if (lightingData == 0)
        lightingData = vid_tex_create_fmt(HD_LIGHTING_RECORDS, 2, VID_RGBA32F);
    return lightingDepth != 0 && lightingData != 0;
}

void hd_lighting_free(void)
{
    vid_target_free(lightingDepth);
    vid_tex_free(lightingData);
    lightingDepth = 0;
    lightingData = 0;
    lightingReady = false;
}

void hd_lighting_frame(sint32 views)
{
    lightingViews = views == 2 ? 2 : 1;
    memset(projections, 0, sizeof(projections));
    memset(records, 0, sizeof(records));
    for (uint32 layer = 0; layer < HD_LIGHTING_LAYERS; ++layer)
        projections[layer].layer = layer;
    lightingReady = false;
    hd_lighting_captures = 0;
    hd_lighting_caster_draws = 0;
}

void hd_lighting_sun(sint32 view, const MATRIXF *camera, const VEC3F *sun, bool enabled, float distance, float sunWeight)
{
    if (view < 0 || view >= 2 || !enabled)
        return;
    if (!(sunWeight >= 0))
        sunWeight = 0;
    if (sunWeight > 1)
        sunWeight = 1;
    VEC3F right = {-sun->z, 0, sun->x};
    if (!hd_lighting_normalize(&right))
        right.x = 1;
    VEC3F direction = {sun->x * sunWeight, sun->y * sunWeight + 1 - sunWeight, sun->z * sunWeight};
    if (!hd_lighting_normalize(&direction))
    {
        direction.x = direction.z = 0;
        direction.y = 1;
    }
    const float radii[HD_LIGHTING_SUN_CASCADES] = {512, 4096, distance > 16384 ? distance : 16384};
    for (sint32 cascade = 0; cascade < HD_LIGHTING_SUN_CASCADES; ++cascade)
    {
        VEC3F center;
        for (sint32 axis = 0; axis < 3; ++axis)
            center.v[axis] = camera->t[axis] + (cascade == HD_LIGHTING_SUN_CASCADES - 1 ? 0 : camera->m[axis][2] * radii[cascade] * 0.6f);
        HdLightingProjection *projection = projections + view * HD_LIGHTING_SUN_CASCADES + cascade;
        hd_lighting_orthographic(projection, &center, &direction, radii[cascade], cascade != 0, &right);
        memcpy(records[view] + cascade * 4, projection->rows, sizeof(projection->rows));
    }
    records[view][12][0] = projections[view * HD_LIGHTING_SUN_CASCADES].active ? hd_shadow_strength : 0;
    records[view][12][1] = (float)(view * HD_LIGHTING_SUN_CASCADES);
    records[view][12][2] = (float)((view + 1) * HD_LIGHTING_SUN_CASCADES - 1);
}

void hd_lighting_lamp(sint32 lamp, const HdLightingLamp *source)
{
    if (lamp < 0 || lamp >= HD_LIGHTING_LAMPS)
        return;
    HdLightingProjection *projection = projections + HD_LIGHTING_HEAD_LAYER + lamp;
    if (!hd_lighting_spot(projection, source))
        return;
    for (sint32 view = 0; view < lightingViews; ++view)
    {
        sint32 record = (sint32)records[view][12][3];
        if (record >= HD_LIGHTING_LAMPS)
            continue;
        records[view][12][3] = (float)(record + 1);
        float(*destination)[4] = records[view] + 16 + record * 8;
        memcpy(destination, projection->rows, sizeof(projection->rows));
        memcpy(destination[4], &source->position, 3 * sizeof(float));
        destination[4][3] = source->range;
        VEC3F direction = source->direction;
        hd_lighting_normalize(&direction);
        memcpy(destination[5], &direction, 3 * sizeof(float));
        destination[5][3] = 1;
        memcpy(destination[6], source->color, 3 * sizeof(float));
        destination[6][3] = (float)projection->layer;
    }
}

bool hd_lighting_capture(HdLightingCaster draw, void *context)
{
    if (!hd_lighting_resources() || draw == 0)
        return false;
    for (uint32 layer = 0; layer < HD_LIGHTING_LAYERS; ++layer)
    {
        const HdLightingProjection *projection = projections + layer;
        if (!projection->active)
            continue;
        if (!vid_begin(0, 0, 0))
            return false;
        if (!vid_target_select_depth_layer(lightingDepth, layer))
        {
            vid_cancel();
            return false;
        }
        VID_RECT viewport = {0, 0, HD_LIGHTING_MAP_SIZE, HD_LIGHTING_MAP_SIZE};
        vid_viewport(viewport);
        if (projection->active && !draw(context, projection))
        {
            vid_cancel();
            return false;
        }
        if (!vid_end())
            return false;
        ++hd_lighting_captures;
    }
    memcpy(vid_tex_data(lightingData), records, sizeof(records));
    vid_tex_commit(lightingData);
    lightingReady = true;
    return true;
}

bool hd_lighting_ready(void)
{
    return lightingReady;
}

VID_TEXTURE *hd_lighting_depth(void)
{
    return lightingDepth != 0 ? vid_target_texture(lightingDepth) : 0;
}

VID_TEXTURE *hd_lighting_data(void)
{
    return lightingData;
}

void hd_lighting_uniforms(float uniforms[26][4], const MATRIXF *model, sint32 view, bool enabled)
{
    for (sint32 row = 0; row < 3; ++row)
    {
        memcpy(uniforms[18 + row], model->m[row], 3 * sizeof(float));
        uniforms[18 + row][3] = model->t[row];
    }
    uniforms[21][0] = enabled && lightingReady ? 1.0f : 0.0f;
    uniforms[21][1] = (float)view;
}
