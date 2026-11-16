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
#include "game/common/carobj.h"
#include "game/common/frame.h"
#include "game/common/r3dcar.h"
#include "game/common/transformer_obj.h"
#include "game/psx/cario.h"
#include "game/psx/drawc.h"

struct HdFrontendGeometry
{
    const Transformer::zObj *source;
    VID_VERTEX16 *vertices;
    uint32 count;
    VID_GEOMETRY *mesh;
    bool valid;
    bool halo;
};

struct HdFrontendModel
{
    const Car::tObj *car;
    sint32 type;
    uint8 country;
    const Transformer::zScene *scene;
    HdFrontendGeometry parts[57];
};

struct HdFrontendPass
{
    float matrices[57][3][4];
    float normal[3][4];
    uint16 metadata[256][4];
    uint8 visible[57];
    uint8 envmap[57];
    uint8 mirror[57];
    sint16 sortOffsets[57];
    sint32 player;
    float x;
    float y;
    uint32 color;
    uint32 environment[2];
    bool valid;
    bool reflected;
};

struct HdFrontendDraw
{
    uint32 first;
    sint32 part;
    sint32 bucket;
};

struct HdFrontendHaloFrame
{
    sint32 types[57];
    sint32 overlays[57];
    sint32 sortOffsets[57];
    bool active[57];
};

struct HdFrontendBackdrop
{
    float camera[3][4];
    float shadow[4][3];
    COORD16 spot[33];
    COORD16 ring[64];
    Draw::tPixMap shadowMap;
    Draw::tPixMap ringMap;
    uint32 shadowColor;
    uint32 spotColor;
    uint32 ringColors[32];
    bool shadowActive;
    bool spotActive;
    bool ringActive;
};

struct HdFrontendBackdropMesh
{
    VID_GEOMETRY *mesh;
    uint8 source[sizeof(COORD16) * 64];
};

struct HdFrontendMotion
{
    HdRenderTransform parts[57];
    HdRenderTransform normal;
    HdRenderTransform camera;
};

static HdFrontendModel menuModels[2];
static HdFrontendDraw *menuDraws;
static uint32 menuDrawCapacity;
static HdFrontendPass menuBuild[4];
static HdFrontendPass menuComplete[4];
static HdFrontendPass menuPrevious[4];
static HdFrontendPass menuRender[4];
static HdFrontendMotion motionComplete[4];
static HdFrontendMotion motionPrevious[4];
static HdFrontendHaloFrame haloBuild[4];
static HdFrontendHaloFrame haloComplete[4];
static VID_GEOMETRY *menuHaloMeshes[4];
static uint32 menuHaloCapacities[4];
static VID_SHADER *menuHaloShader;
static VID_MATERIAL *menuHaloMaterial;
static HdFrontendBackdrop backdropBuild[4];
static HdFrontendBackdrop backdropComplete[4];
static HdFrontendBackdrop backdropPrevious[4];
static HdFrontendBackdrop backdropRender[4];
static HdFrontendBackdropMesh backdropMeshes[4][3];
static uint32 menuBuildViews;
static uint32 menuCompleteViews;
static uint32 menuResetPlayers = 3;
static bool menuBuildOverflow;
static sint32 menuBuildCount;
static sint32 menuCompleteCount;
static sint32 menuPlayer = -1;
static sint32 menuPass = -1;
static sint32 menuX;
static sint32 menuY;
static VID_SHADER *menuShader;
static VID_TEXTURE *menuMetadata;
static VID_MATERIAL *menuMaterials[2][6];
static VID_SHADER *backdropShader;
static VID_MATERIAL *backdropMaterials[6];
static float menuMapping[4] = {1, 1, 0, 0};
static uint32 menuDrawnPlayers;
static uint32 menuRevision;
static uint32 menuUploadSampling;
static uint32 menuUploadRevision;
uint32 hd_frontend_environment_enabled = 1;
uint32 hd_frontend_reflections_enabled = 1;
uint32 hd_frontend_halos_enabled = 1;
uint32 hd_frontend_halo_faces;
uint32 hd_frontend_halo_vertices;
uint32 hd_frontend_shadow_enabled = 1;
uint32 hd_frontend_spot_enabled = 1;
uint32 hd_frontend_ring_enabled = 1;
uint32 hd_frontend_shadow_draws;
uint32 hd_frontend_spot_draws;
uint32 hd_frontend_ring_draws;
uint32 hd_frontend_passes;
uint32 hd_frontend_parts;
uint32 hd_frontend_triangles;
uint32 hd_frontend_invalid;
uint32 hd_frontend_preview_hash;
uint32 hd_frontend_preview_pixels;

void hd_frontend_sample(float phase)
{
    if (!(phase >= 0))
        phase = 0;
    if (phase > 1)
        phase = 1;
    memcpy(menuRender, menuComplete, sizeof(menuRender));
    memcpy(backdropRender, backdropComplete, sizeof(backdropRender));
    for (sint32 slot = 0; slot < menuCompleteCount; ++slot)
    {
        const HdFrontendPass *from = menuPrevious + slot;
        const HdFrontendPass *to = menuComplete + slot;
        for (sint32 part = 0; part < 57; ++part)
            if (from->visible[part] == to->visible[part] && from->mirror[part] == to->mirror[part])
                hd_mix_transform(from->matrices[part], to->matrices[part], &motionPrevious[slot].parts[part], &motionComplete[slot].parts[part], phase, menuRender[slot].matrices[part]);
        hd_mix_transform(from->normal, to->normal, &motionPrevious[slot].normal, &motionComplete[slot].normal, phase, menuRender[slot].normal);
        const HdFrontendBackdrop *oldBackdrop = backdropPrevious + slot;
        const HdFrontendBackdrop *backdrop = backdropComplete + slot;
        hd_mix_transform(oldBackdrop->camera, backdrop->camera, &motionPrevious[slot].camera, &motionComplete[slot].camera, phase, backdropRender[slot].camera);
        if (oldBackdrop->shadowActive && backdrop->shadowActive)
            for (sint32 corner = 0; corner < 4; ++corner)
                for (sint32 axis = 0; axis < 3; ++axis)
                    backdropRender[slot].shadow[corner][axis] = oldBackdrop->shadow[corner][axis] + (backdrop->shadow[corner][axis] - oldBackdrop->shadow[corner][axis]) * phase;
    }
}

void hd_frontend_reset_history(void)
{
    memcpy(menuPrevious, menuComplete, sizeof(menuPrevious));
    memcpy(backdropPrevious, backdropComplete, sizeof(backdropPrevious));
    memcpy(motionPrevious, motionComplete, sizeof(motionPrevious));
    menuResetPlayers = 3;
    hd_frontend_sample(1);
}

static void hd_frontend_free_model(HdFrontendModel *model)
{
    for (sint32 i = 0; i < 57; ++i)
    {
        vid_mesh_free(model->parts[i].mesh);
        free(model->parts[i].vertices);
    }
    memset(model, 0, sizeof(*model));
}

void hd_frontend_forget_car(Car::tObj *car)
{
    FramePresentation::RestoreOriginal();
    for (sint32 player = 0; player < 2; ++player)
        if (menuModels[player].car == car)
        {
            menuResetPlayers |= 1 << player;
            hd_frontend_free_model(menuModels + player);
            for (sint32 i = 0; i < menuBuildCount; ++i)
                if (menuBuild[i].player == player)
                    menuBuild[i].valid = false;
            for (sint32 i = 0; i < menuCompleteCount; ++i)
                if (menuComplete[i].player == player)
                    menuComplete[i].valid = false;
        }
}

void hd_frontend_invalidate(void)
{
    FramePresentation::RestoreOriginal();
    menuBuildCount = menuCompleteCount = 0;
    menuBuildViews = menuCompleteViews = 0;
    menuBuildOverflow = false;
    menuPlayer = menuPass = -1;
    hd_frontend_passes = 0;
    hd_frontend_reset_history();
}

void hd_frontend_begin_frame(void)
{
    menuBuildCount = 0;
    menuBuildViews = 0;
    menuBuildOverflow = false;
    menuPlayer = menuPass = -1;
    memset(backdropBuild, 0, sizeof(backdropBuild));
    memset(haloBuild, 0, sizeof(haloBuild));
}

void hd_frontend_begin(sint32 player, sint32 x, sint32 y)
{
    menuPlayer = player >= 0 && player < 2 ? player : -1;
    menuPass = -1;
    if (menuPlayer >= 0)
        menuBuildViews |= 1 << menuPlayer;
    menuX = x;
    menuY = y;
}

static bool hd_frontend_geometry(HdFrontendGeometry *part, const Transformer::zObj *object, bool halo = false)
{
    if (part->source == object && part->halo == halo && part->valid)
        return true;
    vid_mesh_free(part->mesh);
    free(part->vertices);
    memset(part, 0, sizeof(*part));
    part->source = object;
    part->halo = halo;
    if (object == 0 || object->numFacet == 0)
    {
        part->valid = true;
        return true;
    }
    if (object->vertex == 0 || object->facet == 0)
        return false;
    part->vertices = (VID_VERTEX16 *)calloc(object->numFacet * 3, sizeof(VID_VERTEX16));
    if (part->vertices == 0)
        return false;
    for (sint32 face = object->numFacet - 1; face >= 0; --face)
    {
        const Transformer::zFacet *facet = object->facet + face;
        const uint8 ids[3] = {facet->vertexId0, facet->vertexId1, facet->vertexId2};
        const Transformer::zUV uv[3] = {facet->uv0, facet->uv1, facet->uv2};
        if ((!halo && facet->textureIndex >= 51) || ids[0] >= object->numVertex || ids[1] >= object->numVertex || ids[2] >= object->numVertex)
            return false;
        for (sint32 corner = 0; corner < 3; ++corner)
        {
            VID_VERTEX16 *vertex = part->vertices + part->count++;
            const COORD16 *position = object->vertex + ids[corner];
            vertex->pos[0] = position->x;
            vertex->pos[1] = position->y;
            vertex->pos[2] = position->z;
            if (object->Nvertex != 0)
            {
                const COORD16 *normal = object->Nvertex + ids[corner];
                vertex->n[0] = normal->x;
                vertex->n[1] = normal->y;
                vertex->n[2] = normal->z;
            }
            vertex->uv[0] = uv[corner].u;
            vertex->uv[1] = uv[corner].v;
            vertex->tex[0] = facet->textureIndex;
            vertex->tex[1] = (uint16)facet->flag;
        }
    }
    part->valid = true;
    return true;
}

void hd_frontend_capture(Car::tObj *car, const Draw::CarCache *state)
{
    if (menuPlayer < 0 || car == 0 || state == 0)
        return;
    if (menuBuildCount >= 4)
    {
        menuPass = -1;
        menuBuildOverflow = true;
        ++hd_frontend_invalid;
        return;
    }
    menuPass = menuBuildCount++;
    HdFrontendPass *pass = menuBuild + menuPass;
    memset(pass, 0, sizeof(*pass));
    pass->player = menuPlayer;
    pass->reflected = (R3DCar::InMenu & 0x80) != 0;
    pass->x = (float)menuX;
    pass->y = (float)menuY;
    sint32 type = car->render.currentCarType;
    sint32 country = (uint8)car->render.currentCountry >> 7;
    if (type < 0 || type >= 50 || R3DCar::LoadedScenePointer[country][type] == 0)
        return;
    HdFrontendModel *model = menuModels + menuPlayer;
    const Transformer::zScene *scene = R3DCar::LoadedScenePointer[country][type];
    if (model->car != car || model->type != type || model->country != (uint8)car->render.currentCountry || model->scene != scene)
    {
        menuResetPlayers |= 1 << menuPlayer;
        hd_frontend_free_model(model);
        model->scene = scene;
        model->car = car;
        model->type = type;
        model->country = (uint8)car->render.currentCountry;
    }
    sint32 available = CarIO::carPixMapCount - car->render.textureStartIndex;
    if (car->render.textureStartIndex < 0 || available <= 0 || state->pmxStart == 0)
        return;
    if (available > 51)
        available = 51;
    for (sint32 i = 0; i < available; ++i)
    {
        pass->metadata[i][0] = state->pmxStart[i].clut;
        pass->metadata[i][1] = state->pmxStart[i].tpage;
        pass->metadata[i][2] = state->pmxStart[i].flag;
        HdTextureRegion region;
        hd_texture_region(state->pmxStart + i, &region, false);
        memcpy(pass->metadata[198 + i], &region, sizeof(region));
        pass->metadata[141 + i][0] = (uint16)DrawC::gOverlay[i];
    }
    for (sint32 i = 0; i < 90; ++i)
    {
        pass->metadata[51 + i][0] = (uint16)scene->overlay[i].offset;
        pass->metadata[51 + i][1] = scene->overlay[i].u;
        pass->metadata[51 + i][2] = scene->overlay[i].v;
    }
    pass->metadata[192][0] = state->offsetU0;
    pass->metadata[192][1] = state->offsetV0;
    pass->metadata[192][2] = (uint16)available;
    pass->metadata[194][0] = state->offsetU1;
    pass->metadata[194][1] = state->offsetV1;
    pass->metadata[194][2] = state->offsetU2;
    pass->metadata[194][3] = state->offsetV2;
    pass->metadata[193][0] = state->ePmx0.clut;
    pass->metadata[193][1] = state->ePmx0.tpage;
    pass->metadata[193][2] = state->ePmx0.u0;
    pass->metadata[193][3] = state->ePmx0.v0;
    HdTextureRegion environmentRegion = {0};
    hd_texture_region(&state->ePmx0, &environmentRegion, false);
    memcpy(pass->metadata[195], &environmentRegion, sizeof(environmentRegion));
    pass->color = state->color;
    pass->environment[0] = state->eColor0;
    pass->environment[1] = state->eColor1;
    for (sint32 row = 0; row < 3; ++row)
        for (sint32 column = 0; column < 3; ++column)
            pass->normal[row][column] = DrawC::gMatA.m[row][column] / 4096.0f;
    bool valid = true;
    for (sint32 i = 0; i < 57; ++i)
    {
        pass->visible[i] = (uint8)R3DCar::ObjectVisible[i];
        pass->sortOffsets[i] = (sint16)R3DCar::ReadRenderOrderOffset(car, i, car->render.detail, pass->visible[i]);
        pass->envmap[i] = type < 28 ? (uint8)R3DCar::ObjectInfo[i][1] : (uint8)R3DCar::ObjectInfo[i][1] & 0x80;
        pass->mirror[i] = (state->head.mirror != 0) ^ (R3DCar::rightHandDrive != 0 && i >= 35 && i < 41);
        for (sint32 row = 0; row < 3; ++row)
        {
            float sign = row == 1 ? -1.0f : 1.0f;
            for (sint32 column = 0; column < 3; ++column)
                pass->matrices[i][row][column] = sign * R3DCar::orientMat[i].m[row + column * 3] / 65536.0f;
            const coorddef *position = R3DCar::position + i;
            pass->matrices[i][row][3] = sign * (row == 0 ? position->x : row == 1 ? position->y : position->z) / 256.0f;
        }
        if (pass->visible[i] != 0 && pass->visible[i] < 17)
            valid = hd_frontend_geometry(model->parts + i, scene->obj[i]) && valid;
    }
    pass->valid = valid;
    if (!valid)
        ++hd_frontend_invalid;
}

void hd_frontend_capture_halo(sint32 part, const Transformer::zObj *object, sint32 type, sint32 overlay, sint32 sortOffset)
{
    if (menuPass < 0 || menuPass >= menuBuildCount || part < 0 || part >= 57 || !menuBuild[menuPass].valid)
        return;
    if (overlay >= 51)
    {
        menuBuild[menuPass].valid = false;
        ++hd_frontend_invalid;
        return;
    }
    hd_frontend_capture_halo_tables();
    HdFrontendGeometry *geometry = menuModels[menuPlayer].parts + part;
    if (!hd_frontend_geometry(geometry, object, true))
    {
        menuBuild[menuPass].valid = false;
        ++hd_frontend_invalid;
        return;
    }
    haloBuild[menuPass].types[part] = type;
    haloBuild[menuPass].overlays[part] = overlay;
    haloBuild[menuPass].sortOffsets[part] = sortOffset;
    haloBuild[menuPass].active[part] = true;
}

void hd_frontend_mark_car(void)
{
    if (menuPass >= 0 && menuPass < menuBuildCount && menuBuild[menuPass].valid)
        hd_ui_frontend_marker((uint32)menuPass * 4);
}

void hd_frontend_end_frame(void)
{
    sint32 previousCount = menuCompleteCount;
    memcpy(menuPrevious, menuComplete, sizeof(menuPrevious));
    memcpy(backdropPrevious, backdropComplete, sizeof(backdropPrevious));
    memcpy(motionPrevious, motionComplete, sizeof(motionPrevious));
    memcpy(menuComplete, menuBuild, menuBuildCount * sizeof(*menuBuild));
    memcpy(backdropComplete, backdropBuild, menuBuildCount * sizeof(*backdropBuild));
    memcpy(haloComplete, haloBuild, menuBuildCount * sizeof(*haloBuild));
    menuCompleteCount = menuBuildOverflow ? 0 : menuBuildCount;
    menuCompleteViews = menuBuildViews;
    for (sint32 slot = 0; slot < menuCompleteCount; ++slot)
    {
        const HdFrontendPass *current = menuComplete + slot;
        const HdFrontendPass *previous = menuPrevious + slot;
        for (sint32 part = 0; part < 57; ++part)
            hd_capture_transform(current->matrices[part], &motionComplete[slot].parts[part]);
        hd_capture_transform(current->normal, &motionComplete[slot].normal);
        hd_capture_transform(backdropComplete[slot].camera, &motionComplete[slot].camera);
        if (previousCount != menuCompleteCount || !previous->valid || !current->valid || previous->player != current->player || previous->reflected != current->reflected || previous->x != current->x || previous->y != current->y || (menuResetPlayers & (1 << current->player)) != 0)
        {
            menuPrevious[slot] = *current;
            motionPrevious[slot] = motionComplete[slot];
            backdropPrevious[slot] = backdropComplete[slot];
        }
    }
    menuResetPlayers = 0;
    hd_frontend_sample(1);
    menuPlayer = menuPass = -1;
    ++menuRevision;
    hd_frontend_passes = (uint32)menuCompleteCount;
}

static HdFrontendBackdrop *hd_frontend_backdrop(void)
{
    if (menuPlayer < 0 || menuPass < 0 || menuPass >= menuBuildCount || !menuBuild[menuPass].valid || menuBuild[menuPass].reflected)
        return 0;
    return backdropBuild + menuPass;
}

static void hd_frontend_backdrop_camera(HdFrontendBackdrop *destination, const matrixtdef *matrix, const coorddef *position)
{
    for (sint32 row = 0; row < 3; ++row)
    {
        float sign = row == 1 ? -1.0f : 1.0f;
        for (sint32 column = 0; column < 3; ++column)
            destination->camera[row][column] = sign * matrix->m[row + column * 3] / 65536.0f;
        destination->camera[row][3] = sign * (row == 0 ? position->x : row == 1 ? position->y : position->z) / 256.0f;
    }
}

void hd_frontend_capture_shadow(const Car::tObj *car, const DRender::tView *view, const Draw::tPixMap *map, uint32 color)
{
    HdFrontendBackdrop *destination = hd_frontend_backdrop();
    if (destination == 0 || car == 0 || view == 0 || map == 0)
        return;
    hd_frontend_backdrop_camera(destination, &view->cview.mrotationInv, &view->cview.translationInv);
    for (sint32 i = 0; i < 4; ++i)
    {
        destination->shadow[i][0] = car->N.shadowCoord[i].x / 256.0f;
        destination->shadow[i][1] = car->N.shadowCoord[i].y / 256.0f;
        destination->shadow[i][2] = car->N.shadowCoord[i].z / 256.0f;
    }
    destination->shadowMap = *map;
    destination->shadowMap.tpage = (uint16)((map->tpage & ~96) | 64);
    destination->shadowColor = color;
    destination->shadowActive = true;
    hd_ui_frontend_marker((uint32)menuPass * 4 + 1);
}

void hd_frontend_capture_spot(const matrixtdef *matrix, const coorddef *position, const COORD16 *vertices, uint32 color)
{
    HdFrontendBackdrop *destination = hd_frontend_backdrop();
    if (destination == 0)
        return;
    hd_frontend_backdrop_camera(destination, matrix, position);
    memcpy(destination->spot, vertices, sizeof(destination->spot));
    destination->spotColor = color;
    destination->spotActive = true;
    hd_ui_frontend_marker((uint32)menuPass * 4 + 2);
}

void hd_frontend_capture_ring(const matrixtdef *matrix, const coorddef *position, const COORD16 *vertices, const Draw::tPixMap *map, const sint8 *states, const uint32 *colors)
{
    HdFrontendBackdrop *destination = hd_frontend_backdrop();
    if (destination == 0 || map == 0)
        return;
    hd_frontend_backdrop_camera(destination, matrix, position);
    memcpy(destination->ring, vertices, sizeof(destination->ring));
    destination->ringMap = *map;
    destination->ringMap.tpage = (uint16)((map->tpage & ~96) | 32);
    for (sint32 i = 0; i < 32; ++i)
        destination->ringColors[i] = states[i] >= 0 && states[i] < 5 ? colors[states[i]] : 0;
    destination->ringActive = true;
    hd_ui_frontend_marker((uint32)menuPass * 4 + 3);
}

static const char backdropVertex[] = "layout(location=0)in vec4 aPos;layout(location=1)in vec2 aUV;uniform vec4 uDraw[10];\n"
                                     "out vec2 vUV;out vec4 vColor;out float vDepth;out vec2 vFog;flat out uvec4 vBackdropTex;flat out ivec4 vBackdropRegion;\n"
                                     "void main(){vec4 p=vec4(aPos.xyz,1.0);vec3 c=vec3(dot(uDraw[0],p),dot(uDraw[1],p),dot(uDraw[2],p));"
                                     "vColor=vec4(uDraw[4].rgb*aPos.w,1.0);vBackdropTex=uvec4(uDraw[5]);vBackdropRegion=ivec4(uDraw[9]);"
                                     "vUV=mix(mix(uDraw[6].xy,uDraw[6].zw,aUV.x),mix(uDraw[7].xy,uDraw[7].zw,aUV.x),aUV.y)+vec2(0.5);"
                                     "vDepth=c.z;vFog=vec2(0.0);gl_Position=vec4(c.x*2.0+c.z*uDraw[3].x,c.z*uDraw[3].y-c.y*(1024.0/240.0),1.0000305*c.z-2.0000305,c.z);gl_Position.xy=gl_Position.xy*uDraw[8].xy+gl_Position.w*uDraw[8].zw;}\n";

static const char backdropFragment[] = "flat in uvec4 vBackdropTex;flat in ivec4 vBackdropRegion;\n" VID_PSX_SAMPLING_GLSL HD_VRAM_WORD_GLSL HD_PSX_COLOR_GLSL "void main(){if(vBackdropTex.z==0u){outColor=vec4(vColor.rgb,uParams[0].y);return;}"
                                       "uint word=vidPsxDecode(vBackdropTex.y,vBackdropTex.x,ivec2(floor(vUV)));vec4 color=hdPsxColor(vUV,vBackdropTex.y,vBackdropTex.x,vBackdropRegion,uint(uParams[0].x),word);"
                                       "if(color.a==0.0)discard;outColor=vec4(clamp(color.rgb*vColor.rgb*(255.0/128.0),0.0,1.0),uParams[0].y);}\n";

static bool hd_frontend_backdrop_resources(void)
{
    if (backdropShader == 0)
        backdropShader = vid_shader_create(backdropVertex, backdropFragment);
    VID_TEXTURE *vram = hd_vram_texture();
    if (backdropShader == 0 || vram == 0)
        return false;
    for (sint32 pass = 0; pass < 6; ++pass)
        if (backdropMaterials[pass] == 0)
        {
            VID_MATERIAL *material = vid_mat_create();
            if (material == 0)
                return false;
            backdropMaterials[pass] = material;
            VID_MATERIAL_DATA *data = vid_mat_data(material);
            data->texture = vram;
            data->shader = backdropShader;
            data->params[0] = pass == 0 ? 0.0f : pass == 1 ? 1.0f : 2.0f;
            data->params[1] = pass == 2 ? 0.5f : pass == 5 ? 0.25f : 1.0f;
            data->blend = pass < 2 ? VID_BLEND_NONE : pass == 2 ? VID_BLEND_ALPHA : pass == 4 ? VID_BLEND_SUBTRACT : VID_BLEND_ADD;
            data->filter = VID_FILTER_NEAREST;
            vid_mat_commit(material);
        }
    return true;
}

static VID_GEOMETRY *hd_frontend_backdrop_mesh(sint32 slot, sint32 kind)
{
    const HdFrontendBackdrop *source = backdropRender + slot;
    HdFrontendBackdropMesh *destination = &backdropMeshes[slot][kind];
    const void *positions = kind == 0 ? (const void *)source->shadow : kind == 1 ? (const void *)source->spot : (const void *)source->ring;
    size_t bytes = kind == 0 ? sizeof(source->shadow) : kind == 1 ? sizeof(source->spot) : sizeof(source->ring);
    if (destination->mesh != 0 && memcmp(destination->source, positions, bytes) == 0)
        return destination->mesh;
    vid_mesh_free(destination->mesh);
    uint32 count = kind == 0 ? 6 : kind == 1 ? 96 : 192;
    destination->mesh = kind == 0 ? vid_mesh_create_3d(count) : vid_mesh_create_16(count);
    if (destination->mesh == 0)
        return 0;
    memcpy(destination->source, positions, bytes);
    static const uint8 order[6] = {0, 1, 2, 2, 1, 3};
    if (kind == 0)
    {
        VID_VERTEX3D *vertices = vid_mesh_data_3d(destination->mesh);
        memset(vertices, 0, count * sizeof(*vertices));
        for (uint32 i = 0; i < count; ++i)
        {
            sint32 corner = order[i];
            memcpy(vertices[i].pos, source->shadow[corner], sizeof(source->shadow[corner]));
            vertices[i].pos[3] = 1;
            vertices[i].uv[0] = (float)(corner & 1);
            vertices[i].uv[1] = corner >= 2 ? 1.0f : 0.0f;
        }
    }
    else
    {
        VID_VERTEX16 *vertices = vid_mesh_data_16(destination->mesh);
        memset(vertices, 0, count * sizeof(*vertices));
        for (uint32 i = 0; i < count; ++i)
        {
            sint32 segment = (sint32)i / (kind == 1 ? 3 : 6);
            sint32 corner = kind == 1 ? (sint32)i % 3 : order[i % 6];
            sint32 point = kind == 1 ? corner == 0 ? 32 : (segment + corner - 1) & 31 : (segment * 2 + (corner & 1) * 2 + (corner >= 2 ? 1 : 0)) & 63;
            const COORD16 *position = (kind == 1 ? source->spot : source->ring) + point;
            vertices[i].pos[0] = position->x;
            vertices[i].pos[1] = position->y;
            vertices[i].pos[2] = position->z;
            vertices[i].pos[3] = kind == 1 && corner != 0 ? 0 : 1;
            vertices[i].uv[0] = (sint16)(corner & 1);
            vertices[i].uv[1] = corner >= 2 ? 1 : 0;
        }
    }
    vid_mesh_commit(destination->mesh);
    vid_mesh_static(destination->mesh);
    return destination->mesh;
}

static bool hd_frontend_backdrop_submit(sint32 slot, sint32 kind, uint32 first, uint32 count, const Draw::tPixMap *map, uint32 color)
{
    VID_GEOMETRY *mesh = hd_frontend_backdrop_mesh(slot, kind);
    if (mesh == 0)
        return false;
    const HdFrontendPass *pass = menuRender + slot;
    float uniforms[10][4] = {0};
    memcpy(uniforms, backdropRender[slot].camera, sizeof(backdropRender[slot].camera));
    uniforms[3][0] = pass->x / 256 - 1;
    uniforms[3][1] = 1 - pass->y / 120;
    for (sint32 channel = 0; channel < 3; ++channel)
        uniforms[4][channel] = ((color >> (channel * 8)) & 255) / 255.0f;
    if (map != 0)
    {
        uniforms[5][0] = (float)map->clut;
        uniforms[5][1] = (float)map->tpage;
        uniforms[5][2] = 1;
        uniforms[6][0] = (float)map->u0;
        uniforms[6][1] = (float)map->v0;
        uniforms[6][2] = (float)map->u1;
        uniforms[6][3] = (float)map->v1;
        uniforms[7][0] = (float)map->u2;
        uniforms[7][1] = (float)map->v2;
        uniforms[7][2] = (float)map->u3;
        uniforms[7][3] = (float)map->v3;
        HdTextureRegion region = {0};
        if ((hd_texture_filtering_enabled & HD_FILTER_BACKDROP) != 0)
            hd_texture_region(map, &region);
        uniforms[9][0] = (float)region.x;
        uniforms[9][1] = (float)region.y;
        uniforms[9][2] = (float)region.width;
        uniforms[9][3] = (float)region.height;
    }
    memcpy(uniforms[8], menuMapping, sizeof(menuMapping));
    vid_uniforms(&uniforms[0][0], 10);
    vid_pass(backdropMaterials[map != 0 ? 1 : 3]);
    bool success = vid_submit(mesh, 0, first, count) != 0;
    if (map != 0)
    {
        vid_pass(backdropMaterials[2 + ((map->tpage >> 5) & 3)]);
        success = vid_submit(mesh, 0, first, count) != 0 && success;
    }
    return success;
}

static bool hd_frontend_backdrop_render(sint32 slot, uint32 layers = 7)
{
    const HdFrontendBackdrop *source = backdropRender + slot;
    bool success = true;
    if ((layers & 2) != 0 && source->spotActive && hd_frontend_spot_enabled != 0)
    {
        success = hd_frontend_backdrop_submit(slot, 1, 0, 96, 0, source->spotColor) && success;
        ++hd_frontend_spot_draws;
    }
    if ((layers & 4) != 0 && source->ringActive && hd_frontend_ring_enabled != 0)
        for (sint32 i = 31; i >= 0; --i)
        {
            if (source->ringColors[i] != 0)
            {
                success = hd_frontend_backdrop_submit(slot, 2, i * 6, 6, &source->ringMap, source->ringColors[i]) && success;
                ++hd_frontend_ring_draws;
            }
            success = hd_frontend_backdrop_submit(slot, 2, i * 6, 6, &source->ringMap, 0x300A00) && success;
            ++hd_frontend_ring_draws;
        }
    if ((layers & 1) != 0 && source->shadowActive && hd_frontend_shadow_enabled != 0)
    {
        success = hd_frontend_backdrop_submit(slot, 0, 0, 6, &source->shadowMap, source->shadowColor) && success;
        ++hd_frontend_shadow_draws;
    }
    return success;
}

static const char menuVertex[] = "layout(location=0)in vec4 aPos;layout(location=1)in vec2 aUV;layout(location=3)in vec4 aN;layout(location=5)in uvec4 aTex;\n"
                                 "uniform vec4 uDraw[11];uniform highp usampler2D uAux0;out vec2 vUV;out vec4 vColor;out float vDepth;out vec2 vFog;flat out uvec4 vMenuTex;flat out ivec4 vRegion;smooth out vec3 vMenuNormal;flat out uint vMenuFacet;\n"
                                 "uvec4 meta(uint i){return texelFetch(uAux0,ivec2(int(i),int(uDraw[3].w)),0);}\n"
                                 "void main(){vec4 p=vec4(aPos.xyz,1.0);vec3 c=vec3(dot(uDraw[0],p),dot(uDraw[1],p),dot(uDraw[2],p));uint index=aTex.x;uint mode=uint(uDraw[4].w)&9u;uint flags=(mode&8u)!=0u?meta(141u+index).x:0u;\n"
                                 "flags=(aTex.y&32768u)!=0u?flags>>8:flags&255u;if((aTex.y&1008u)!=0u){flags&=(aTex.y&4095u)>>4;if(flags!=0u){while((flags&3u)==0u)flags>>=2;}}\n"
                                 "uint which=flags&3u;ivec2 offset=ivec2(0);if(which!=0u){uvec4 o=meta(51u+index*3u+which-1u);int d=int(o.x);if(d>=32768)d-=65536;index=uint(int(index)+d);offset=ivec2(o.yz);}\n"
                                 "uvec4 settings=meta(192u);uvec4 info=index<settings.z?meta(index):uvec4(0u);if(which==0u&&mode==0u){if((uint(uDraw[4].w)&2u)!=0u){if((aTex.y&1u)==0u)offset=ivec2(settings.xy);}else if((info.z&127u)!=0u){uvec4 license=meta(194u);offset=(info.z&2u)!=0u?ivec2(license.zw):ivec2(license.xy);}}\n"
                                 "vUV=vec2(uvec2(ivec2(aUV)+offset)&uvec2(255u))+vec2(0.5);vColor=vec4(mode==8u&&(flags&128u)!=0u?vec3(128.0/255.0):uDraw[4].rgb,1.0);vMenuTex=uvec4(info.xy,mode==0u?((uint(uDraw[4].w)&2u)!=0u?aTex.y&1u:0u):(mode&8u)!=0u?aTex.y&1u:0u,index<settings.z?1u:0u);vRegion=index<settings.z?ivec4(meta(198u+index)):ivec4(0);\n"
                                 "vMenuNormal=aN.xyz;vMenuFacet=aTex.y;\n"
                                 "vDepth=c.z;vFog=vec2(0.0);gl_Position=vec4(c.x*uDraw[3].x+c.z*uDraw[3].y,c.z*uDraw[3].z-c.y*uDraw[9].w,1.0000305*c.z-2.0000305,c.z);gl_Position.xy=gl_Position.xy*uDraw[10].xy+gl_Position.w*uDraw[10].zw;}\n";

static const char menuFragment[] = "flat in uvec4 vMenuTex;flat in ivec4 vRegion;smooth in vec3 vMenuNormal;flat in uint vMenuFacet;uniform vec4 uDraw[11];uniform highp usampler2D uAux0;\n" VID_PSX_SAMPLING_GLSL HD_VRAM_WORD_GLSL HD_PSX_COLOR_GLSL "uvec4 menuMeta(uint id){return texelFetch(uAux0,ivec2(int(id),int(uDraw[3].w)),0);}\n"
                                   "void main(){vec2 uv=vUV;vec3 tint=vColor.rgb;uvec4 tex=vMenuTex;ivec4 region=vRegion;if(uDraw[5].x>0.5){vec3 n=vec3(dot(uDraw[6].xyz,vMenuNormal),dot(uDraw[7].xyz,vMenuNormal),dot(uDraw[8].xyz,vMenuNormal));uvec4 e=menuMeta(193u);"
                                   "uv=vec2(n.x,abs(n.z))+vec2(e.z+64u,e.w)+vec2(0.5);region=ivec4(menuMeta(195u));"
                                   "if(region.z>0&&region.w>0)uv=clamp(uv,vec2(region.xy)+vec2(0.5),vec2(region.xy+region.zw)-vec2(0.5));else uv=mod(uv-vec2(0.5),vec2(256.0))+vec2(0.5);"
                                   "tint=(vMenuFacet&4u)!=0u?uDraw[9].rgb:uDraw[5].yzw;tex=uvec4(e.xy,1u,1u);}if(tex.w==0u)discard;uint word=vidPsxDecode(tex.y,tex.x,ivec2(floor(uv)));if(word==0u)discard;uint pass=uint(uParams[0].x);bool semi=tex.z!=0u;bool stp=(word&32768u)!=0u;\n"
                                   "if(pass==0u){if(semi)discard;}else if(pass==1u){if(!semi||stp)discard;}else{if(!semi||!stp||((tex.y>>5)&3u)!=pass-2u)discard;}\n"
                                   "vec4 color=hdPsxColor(uv,tex.y,tex.x,region,pass==0u?0u:pass==1u?1u:2u,word);outColor=vec4(clamp(color.rgb*tint*(255.0/128.0),0.0,1.0),uParams[0].y);}\n";

static bool hd_frontend_resources(void)
{
    if (!hd_frontend_backdrop_resources())
        return false;
    VID_TEXTURE *vram = hd_vram_texture();
    if (vram == 0)
        return false;
    if (menuShader == 0)
        menuShader = vid_shader_create(menuVertex, menuFragment);
    if (menuMetadata == 0)
    {
        menuMetadata = vid_tex_create_fmt(256, 4, VID_RGBA16UI);
        menuUploadRevision = ~menuRevision;
    }
    if (menuShader == 0 || menuMetadata == 0)
        return false;
    if (menuUploadRevision != menuRevision || menuUploadSampling != hd_texture_filtering_enabled)
    {
        uint16 *data = (uint16 *)vid_tex_data(menuMetadata);
        for (sint32 i = 0; i < menuCompleteCount; ++i)
        {
            memcpy(data + i * 256 * 4, menuComplete[i].metadata, sizeof(menuComplete[i].metadata));
            if ((hd_texture_filtering_enabled & HD_FILTER_FRONTEND_CAR) == 0)
            {
                memset(data + (i * 256 + 198) * 4, 0, 51 * 4 * sizeof(*data));
                memset(data + (i * 256 + 195) * 4, 0, 4 * sizeof(*data));
            }
        }
        vid_tex_commit(menuMetadata);
        menuUploadRevision = menuRevision;
        menuUploadSampling = hd_texture_filtering_enabled;
    }
    for (sint32 mirror = 0; mirror < 2; ++mirror)
        for (sint32 pass = 0; pass < 6; ++pass)
            if (menuMaterials[mirror][pass] == 0)
            {
                VID_MATERIAL *material = vid_mat_create();
                if (material == 0)
                    return false;
                menuMaterials[mirror][pass] = material;
                VID_MATERIAL_DATA *data = vid_mat_data(material);
                data->texture = vram;
                data->aux[0] = menuMetadata;
                data->shader = menuShader;
                data->params[0] = (float)pass;
                data->params[1] = pass == 2 ? 0.5f : pass == 5 ? 0.25f : 1.0f;
                data->blend = pass < 2 ? VID_BLEND_NONE : pass == 2 ? VID_BLEND_ALPHA : pass == 4 ? VID_BLEND_SUBTRACT : VID_BLEND_ADD;
                data->filter = VID_FILTER_NEAREST;
                data->cull = mirror == 0 ? 1 : 2;
                data->depth_test = 1;
                data->depth_write = 0;
                vid_mat_commit(material);
            }
    return true;
}

static const char menuHaloVertex[] = "layout(location=0)in vec4 aPos;layout(location=2)in vec4 aColor;uniform vec4 uDraw[1];out vec2 vUV;out vec4 vColor;out float vDepth;out vec2 vFog;\n"
                                     "void main(){gl_Position=aPos;gl_Position.xy=gl_Position.xy*uDraw[0].xy+gl_Position.w*uDraw[0].zw;vUV=vec2(0.0);vColor=aColor/255.0;vDepth=0.0;vFog=vec2(0.0); }\n";

static const char menuHaloFragment[] = "void main(){outColor=vec4(vColor.rgb,1.0); }\n";

static sint32 hd_frontend_halo_type(const HdFrontendPass *pass, sint32 type, sint32 overlay, sint16 flag)
{
    sint32 selected = type & 0xffbf;
    if (overlay >= 0)
    {
        uint16 word = pass->metadata[141 + overlay][0];
        sint32 value = flag < 0 ? (sint8)(word >> 8) : (uint8)word;
        if ((type & 0x40) != 0 && (value & 0x40) == 0)
        {
            word = pass->metadata[141 + 24][0];
            value = flag < 0 ? (sint8)(word >> 8) : (uint8)word;
        }
        if ((value & 0x81) == 0)
            return -1;
        if ((value & 3) != 1 && (type & 0x7f00) != 0)
            selected >>= 8;
    }
    return selected & 255;
}

static bool hd_frontend_halos(sint32 slot)
{
    if (hd_frontend_halos_enabled == 0)
        return true;
    const HdFrontendPass *pass = menuRender + slot;
    HdFrontendModel *model = menuModels + pass->player;
    uint32 faces = 0;
    for (sint32 i = 0; i < 57; ++i)
        if (haloComplete[slot].active[i])
            faces += model->parts[i].count / 3;
    if (faces == 0)
        return true;
    if (!hd_frontend_halo_buffer_begin(slot, faces))
        return false;
    uint32 used = 0;
    for (sint32 part = 0; part < 57; ++part)
        if (haloComplete[slot].active[part])
        {
            const HdFrontendGeometry *geometry = model->parts + part;
            for (uint32 face = 0; face < geometry->count; face += 3)
            {
                sint16 flag = (sint16)geometry->vertices[face].tex[1];
                sint32 type = hd_frontend_halo_type(pass, haloComplete[slot].types[part], haloComplete[slot].overlays[part], flag);
                if (type < 0)
                    continue;
                COORD16 points[3];
                for (sint32 i = 0; i < 3; ++i)
                {
                    points[i].x = geometry->vertices[face + i].pos[0];
                    points[i].y = geometry->vertices[face + i].pos[1];
                    points[i].z = geometry->vertices[face + i].pos[2];
                }
                uint32 added = hd_frontend_halo_buffer_geometry(slot, used, points, flag, type, pass->matrices[part], pass->mirror[part] != 0, pass->x, pass->y, (float)haloComplete[slot].sortOffsets[part] * 4);
                if (added != 0)
                    used += 72;
                if (added != 0)
                    ++hd_frontend_halo_faces;
            }
        }
    hd_frontend_halo_vertices += used;
    if (used == 0)
        return true;
    return hd_frontend_halo_buffer_render(slot, used, menuMapping);
}

static int hd_frontend_draw_compare(const void *left, const void *right)
{
    const HdFrontendDraw *a = (const HdFrontendDraw *)left;
    const HdFrontendDraw *b = (const HdFrontendDraw *)right;
    if (a->bucket != b->bucket)
        return a->bucket > b->bucket ? -1 : 1;
    if (a->part != b->part)
        return a->part > b->part ? -1 : 1;
    return a->first > b->first ? -1 : a->first < b->first ? 1 : 0;
}

static bool hd_frontend_car(sint32 index)
{
    const HdFrontendPass *source = menuRender + index;
    if (!source->valid || source->player < 0 || source->player >= 2)
        return false;
    if (source->reflected && hd_frontend_reflections_enabled == 0)
        return true;
    bool success = true;
    HdFrontendModel *model = menuModels + source->player;
    uint32 capacity = 0;
    for (sint32 partIndex = 0; partIndex < 57; ++partIndex)
        if (source->visible[partIndex] != 0 && source->visible[partIndex] < 17)
            capacity += model->parts[partIndex].count / 3;
    if (capacity > menuDrawCapacity)
    {
        HdFrontendDraw *draws = (HdFrontendDraw *)realloc(menuDraws, capacity * sizeof(*menuDraws));
        if (draws == 0)
            return false;
        menuDraws = draws;
        menuDrawCapacity = capacity;
    }
    uint32 count = 0;
    for (sint32 partIndex = 0; partIndex < 57; ++partIndex)
    {
        if (source->visible[partIndex] == 0 || source->visible[partIndex] >= 17)
            continue;
        HdFrontendGeometry *part = model->parts + partIndex;
        if (part->count == 0)
            continue;
        if (part->mesh == 0)
        {
            part->mesh = vid_mesh_create_16(part->count);
            if (part->mesh == 0)
                return false;
            memcpy(vid_mesh_data_16(part->mesh), part->vertices, part->count * sizeof(*part->vertices));
            vid_mesh_commit(part->mesh);
            vid_mesh_static(part->mesh);
        }
        for (uint32 first = 0; first < part->count; first += 3)
        {
            uint32 sum = 0;
            for (sint32 corner = 0; corner < 3; ++corner)
            {
                const sint16 *position = part->vertices[first + corner].pos;
                float depth = source->matrices[partIndex][2][3];
                for (sint32 axis = 0; axis < 3; ++axis)
                    depth += source->matrices[partIndex][2][axis] * position[axis];
                if (!(depth >= 0))
                    depth = 0;
                if (depth > 65535)
                    depth = 65535;
                sum += (uint32)floorf(depth);
            }
            HdFrontendDraw draw = {first, partIndex, (sint32)((sum * 341) >> 12) + source->sortOffsets[partIndex]};
            menuDraws[count++] = draw;
        }
        ++hd_frontend_parts;
        hd_frontend_triangles += part->count / 3;
    }
    if (count > 1)
        qsortPSX(menuDraws, count, sizeof(*menuDraws), hd_frontend_draw_compare);
    for (uint32 drawIndex = 0; drawIndex < count; ++drawIndex)
    {
        const HdFrontendDraw *draw = menuDraws + drawIndex;
        sint32 partIndex = draw->part;
        HdFrontendGeometry *part = model->parts + partIndex;
        float uniforms[11][4] = {0};
        memcpy(uniforms, source->matrices[partIndex], sizeof(source->matrices[partIndex]));
        uniforms[3][0] = 2;
        uniforms[3][1] = source->x / 256 - 1;
        uniforms[3][2] = 1 - source->y / 120;
        uniforms[3][3] = (float)index;
        for (sint32 channel = 0; channel < 3; ++channel)
        {
            uniforms[4][channel] = ((source->color >> (channel * 8)) & 255) / 255.0f;
            uniforms[5][channel + 1] = ((source->environment[0] >> (channel * 8)) & 255) / 255.0f;
            uniforms[9][channel] = ((source->environment[1] >> (channel * 8)) & 255) / 255.0f;
        }
        uniforms[4][3] = (float)source->envmap[partIndex];
        memcpy(uniforms[6], source->normal, sizeof(source->normal));
        memcpy(uniforms[10], menuMapping, sizeof(menuMapping));
        uniforms[9][3] = 1024.0f / 240;
        for (sint32 effect = 0; effect < 2; ++effect)
        {
            if (effect != 0 && ((source->envmap[partIndex] & 1) == 0 || hd_frontend_environment_enabled == 0))
                continue;
            uniforms[5][0] = (float)effect;
            for (sint32 blend = 0; blend < 6; ++blend)
            {
                vid_pass(menuMaterials[source->mirror[partIndex]][blend]);
                vid_uniforms(&uniforms[0][0], 11);
                success = vid_submit(part->mesh, 0, draw->first, 3) != 0 && success;
            }
        }
    }
    success = hd_frontend_halos(index) && success;
    return success;
}

void hd_frontend_begin_ui(void)
{
    hd_frontend_parts = hd_frontend_triangles = 0;
    hd_frontend_halo_faces = hd_frontend_halo_vertices = 0;
    hd_frontend_shadow_draws = hd_frontend_spot_draws = hd_frontend_ring_draws = 0;
    menuDrawnPlayers = 0;
}

bool hd_frontend_page_ui(uint32 token, uint16 *page)
{
    sint32 slot = (sint32)(token / 4);
    uint32 layer = token % 4;
    if (slot < 0 || slot >= menuCompleteCount || layer == 0)
        return false;
    *page = layer == 1 ? backdropComplete[slot].shadowMap.tpage : layer == 2 ? 0x120 : backdropComplete[slot].ringMap.tpage;
    return true;
}

bool hd_frontend_ready_ui(void)
{
    if (menuCompleteCount <= 0 || menuCompleteCount > 4)
        return false;
    uint32 views = 0;
    for (sint32 i = 0; i < menuCompleteCount; ++i)
    {
        if (!menuComplete[i].valid || menuComplete[i].player < 0 || menuComplete[i].player >= 2)
            return false;
        views |= 1 << menuComplete[i].player;
    }
    return views == menuCompleteViews;
}

bool hd_frontend_prepare_ui(void)
{
    return hd_frontend_ready_ui() && hd_frontend_resources();
}

bool hd_frontend_draw_ui(uint32 token, const float mapping[4])
{
    sint32 slot = (sint32)(token / 4);
    uint32 layer = token % 4;
    if (slot < 0 || slot >= menuCompleteCount || !menuComplete[slot].valid)
        return false;
    uint32 player = (uint32)menuComplete[slot].player;
    if (player >= 2)
        return false;
    if ((menuDrawnPlayers & (1 << player)) == 0)
    {
        if (!vid_clear_depth())
            return false;
        menuDrawnPlayers |= 1 << player;
    }
    memcpy(menuMapping, mapping, sizeof(menuMapping));
    return layer == 0 ? hd_frontend_car(slot) : hd_frontend_backdrop_render(slot, 1 << (layer - 1));
}

static bool hd_frontend_render(void)
{
    hd_frontend_begin_ui();
    menuMapping[0] = menuMapping[1] = 1;
    menuMapping[2] = menuMapping[3] = 0;
    bool success = true;
    for (sint32 index = menuCompleteCount - 1; index >= 0; --index)
    {
        if (!menuComplete[index].reflected)
            success = hd_frontend_backdrop_render(index) && success;
        success = hd_frontend_car(index) && success;
    }
    return success;
}

static bool hd_frontend_preview_frame(float phase)
{
    hd_frontend_sample(phase);
    if (menuCompleteCount <= 0 || menuCompleteCount > 4)
        return false;
    sint32 width, height;
    vid_resize(&width, &height);
    if (width <= 0 || height <= 0 || !hd_frontend_resources())
        return false;
    VID_SAMPLE *samples[80 * 60];
    for (sint32 i = 0; i < 80 * 60; ++i)
        samples[i] = vid_sample_create();
    bool success = vid_begin(0, 0, 0) != 0;
    if (success)
    {
        VID_RECT viewport = {0, 0, width, height};
        vid_viewport(viewport);
        success = hd_frontend_render();
        if (success)
        {
            for (sint32 y = 0; y < 60; ++y)
                for (sint32 x = 0; x < 80; ++x)
                    vid_sample(samples[y * 80 + x], (x + 0.5f) / 80, (y + 0.5f) / 60);
            success = vid_end() != 0;
        }
        else
            vid_cancel();
    }
#if defined(_DEBUG)
    FILE *file = success ? xport_fopen("../status/enhance/frontend-preview.ppm", "wb") : 0;
    if (file != 0)
        fprintf(file, "P6\n80 60\n255\n");
#endif
    hd_frontend_preview_hash = 2166136261;
    hd_frontend_preview_pixels = 0;
    for (sint32 i = 0; i < 80 * 60; ++i)
    {
        uint32 color = success ? vid_sample_color(samples[i]) : 0;
        hd_frontend_preview_hash = (hd_frontend_preview_hash ^ color) * 16777619;
        if (color != 0)
            ++hd_frontend_preview_pixels;
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
    return success;
}

bool hd_frontend_preview(void)
{
    return hd_frontend_preview_frame(1);
}

bool hd_frontend_preview_at(uint32 phase)
{
    return hd_frontend_preview_frame(phase / 1000.0f);
}

void hd_frontend_release(void)
{
    free(menuDraws);
    menuDraws = 0;
    menuDrawCapacity = 0;
    hd_frontend_invalidate();
    for (sint32 player = 0; player < 2; ++player)
        hd_frontend_free_model(menuModels + player);
    for (sint32 mirror = 0; mirror < 2; ++mirror)
        for (sint32 pass = 0; pass < 6; ++pass)
        {
            vid_mat_free(menuMaterials[mirror][pass]);
            menuMaterials[mirror][pass] = 0;
        }
    vid_tex_free(menuMetadata);
    vid_shader_free(menuShader);
    menuMetadata = 0;
    menuShader = 0;
    for (sint32 i = 0; i < 4; ++i)
        for (sint32 kind = 0; kind < 3; ++kind)
        {
            vid_mesh_free(backdropMeshes[i][kind].mesh);
            memset(&backdropMeshes[i][kind], 0, sizeof(backdropMeshes[i][kind]));
        }
    for (sint32 pass = 0; pass < 6; ++pass)
    {
        vid_mat_free(backdropMaterials[pass]);
        backdropMaterials[pass] = 0;
    }
    vid_shader_free(backdropShader);
    backdropShader = 0;
    for (sint32 i = 0; i < 4; ++i)
    {
        vid_mesh_free(menuHaloMeshes[i]);
        menuHaloMeshes[i] = 0;
        menuHaloCapacities[i] = 0;
    }
    vid_mat_free(menuHaloMaterial);
    vid_shader_free(menuHaloShader);
    menuHaloMaterial = 0;
    menuHaloShader = 0;
}
