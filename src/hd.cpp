#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "xport.h"
#include "psx.h"
#include "psx_gpu.h"
#include "vid.h"
#include "vid_psx_sampling.h"
#include "hd.h"
#include "hd_car_lighting.h"
#include "hd_car_normals.h"
#include "hd_fog.h"
#include "hd_frontend.h"
#include "hd_geometry.h"
#include "hd_lens_gpu.h"
#include "hd_lighting.h"
#include "hd_sampling.h"
#include "hd_ui.h"
#include "hd_world_vertex.h"
#include "game/common/anim.h"
#include "game/common/bworld.h"
#include "game/common/camera.h"
#include "game/common/cameraobj.h"
#include "game/common/carobj.h"
#include "game/common/cars.h"
#include "game/common/genericpmx.h"
#include "game/common/gmesetup.h"
#include "game/common/input.h"
#include "game/common/newton.h"
#include "game/common/object.h"
#include "game/common/r3dcar.h"
#include "game/common/replay.h"
#include "game/common/sim.h"
#include "game/common/transformer_obj.h"
#include "game/common/track.h"
#include "game/psx/cario.h"
#include "game/psx/drawc.h"
#include "game/psx/draww.h"
#include "game/psx/flare.h"
#include "game/psx/hrzsku.h"
#include "game/psx/night.h"
#include "game/psx/sfx.h"
#include "game/psx/skidmark.h"
#include "game/psx/textureprocess.h"
#include "game/psx/trackspec.h"
#include "game/psx/weather.h"

struct HdSnapshot
{
    HdCarPose cars[9];
    HdViewPose views[2];
    sint32 viewCount;
};

struct HdSceneFrame
{
    VID_TARGET *image;
    VID_MATERIAL *material;
    VID_GEOMETRY *mesh;
    sint32 width;
    sint32 height;
    bool ready;
};

static HdSceneFrame sceneFrame;
static VID_SHADER *sceneShader;
uint32 hd_scene_captures;
uint32 hd_pause_frames;
uint32 hd_pause_preview_hash;
uint32 hd_pause_preview_pixels;
uint32 hd_pause_preview_colors[80 * 60];

struct HdFeedbackSource
{
    uint64 generation;
    uint64 cameraGeneration[2];
    sint32 mode;
    sint32 firstDepth;
    sint32 secondDepth;
};

struct HdFeedbackState
{
    VID_TARGET *output;
    VID_TARGET *history;
    VID_GEOMETRY *meshes[2];
    VID_MATERIAL *material;
    VID_MATERIAL *maskMaterial;
    VID_MATERIAL *surfaceMaterial;
    VID_GEOMETRY *maskMesh;
    uint64 completedGeneration;
    uint64 capturedGeneration;
    uint64 completedCameraGeneration[2];
    uint64 historyCameraGeneration[2];
    sint32 width;
    sint32 height;
    sint32 views;
    bool ui;
    bool outputReady;
    bool historyReady;
    bool reset;
};

static HdFeedbackSource feedbackSource;
static HdFeedbackState feedbackState;
static VID_SHADER *feedbackShader;
static VID_SHADER *feedbackMaskShader;
static VID_SHADER *feedbackSurfaceShader;
uint32 hd_world_ui;
uint32 hd_feedback_enabled = 1;
uint32 hd_feedback_draws;
uint32 hd_feedback_copies;
uint32 hd_feedback_unsupported;

void hd_capture_feedback(sint32 mode, sint32 firstDepth, sint32 secondDepth, bool advance)
{
    if (advance)
        ++feedbackSource.generation;
    if (feedbackSource.mode != mode || feedbackSource.firstDepth != firstDepth || feedbackSource.secondDepth != secondDepth)
        feedbackState.reset = true;
    feedbackSource.mode = mode;
    feedbackSource.firstDepth = firstDepth;
    feedbackSource.secondDepth = secondDepth;
}

struct HdTrackHalo
{
    PreparedTrackHalo source;
    VEC3F previousStart;
    VEC3F previousEnd;
    float previousScale;
};

struct HdTrackHaloState
{
    HdTrackHalo *entries;
    uint32 count;
    uint32 capacity;
    HdTrackHalo *scratch;
    uint32 scratchCapacity;
    bool reset;
    bool valid;
};

struct HdTrackHaloCommand
{
    uint32 first;
    uint32 count;
    sint32 bucket;
    uint32 order;
};

static HdTrackHaloCommand *trackHaloCommands[2];
static uint32 trackHaloCommandCapacities[2];
static HdTrackHaloState trackHaloStates[2];
static VID_GEOMETRY *trackHaloMeshes[2];
static uint32 trackHaloCapacities[2];
static VID_MATERIAL *trackHaloMaterial;
uint32 hd_track_halos_enabled = 1;
uint32 hd_track_halo_count;
uint32 hd_track_halo_vertices;

static VEC3F hd_track_halo_position(const coorddef *source)
{
    VEC3F result = {source->x / 1024.0f, source->y / 1024.0f, source->z / 1024.0f};
    return result;
}

static void hd_hold_track_halos(void)
{
    for (sint32 player = 0; player < 2; ++player)
        for (uint32 i = 0; i < trackHaloStates[player].count; ++i)
        {
            HdTrackHalo *halo = trackHaloStates[player].entries + i;
            halo->previousStart = hd_track_halo_position(&halo->source.start);
            halo->previousEnd = hd_track_halo_position(&halo->source.end);
            halo->previousScale = (float)halo->source.scale;
        }
}

void hd_capture_track_halos(sint32 player)
{
    uint32 count;
    const PreparedTrackHalo *source = Flare::GetTrackHalos(player, &count);
    HdTrackHaloState *state = trackHaloStates + player;
    if (state->scratchCapacity < count)
    {
        HdTrackHalo *entries = (HdTrackHalo *)realloc(state->scratch, count * sizeof(*entries));
        if (entries == 0)
        {
            state->count = 0;
            state->reset = true;
            state->valid = false;
            return;
        }
        state->scratch = entries;
        state->scratchCapacity = count;
    }
    HdTrackHalo *entries = state->scratch;
    for (uint32 i = 0; i < count; ++i)
    {
        HdTrackHalo *halo = entries + i;
        halo->source = source[i];
        halo->previousStart = hd_track_halo_position(&source[i].start);
        halo->previousEnd = hd_track_halo_position(&source[i].end);
        halo->previousScale = (float)source[i].scale;
        if (!state->reset)
            for (uint32 j = 0; j < state->count; ++j)
            {
                const PreparedTrackHalo *old = &state->entries[j].source;
                if (old->owner != source[i].owner || old->occurrence != source[i].occurrence || old->type != source[i].type || old->hasEnd != source[i].hasEnd || old->flags != source[i].flags || old->objectHalo != source[i].objectHalo)
                    continue;
                VEC3F previousStart = hd_track_halo_position(&old->start);
                VEC3F delta = {previousStart.x - halo->previousStart.x, previousStart.y - halo->previousStart.y, previousStart.z - halo->previousStart.z};
                if (delta.x * delta.x + delta.y * delta.y + delta.z * delta.z < 1048576)
                {
                    halo->previousStart = previousStart;
                    halo->previousEnd = hd_track_halo_position(&old->end);
                    halo->previousScale = (float)old->scale;
                }
                break;
            }
    }
    state->scratch = state->entries;
    state->entries = entries;
    uint32 capacity = state->scratchCapacity;
    state->scratchCapacity = state->capacity;
    state->capacity = capacity;
    state->count = count;
    state->reset = false;
    state->valid = true;
}

struct HdNightState
{
    HdNightSource source;
    HdPose previousTarget;
    HdPose currentTarget;
    HdPose previousCop;
    HdPose currentCop;
    VEC3F previousHuman;
    VEC3F currentHuman;
    bool valid;
};

static HdNightState nightStates[2];
static MATRIXF renderNightTarget;
static MATRIXF renderNightCop;
static VEC3F renderNightHuman;
static const HdNightSource *renderNightSource;
uint32 hd_night_enabled = 1;
uint32 hd_night_world_enabled = 1;
uint32 hd_night_car_enabled = 1;
uint32 hd_night_lightning_enabled = 1;
uint32 hd_night_police_enabled = 1;
uint32 hd_night_draws;
uint32 hd_night_car_lights;

static HdRoadSource roadPrevious;
static HdRoadSource roadCurrent;
static bool roadReset = true;
uint32 hd_road_lines_enabled = 1;
uint32 hd_spike_enabled = 1;
uint32 hd_road_line_quads;
uint32 hd_road_line_submits;
uint32 hd_road_line_total;
uint32 hd_spike_quads;

void hd_capture_road(void)
{
    roadPrevious = roadCurrent;
    DrawW::CaptureRoadSource(&roadCurrent);
    if (roadReset || roadPrevious.spikeActive != roadCurrent.spikeActive || roadPrevious.slice != roadCurrent.slice)
        roadPrevious = roadCurrent;
    roadReset = false;
}

static bool weatherReset[2] = {true, true};
static bool weatherSnapToCurrent[2];
static bool weatherSlotReset[2][HD_WEATHER_PARTICLES];
static bool carsReset = true;
static bool skyReset = true;
static bool objectReset = true;
static bool historyValid;
static bool resetViews[2];
static HdSnapshot previous;
static HdSnapshot current;
static HdSnapshot posePrevious32;
static HdSnapshot poseCurrent32;
static uint64 poseLogicCursor;
static uint64 poseSampleCursor;
static uint64 poseSampleInterval = 4;
static uint32 poseSampleStep;
static sint32 posePlaybackSpeed = -1;
static float poseRenderPhase = 1;
static bool poseHistoryValid;
static bool poseForce = true;
static bool poseCapture = true;
static VID_GEOMETRY *lensMesh;
static bool lensGpuFrame;
static bool lensMarkerValid;
static float lensCenterX;
static float lensCenterY;
uint32 hd_lens_enabled = 1;
uint32 hd_lens_quads;
uint32 hd_lens_queries;

static void hd_reset_lens(void)
{
    hd_lens_gpu_reset();
    lensMarkerValid = false;
}

struct HdSkidState
{
    HdSkidSource previous;
    HdSkidSource current;
    bool valid;
};

static HdSkidState skidStates[2];
static VID_GEOMETRY *skidMeshes[2];
static VID_MATERIAL *skidMaterials[5];
uint32 hd_skid_quads;
uint32 hd_skids_enabled = 1;

void hd_reset_skids(void)
{
    skidStates[0].valid = skidStates[1].valid = false;
}

void hd_capture_skids(sint32 player, const DRender::tView *view)
{
    if (player < 0 || player >= 2)
        return;
    HdSkidState *state = skidStates + player;
    state->previous = state->current;
    Skidmark::CaptureRenderSource(&state->current, view);
    if (!state->valid)
        state->previous = state->current;
    state->valid = true;
}

struct HdSouffleState
{
    HdSouffleSource previous;
    HdSouffleSource current;
    bool reset;
};

static HdSouffleState souffleStates[HD_SOUFFLE_PARTICLES];
static VID_GEOMETRY *souffleMeshes[2];
static VID_MATERIAL *souffleMaterials[6];
uint32 hd_souffle_enabled = 1;
uint32 hd_souffle_quads;
uint32 hd_souffle_filtered_quads;

void hd_reset_souffle(sint32 slot)
{
    if (slot < 0)
        memset(souffleStates, 0, sizeof(souffleStates));
    else if (slot < HD_SOUFFLE_PARTICLES)
    {
        memset(souffleStates + slot, 0, sizeof(souffleStates[slot]));
        souffleStates[slot].reset = true;
    }
}

void hd_move_souffle(sint32 destination, sint32 source)
{
    if (destination < 0 || destination >= HD_SOUFFLE_PARTICLES || source < 0 || source >= HD_SOUFFLE_PARTICLES)
        return;
    if (destination != source)
        souffleStates[destination] = souffleStates[source];
    hd_reset_souffle(source);
}

void hd_capture_souffle(void)
{
    for (sint32 slot = 0; slot < HD_SOUFFLE_PARTICLES; ++slot)
    {
        HdSouffleState *state = souffleStates + slot;
        state->previous = state->current;
        Sfx::CaptureRenderSource(gISouffle != 0 && slot < gCISouffle ? gISouffle + slot : 0, &state->current);
        if (state->reset || !state->previous.active || state->previous.type != state->current.type)
            state->previous = state->current;
        state->reset = false;
    }
}

static void hd_rotation(const matrixtdef *source, QUATF *rotation)
{
    MATRIXF matrix;
    IdentityMatrixf(&matrix);
    for (sint32 row = 0; row < 3; ++row)
        for (sint32 column = 0; column < 3; ++column)
            matrix.m[row][column] = source->m[row + column * 3] / 65536.0f;
    MatrixQuatf(&matrix, rotation);
    NormalizeQuatf(rotation, rotation);
}

static void hd_position(const coorddef *source, VEC3F *position)
{
    position->x = source->x / 1024.0f;
    position->y = source->y / 1024.0f;
    position->z = source->z / 1024.0f;
}

static void hd_mix_pose(const HdPose *from, const HdPose *to, float phase, MATRIXF *matrix, HdPose *pose)
{
    if (!(phase >= 0))
        phase = 0;
    if (phase > 1)
        phase = 1;
    QUATF rotation;
    SlerpQuatf(&from->rotation, &to->rotation, phase, &rotation);
    QuatMatrixf(&rotation, matrix);
    VECTORF position;
    position.vx = from->position.x + (to->position.x - from->position.x) * phase;
    position.vy = from->position.y + (to->position.y - from->position.y) * phase;
    position.vz = from->position.z + (to->position.z - from->position.z) * phase;
    TransMatrixf(matrix, &position);
    if (pose != 0)
    {
        pose->position.x = position.vx;
        pose->position.y = position.vy;
        pose->position.z = position.vz;
        pose->rotation = rotation;
    }
}

static bool hd_car_part_rotation(const matrixtdef *source, QUATF *rotation)
{
    double determinant = source->m[0] * ((double)source->m[4] * source->m[8] - (double)source->m[7] * source->m[5]) - source->m[3] * ((double)source->m[1] * source->m[8] - (double)source->m[7] * source->m[2]) + source->m[6] * ((double)source->m[1] * source->m[5] - (double)source->m[4] * source->m[2]);
    bool mirrored = determinant < 0;
    matrixtdef matrix = *source;
    if (mirrored)
        for (sint32 row = 0; row < 3; ++row)
            matrix.m[row] = -matrix.m[row];
    hd_rotation(&matrix, rotation);
    return mirrored;
}

static float hd_affine_determinant(const float m[3][3])
{
    return m[0][0] * (m[1][1] * m[2][2] - m[1][2] * m[2][1]) - m[0][1] * (m[1][0] * m[2][2] - m[1][2] * m[2][0]) + m[0][2] * (m[1][0] * m[2][1] - m[1][1] * m[2][0]);
}

void hd_capture_transform(const float source[3][4], HdRenderTransform *result)
{
    result->valid = false;
    float rotation[3][3];
    for (sint32 row = 0; row < 3; ++row)
        for (sint32 column = 0; column < 3; ++column)
            rotation[row][column] = source[row][column];
    float determinant = hd_affine_determinant(rotation);
    if (!(fabsf(determinant) > 0.00000001f && fabsf(determinant) < 100000000))
        return;
    if (determinant < 0)
        for (sint32 column = 0; column < 3; ++column)
            rotation[0][column] = -rotation[0][column];
    for (sint32 iteration = 0; iteration < 12; ++iteration)
    {
        determinant = hd_affine_determinant(rotation);
        if (!(determinant > 0.00000001f && determinant < 100000000))
            return;
        float next[3][3];
        float error = 0;
        for (sint32 row = 0; row < 3; ++row)
            for (sint32 column = 0; column < 3; ++column)
            {
                sint32 r1 = (row + 1) % 3, r2 = (row + 2) % 3;
                sint32 c1 = (column + 1) % 3, c2 = (column + 2) % 3;
                float inverse = (rotation[r1][c1] * rotation[r2][c2] - rotation[r1][c2] * rotation[r2][c1]) / determinant;
                next[row][column] = (rotation[row][column] + inverse) * 0.5f;
                error += fabsf(next[row][column] - rotation[row][column]);
            }
        memcpy(rotation, next, sizeof(rotation));
        if (error < 0.000001f)
            break;
        if (iteration == 11)
            return;
    }
    MATRIXF matrix;
    IdentityMatrixf(&matrix);
    memcpy(matrix.m, rotation, sizeof(rotation));
    MatrixQuatf(&matrix, &result->rotation);
    NormalizeQuatf(&result->rotation, &result->rotation);
    QuatMatrixf(&result->rotation, &matrix);
    for (sint32 row = 0; row < 3; ++row)
        for (sint32 column = 0; column < 3; ++column)
        {
            result->stretch[row][column] = 0;
            for (sint32 k = 0; k < 3; ++k)
                result->stretch[row][column] += source[row][k] * matrix.m[column][k];
        }
    result->valid = true;
}

void hd_mix_transform(const float from[3][4], const float to[3][4], const HdRenderTransform *a, const HdRenderTransform *b, float phase, float result[3][4])
{
    memcpy(result, to, sizeof(float) * 12);
    if (phase >= 1 || !a->valid || !b->valid || memcmp(from, to, sizeof(float) * 12) == 0)
        return;
    if (phase <= 0)
    {
        memcpy(result, from, sizeof(float) * 12);
        return;
    }
    QUATF rotation;
    MATRIXF matrix;
    SlerpQuatf(&a->rotation, &b->rotation, phase, &rotation);
    QuatMatrixf(&rotation, &matrix);
    for (sint32 row = 0; row < 3; ++row)
    {
        for (sint32 column = 0; column < 3; ++column)
        {
            result[row][column] = 0;
            for (sint32 k = 0; k < 3; ++k)
                result[row][column] += (a->stretch[row][k] + (b->stretch[row][k] - a->stretch[row][k]) * phase) * matrix.m[k][column];
        }
        result[row][3] = from[row][3] + (to[row][3] - from[row][3]) * phase;
    }
}

static void hd_hold_weather(void)
{
    weatherSnapToCurrent[0] = weatherSnapToCurrent[1] = true;
}

void hd_set_logic_cursor(uint64 tick)
{
    poseLogicCursor = tick;
}

static void hd_prepare_pose_phase(void)
{
    poseRenderPhase = 1;
    if (!poseHistoryValid || simVar.pauseSim != 0 || simVar.quickPauseSim != 0)
        return;
    uint64 frame;
    float fraction = xport_frame_fract_ex(&frame, 32);
    uint64 origin = poseSampleCursor / 4;
    if (frame < origin)
    {
        poseRenderPhase = 0;
        return;
    }
    if (frame - origin > 64)
        return;
    float elapsed = (float)((frame - origin) * 4) + fraction * 4 - (float)(poseSampleCursor % 4);
    poseRenderPhase = elapsed / (float)poseSampleInterval;
    if (poseRenderPhase < 0)
        poseRenderPhase = 0;
    if (poseRenderPhase > 1)
        poseRenderPhase = 1;
}

void hd_clear_history(void)
{
    hd_ui_tach_reset();
    feedbackState.reset = true;
    trackHaloStates[0].reset = trackHaloStates[1].reset = true;
    roadReset = true;
    memset(nightStates, 0, sizeof(nightStates));
    renderNightSource = 0;
    hd_reset_skids();
    hd_reset_lens();
    hd_reset_souffle();
    hd_reset_weather();
    carsReset = true;
    objectReset = true;
    skyReset = true;
    historyValid = false;
    poseHistoryValid = false;
    poseForce = true;
    poseCapture = true;
    poseRenderPhase = 1;
    poseSampleInterval = 4;
    memset(&posePrevious32, 0, sizeof(posePrevious32));
    memset(&poseCurrent32, 0, sizeof(poseCurrent32));
    memset(&previous, 0, sizeof(previous));
    memset(&current, 0, sizeof(current));
    resetViews[0] = false;
    resetViews[1] = false;
}

static void hd_hold_cars(void);

void hd_reset_history(void)
{
    hd_frontend_reset_history();
    hd_ui_tach_reset(true);
    feedbackState.reset = true;
    hd_hold_track_halos();
    roadPrevious = roadCurrent;
    for (sint32 player = 0; player < 2; ++player)
    {
        nightStates[player].previousTarget = nightStates[player].currentTarget;
        nightStates[player].previousCop = nightStates[player].currentCop;
        nightStates[player].previousHuman = nightStates[player].currentHuman;
    }
    for (sint32 player = 0; player < 2; ++player)
        skidStates[player].previous = skidStates[player].current;
    hd_lens_gpu_reset(true);
    for (sint32 slot = 0; slot < HD_SOUFFLE_PARTICLES; ++slot)
    {
        souffleStates[slot].previous = souffleStates[slot].current;
        souffleStates[slot].reset = true;
    }
    hd_hold_weather();
    hd_hold_cars();
    carsReset = true;
    objectReset = true;
    skyReset = true;
    previous = current;
    posePrevious32 = poseCurrent32;
    poseSampleCursor = poseLogicCursor;
    poseSampleInterval = 4;
    poseRenderPhase = 1;
    poseForce = true;
}

void hd_reset_car(const Car::tObj *car)
{
    if (car == 0)
        return;
    for (sint32 slot = 0; slot < 9; ++slot)
        if (current.cars[slot].source == car)
        {
            hd_reset_object(current.cars[slot].object);
            return;
        }
}

void hd_reset_weather(sint32 player)
{
    if (player < 0)
    {
        weatherReset[0] = weatherReset[1] = true;
        weatherSnapToCurrent[0] = weatherSnapToCurrent[1] = false;
    }
    else if (player < 2)
    {
        weatherReset[player] = true;
        weatherSnapToCurrent[player] = false;
    }
}

void hd_reset_weather_particle(sint32 player, sint32 slot)
{
    if (player >= 0 && player < 2 && slot >= 0 && slot < HD_WEATHER_PARTICLES)
        weatherSlotReset[player][slot] = true;
}

void hd_reset_view(sint32 player)
{
    if (player >= 0 && player < 2)
    {
        resetViews[player] = true;
        hd_reset_weather(player);
        if (player == 0)
            hd_reset_lens();
    }
}

void hd_reset_object(const BO::tNewtonObj *object)
{
    for (sint32 slot = 0; slot < 9; ++slot)
        if (current.cars[slot].object == object)
        {
            current.cars[slot].active = false;
            previous.cars[slot].active = false;
            poseForce = true;
        }
    for (sint32 player = 0; player < current.viewCount; ++player)
        if (current.views[player].anchor == object || current.views[player].target == object)
            hd_reset_view(player);
}

void hd_capture(const DRender::tView *views, sint32 count, const float *fogStarts, float fogScale, bool fogEnabled, const HdWorldViewSettings *world)
{
    sint32 playbackSpeed = Replay::ReplayMode == 2 ? Replay::ReplayInterface.speed : 2;
    if (posePlaybackSpeed != playbackSpeed)
    {
        poseForce = true;
        posePlaybackSpeed = playbackSpeed;
    }
    previous = current;
    current.viewCount = count;
    for (sint32 slot = 0; slot < 9; ++slot)
    {
        HdCarPose *pose = &current.cars[slot];
        const Car::tObj *car = slot < Cars::gNumCars ? Cars::gList[slot] : 0;
        pose->source = car;
        pose->object = car != 0 ? &car->N : 0;
        pose->active = car != 0 && car->N.active != 0;
        if (pose->active)
        {
            hd_position(&car->N.position, &pose->pose.position);
            hd_rotation(&car->N.orientMat, &pose->pose.rotation);
        }
        if (!historyValid || previous.cars[slot].source != car || !previous.cars[slot].active)
            previous.cars[slot] = *pose;
        if (pose->active && (!poseCurrent32.cars[slot].active || poseCurrent32.cars[slot].source != car))
            poseForce = true;
    }
    for (sint32 player = 0; player < count; ++player)
    {
        HdViewPose *pose = &current.views[player];
        const DRender::tCalcView *view = &views[player].cview;
        hd_position(&view->translation, &pose->pose.position);
        matrixtdef rotation;
        for (sint32 row = 0; row < 3; ++row)
            for (sint32 column = 0; column < 3; ++column)
                rotation.m[row + column * 3] = view->mrotationInvRaw.m[column + row * 3];
        pose->mirror = GameSetup::gData.mirrorTrack != 0;
        if (pose->mirror)
            for (sint32 row = 0; row < 3; ++row)
                rotation.m[row] = -rotation.m[row];
        hd_rotation(&rotation, &pose->pose.rotation);
        pose->mode = Camera::GetMode(player);
        pose->zoom = (uint8)Camera::gInfo[player].zooming & 3;
        pose->replay = Replay::ReplayMode == 2;
        pose->inCar = Camera::gInfo[player].inCar != 0;
        pose->anchor = Camera::gInfo[player].anchor;
        pose->target = Camera::gInfo[player].target;
        pose->camera = Camera::gInfo[player].camNum;
        pose->animation = Camera::gInfo[player].animHandle;
        pose->lookBehind = Input::gLookBehind[player];
        pose->projection = (float)Camera::gGeomScreen;
        pose->orderTableSize = views[player].id >= 0 && views[player].id < 10 ? Draw::gView[views[player].id].otsize : 0;
        pose->fogStart = fogStarts != 0 ? fogStarts[player] : 0;
        pose->fogScale = fogScale;
        pose->fogEnabled = fogEnabled;
        pose->fogStrength = TrackSpec::gSpec.fogspec.contrast / 65536.0f;
        pose->fogColor[0] = TrackSpec::gSpec.fogspec.color.r / 255.0f;
        pose->fogColor[1] = TrackSpec::gSpec.fogspec.color.g / 255.0f;
        pose->fogColor[2] = TrackSpec::gSpec.fogspec.color.b / 255.0f;
        if (world != 0)
            pose->world = world[player];
        if (!historyValid || resetViews[player] || player >= previous.viewCount || previous.views[player].mode != pose->mode || previous.views[player].inCar != pose->inCar || previous.views[player].zoom != pose->zoom || previous.views[player].replay != pose->replay || previous.views[player].lookBehind != pose->lookBehind || previous.views[player].mirror != pose->mirror || previous.views[player].anchor != pose->anchor || previous.views[player].target != pose->target || previous.views[player].camera != pose->camera || previous.views[player].animation != pose->animation)
        {
            ++feedbackSource.cameraGeneration[player];
            previous.views[player] = *pose;
            poseForce = true;
            if (player == 0)
                hd_reset_lens();
        }
        resetViews[player] = false;
    }
    uint32 sample = (uint32)simGlobal.gameTicks / 2;
    poseCapture = !poseHistoryValid || poseForce || sample != poseSampleStep;
    if (poseCapture)
    {
        bool reset = !poseHistoryValid || poseForce || sample < poseSampleStep || poseLogicCursor <= poseSampleCursor || poseLogicCursor - poseSampleCursor > 64;
        posePrevious32 = poseCurrent32;
        poseCurrent32 = current;
        if (reset)
        {
            posePrevious32 = poseCurrent32;
            poseSampleInterval = 4;
            carsReset = true;
        }
        else
            poseSampleInterval = poseLogicCursor - poseSampleCursor;
        poseSampleCursor = poseLogicCursor;
        poseSampleStep = sample;
        poseHistoryValid = true;
    }
    historyValid = true;
}

bool hd_car_pose(sint32 slot, float phase, MATRIXF *matrix)
{
    if (!historyValid || slot < 0 || slot >= 9 || !current.cars[slot].active)
        return false;
    hd_mix_pose(&previous.cars[slot].pose, &current.cars[slot].pose, phase, matrix, 0);
    return true;
}

static bool hd_view_pose_pair(const HdSnapshot *from, const HdSnapshot *to, sint32 player, float phase, MATRIXF *matrix, HdViewPose *view)
{
    if (!historyValid || player < 0 || player >= to->viewCount)
        return false;
    if (!(phase >= 0))
        phase = 0;
    if (phase > 1)
        phase = 1;
    *view = to->views[player];
    view->projection = from->views[player].projection + (to->views[player].projection - from->views[player].projection) * phase;
    view->fogStart = from->views[player].fogStart + (to->views[player].fogStart - from->views[player].fogStart) * phase;
    view->fogScale = from->views[player].fogScale + (to->views[player].fogScale - from->views[player].fogScale) * phase;
    view->fogStrength = from->views[player].fogStrength + (to->views[player].fogStrength - from->views[player].fogStrength) * phase;
    for (sint32 axis = 0; axis < 3; ++axis)
        view->fogColor[axis] = from->views[player].fogColor[axis] + (to->views[player].fogColor[axis] - from->views[player].fogColor[axis]) * phase;
    hd_mix_pose(&from->views[player].pose, &to->views[player].pose, phase, matrix, &view->pose);
    return true;
}

bool hd_view_pose(sint32 player, float phase, MATRIXF *matrix, HdViewPose *view)
{
    return hd_view_pose_pair(&previous, &current, player, phase, matrix, view);
}

static bool hd_render_view_pose(sint32 player, float phase, MATRIXF *matrix, HdViewPose *view)
{
    if (!poseHistoryValid)
        return hd_view_pose(player, phase, matrix, view);
    return hd_view_pose_pair(&posePrevious32, &poseCurrent32, player, poseRenderPhase, matrix, view);
}

static bool hd_render_car_pose(sint32 slot, float phase, MATRIXF *matrix)
{
    if (!poseHistoryValid)
        return hd_car_pose(slot, phase, matrix);
    if (slot < 0 || slot >= 9 || !poseCurrent32.cars[slot].active)
        return false;
    hd_mix_pose(&posePrevious32.cars[slot].pose, &poseCurrent32.cars[slot].pose, poseRenderPhase, matrix, 0);
    return true;
}

static void hd_night_pose(const matrixtdef *rotation, const coorddef *position, HdPose *pose)
{
    matrixtdef proper = *rotation;
    for (sint32 column = 0; column < 3; ++column)
        proper.m[1 + column * 3] = -proper.m[1 + column * 3];
    hd_rotation(&proper, &pose->rotation);
    hd_position(position, &pose->position);
}

void hd_capture_night(sint32 player, const DRender::tView *view)
{
    if (player < 0 || player >= 2)
        return;
    HdNightState *state = nightStates + player;
    const BO::tNewtonObj *oldTarget = state->source.target, *oldCop = state->source.cop;
    bool wasActive = state->source.active;
    Night::CaptureRenderSource(&state->source, view);
    const HdNightSource *source = &state->source;
    bool reset = !state->valid || oldTarget != source->target || wasActive != source->active;
    bool copReset = reset || oldCop != source->cop;
    state->previousTarget = state->currentTarget;
    state->previousCop = state->currentCop;
    state->previousHuman = state->currentHuman;
    if (source->active)
    {
        hd_night_pose(&source->targetMatrix, &source->targetPosition, &state->currentTarget);
        hd_position(&source->humanPosition, &state->currentHuman);
        if (source->cop != 0)
            hd_night_pose(&source->copMatrix, &source->copPosition, &state->currentCop);
    }
    if (reset)
    {
        state->previousTarget = state->currentTarget;
        state->previousHuman = state->currentHuman;
    }
    if (copReset)
        state->previousCop = state->currentCop;
    state->valid = true;
}

static void hd_night_view(sint32 player, float phase)
{
    const HdNightState *state = nightStates + player;
    renderNightSource = state->valid && state->source.active && hd_night_enabled != 0 ? &state->source : 0;
    if (renderNightSource == 0)
        return;
    hd_mix_pose(&state->previousTarget, &state->currentTarget, phase, &renderNightTarget, 0);
    if (state->source.cop != 0)
        hd_mix_pose(&state->previousCop, &state->currentCop, phase, &renderNightCop, 0);
    if (!(phase >= 0))
        phase = 0;
    if (phase > 1)
        phase = 1;
    for (sint32 axis = 0; axis < 3; ++axis)
        renderNightHuman.v[axis] = state->previousHuman.v[axis] + (state->currentHuman.v[axis] - state->previousHuman.v[axis]) * phase;
}

static sint32 hd_night_strength(const HdNightSource *source, float x, float z)
{
    float width = (float)((uint32)1 << (source->xShift + 5));
    float length = (float)((uint32)1 << (source->zShift + 6));
    if (x <= -width || x >= width || z <= source->nearZ || z >= source->nearZ + length)
        return -1;
    sint32 column = (sint32)floorf((x + width) / (float)((uint32)1 << source->xShift));
    sint32 row = (sint32)floorf((z - source->nearZ) / (float)((uint32)1 << source->zShift));
    return source->shape[row * 64 + column];
}

static uint32 hd_night_car_color(uint32 color, const MATRIXF *car)
{
    const HdNightSource *source = renderNightSource;
    if (source == 0 || !source->headlights || hd_night_car_enabled == 0)
        return color;
    float x = 0, z = 0;
    for (sint32 axis = 0; axis < 3; ++axis)
    {
        float delta = car->t[axis] - renderNightHuman.v[axis];
        x += renderNightTarget.m[0][axis] * delta;
        z += renderNightTarget.m[2][axis] * delta;
    }
    sint32 strength = hd_night_strength(source, x, z);
    if (strength < 0 || strength > 15)
        return color;
    const CVECTOR *add = source->additive + strength;
    sint32 r = (color & 255) + add->r, g = ((color >> 8) & 255) + add->g, b = ((color >> 16) & 255) + add->b;
    if (r > 255)
        r = 255;
    if (g > 255)
        g = 255;
    if (b > 255)
        b = 255;
    ++hd_night_car_lights;
    return r | (g << 8) | (b << 16);
}

static HdSkySource skyPrevious;
static HdSkySource skyCurrent;

void hd_capture_sky(void)
{
    skyPrevious = skyCurrent;
    Hrz::CaptureRenderSource(&skyCurrent);
    hd_lighting_raise_sun(&skyCurrent.sun, GameSetup::gData.track);
    if (skyReset || skyPrevious.mesh != skyCurrent.mesh)
        skyPrevious = skyCurrent;
    skyReset = false;
}

struct HdWeatherParticle
{
    VEC3F previous;
    VEC3F current;
    bool trail;
};

struct HdWeatherState
{
    HdWeatherSource source;
    HdWeatherParticle particles[HD_WEATHER_PARTICLES];
    uint32 processSpan;
    bool valid;
};

static HdWeatherState weatherStates[2];
static VID_GEOMETRY *weatherMeshes[2];
static VID_SHADER *weatherShader;
static VID_MATERIAL *weatherMaterials[5];
uint32 hd_weather_enabled = 1;
uint32 hd_weather_snow_quads;
uint32 hd_weather_rain_lines;
uint32 hd_weather_splat_quads;

struct HdWorldFace
{
    VEC3F center;
    sint16 points[4][3];
    uint16 material;
    sint16 offset;
    bool twoSided;
};

struct HdWorldChunk
{
    VID_GEOMETRY *mesh;
    VID_INDEX *indices;
    uint32 count;
    VEC3F center;
    bool terrain;
    bool quadTopology;
    VID_VERTEX16 *sourceVertices;
    float radius;
    uint32 surfaceCount;
    bool line;
    bool noNight;
    uint32 *semiFaces;
    uint32 semiCount;
    uint64 semiRevision;
    struct HdWorldFace *faces;
    VID_INDEX *twoSidedIndices;
    uint32 twoSidedCount;
};

struct HdWorldSemiCommand
{
    const HdWorldChunk *mesh;
    MATRIXF model;
    uint32 face;
    uint32 group;
    uint32 object;
    uint32 piece;
    uint32 order;
    sint32 bucket;
    sint16 light;
    uint8 blend;
    bool midground;
    bool shadowReceiver;
};

enum
{
    HD_ORDER_TERRAIN,
    HD_ORDER_LINES,
    HD_ORDER_SPIKE,
    HD_ORDER_TRACK_EFFECTS,
    HD_ORDER_OBJECTS,
    HD_ORDER_SKIDS,
    HD_ORDER_SOUFFLE,
    HD_ORDER_CARS
};

struct HdOrderKey
{
    sint32 bucket;
    sint32 family;
    uint32 group;
    uint32 object;
    uint32 piece;
    uint32 face;
    uint32 pass;
};

struct HdOrderedDraw
{
    HdOrderKey key;
    VID_MATERIAL *material;
    VID_GEOMETRY *mesh;
    VID_INDEX *indices;
    uint32 first;
    uint32 count;
    uint32 uniformCount;
    uint32 sequence;
    uint32 captureFirst;
    bool quadTopology;
    float uniforms[32][4];
};

struct HdCarPartPose
{
    HdPose previous;
    HdPose current;
    uint8 visible;
    bool mirrored;
    sint16 orderOffset;
};

struct HdCarSurface;

struct HdCarPart
{
    HdCarPartPose poses[4];
    const Transformer::zObj *object;
    HdWorldChunk geometry;
    uint32 *facets;
    MATRIXF renderPose[4];
    uint64 renderGeneration[4];
    HdCarSurface *renderSurfaces[4];
    uint64 surfaceGeneration[4];
    VID_INDEX *shadowIndices;
    uint32 shadowCount;
    uint64 shadowGeneration;
};

struct HdCarModel
{
    const Car::tObj *source;
    const Transformer::zScene *scene;
    const BO::tNewtonObj *object;
    uint32 flags;
    sint32 type;
    sint16 inside;
    bool envMapEnabled;
    coorddef dimensions;
    bool mediumOnly;
    float sunPrevious;
    float sunCurrent;
    uint32 sunTick;
    uint8 headlights;
    CVECTOR headlightColor;
    HdCarPart parts[57];
    Draw::tPixMap pixmaps[51];
    Transformer::zOverlay overlayMaps[90];
    sint16 overlays[51];
    DrawC::RenderEnvironment environment;
    VEC3F shadowPrevious[4];
    VEC3F shadowCurrent[4];
    CVECTOR shadowColor;
    bool shadowActive;
    uint8 offsets[6];
    uint32 light;
    sint32 pixmapCount;
    bool active;
    bool dirty;
    HdCarHaloSource haloTypes[6];
    sint32 haloReflection;
};

struct HdCarOrder
{
    sint32 root;
    sint32 detail;
    sint32 bucket;
    sint32 bias;
    sint32 size;
    sint32 group;
    float depth;
};

struct HdCarSurface
{
    sint32 blend;
    sint32 bias;
    bool semi;
    bool valid;
    bool castsShadow;
};

struct HdLineFace
{
    VEC3F center;
    uint32 first;
    uint8 type;
};

struct HdLineChunk
{
    HdWorldChunk geometry;
    HdLineFace *faces;
    uint32 count;
};

struct HdLineDraw
{
    sint32 chunk;
    sint32 bucket;
    uint32 face;
    uint32 order;
};

static HdLineChunk *roadChunks;
static sint32 roadChunkCount;
static HdLineDraw *roadDraws[2];
static uint32 roadDrawCapacity;
static HdWorldChunk spikeGeometry;
static VID_TEXTURE *roadMetadata;
static VID_MATERIAL *roadMaterials[2][6];
static VID_MATERIAL *roadCompositeMaterials[2];

static HdCarModel carModels[9];
static HdHaloTables haloTables;
static VID_GEOMETRY *carHaloMeshes[2];
static uint32 carHaloCapacities[2];
static VID_MATERIAL *carHaloMaterial;
uint32 hd_car_halos_enabled = 1;
uint32 hd_car_halo_count;
uint32 hd_car_halo_reflections;
uint32 hd_car_halo_vertices;
uint32 hd_car_halo_reflections_enabled = 1;
static VID_SHADER *carShader;
static VID_MATERIAL *carMaterial[2];
static VID_MATERIAL *carEffectMaterials[5][2];
static VID_MATERIAL *carSemiMaterials[4][2];
static VID_MATERIAL *carShadowMaterials[2];
static HdWorldChunk carShadows;
static Draw::tPixMap carShadowMap;
static bool carShadowMapValid;
static VID_TARGET *playerCube;
static VID_SHADER *carCubeShader;
static VID_MATERIAL *carCubeMaterials[2];
static VID_MATERIAL *carCubeSemiMaterials[4][2];
static VID_GEOMETRY *cubeSkyMeshes[6];
static VID_GEOMETRY *cubeSkidMeshes[6];
static VID_INDEX *cubeOrderIndices[8];
static uint32 cubeOrderCapacity[8];
static uint32 cubeOrderCount[8];
static uint32 cubeFace;
static uint32 cubeSkyCapacity;
static bool cubeReady;
static float carRenderPhase;
static bool cubeRendering;
static const Car::tObj *cubeOwner;
static VEC3F cubeOrigin;
static sint32 cubeTick = -1;
static float cubePhase;
uint32 hd_cube_faces;
uint32 hd_cube_captures;
uint32 hd_cube_car_draws;
uint32 hd_dynamic_reflections_enabled = 1;
static VID_TEXTURE *carMaterials;
static uint64 carCaptureRevision;
static uint64 carUploadRevision;
static uint64 renderGeneration;
uint32 hd_car_part_draws;
uint32 hd_car_triangle_count;
uint32 hd_car_reflection_draws;
uint32 hd_car_reflections_enabled = 1;
uint32 hd_car_shadows_enabled = 1;
uint32 hd_car_shadow_draws;
uint32 hd_car_invalid_count;
uint32 hd_car_material_checks;
uint32 hd_car_material_errors;
uint32 hd_car_semi_enabled = 1;
uint32 hd_car_semi_draws;
uint32 hd_car_variant_draws[4];
uint32 hd_car_interior_parts;
sint32 hd_car_variant_override = -1;

struct HdObjectState
{
    HdPose pose;
    VEC3F scale;
    sint32 definition;
    sint32 animationCycle;
    sint16 light;
    uint8 type;
    bool active;
    bool billboard;
    sint32 orderOffset;
};

struct HdObjectPiece
{
    float previous[3][4];
    float current[3][4];
    HdRenderTransform previousTransform;
    HdRenderTransform currentTransform;
    sint32 definition;
    sint16 light;
    float surfaceLift;
    bool surfaceReady;
};

struct HdObjectEntry
{
    const Trk::SimpleInst *source;
    const Group *simObjects;
    sint32 chunk;
    uint8 kind;
    uint32 ordinal;
    HdObjectState previous;
    HdObjectState current;
    HdObjectPiece *pieces;
    uint32 pieceCount;
    uint32 pieceCapacity;
    ObjectAnim *animation;
    MATRIXF renderModel[2];
    uint64 renderModelGeneration[2];
};

struct HdStaticWorldEntry
{
    const Trk::SimpleInst *source;
    MATRIXF model;
    uint32 first;
    uint32 count;
    sint32 definition;
    sint16 light;
    CVECTOR color;
};

static HdWorldChunk staticWorld;
static HdStaticWorldEntry *staticWorldEntries;
static uint32 staticWorldEntryCount;
static bool staticWorldReady;

struct HdBillboardBank
{
    VID_GEOMETRY *mesh;
    HdStaticWorldEntry *entries;
    uint32 entryCapacity;
    uint32 vertexCapacity;
    uint32 indexCapacity;
    uint64 generation;
};

static HdBillboardBank billboardBanks[2];
static VID_INDEX *billboardIndices[8][2];
static uint32 billboardCounts[8][2];
uint32 hd_billboard_batch_draws[8];
uint32 hd_billboard_batch_objects[8];
static VID_INDEX *staticWorldIndices[8][2];
static uint32 staticWorldCounts[8][2];
static VID_MATERIAL *staticWorldMaterials[2];
uint32 hd_static_batch_draws[8];
uint32 hd_static_batch_objects[8];

static HdObjectEntry *worldObjects;
static uint32 worldObjectCount;
static bool worldObjectSnapshotValid;
static uint32 worldCustomObjectCount;
static const Group *worldCustomObjectGroup;
static HdWorldChunk *worldDefinitions;
static sint32 worldDefinitionCount;
static HdWorldChunk *worldChunks;
static sint32 worldChunkCount;
static VID_SHADER *skyShader;
static VID_MATERIAL *skyMaterial;
static VID_GEOMETRY *skyMeshes[2];
static uint32 skyCapacities[2];
static VID_GEOMETRY *celestialMeshes[2];

struct HdCelestialCommand
{
    uint32 first;
    uint32 count;
    sint32 blend;
    bool sun;
};

static HdCelestialCommand celestialCommands[2][19];
static uint32 celestialCommandCount[2];
uint32 hd_celestial_enabled = 1;
uint32 hd_sun_triangles;
uint32 hd_moon_quads;
uint32 hd_lightning_quads;
static VID_SHADER *worldShader;
static VID_SHADER *worldBatchShader;
static VID_MATERIAL *worldBatchMaterial;
static HdWorldChunk worldBatch;
static VEC3F worldBatchOrigin;
static uint32 *worldBatchStarts;
static uint32 *worldBatchTopology;
static bool worldBatchReady;
static VID_INDEX *worldBatchIndices[8];
static uint32 worldBatchCounts[8];
static uint64 worldSemiRevision = 1;
static uint64 worldSemiSignature;
uint32 hd_world_batch_draws[8];
uint32 hd_world_batch_chunks[8];
VID_STATS hd_render_stats;
static VID_STATS renderStatsBegin;
uint64 hd_shader_draws[7];
static uint64 shaderStatsBegin[7];
static VID_MATERIAL *worldMaterial;
static VID_MATERIAL *worldObjectMaterials[2];
static VID_MATERIAL *worldTwoSidedMaterials[5];
static VID_MATERIAL *worldObjectSemiMaterials[2][4];
static bool worldSubmissionValid;
static VID_MATERIAL *worldSemiMaterials[4];

static HdWorldSemiCommand *worldSemiCommands;
static uint32 worldSemiCount;
static uint32 worldSemiCapacity;
static HdOrderedDraw *orderedDraws;
static uint32 orderedCount;
static uint32 orderedCapacity;
uint32 hd_ordered_draws;
uint32 hd_world_semi_enabled = 1;
uint32 hd_world_semi_draws;
uint32 hd_world_material_checks;
uint32 hd_world_material_errors;
uint32 hd_order_material_checks;
uint32 hd_order_material_errors;
static VID_TEXTURE *worldVram;
static VID_TEXTURE *worldMaterials;
static VID_TEXTURE *worldPalettes;
static VID_TEXTURE *nightTables;
static VID_TEXTURE *nightShape;
static VID_TEXTURE *nightColors;
static sint32 lightingView;
static const HdLightingProjection *lightingCapture;
static VID_SHADER *carDepthShader;
static VID_MATERIAL *carDepthMaterial;
uint32 hd_gpu_shadows_enabled = 1;
uint32 hd_headlights_enabled = 1;
static uint32 worldVramRows[512];
static uint32 worldVramVersion;
static bool worldVramValid;
static bool cleanupRegistered;
static XportCleanup previousCleanup;
uint32 hd_world_quad_count;
uint32 hd_world_filter_regions;
uint32 hd_car_filter_regions;
uint32 hd_world_invalid_count;
uint32 hd_world_preview_pixels;
uint32 hd_world_preview_hash;
uint32 hd_world_preview_colors[80 * 60];
uint8 hd_world_preview_masks[80 * 60];
uint32 hd_world_preview_surface;
uint32 hd_world_preview_view_hashes[2];
float hd_world_preview_phase = 1;
uint32 hd_world_object_draws;
uint32 hd_world_object_pending;
uint32 hd_world_object_active;
uint32 hd_destruction_draws;
uint32 hd_destruction_captured;
uint32 hd_sky_quads;
uint32 hd_ring_quads;
uint32 hd_star_quads;

static const char skyVertex[] = "layout(location=0) in vec4 aPos; layout(location=1) in vec2 aUV;\n"
                                "layout(location=2) in vec4 aColor; layout(location=5) in uvec4 aTex;layout(location=6)in uvec2 aRect;\n"
                                "out vec2 vUV; out vec4 vColor; out float vDepth; out vec2 vFog; flat out uvec4 vSkyTex;flat out ivec4 vRegion;\n"
                                "void main(){gl_Position=aPos;vUV=aUV;vColor=aColor/255.0;vDepth=0.0;vFog=vec2(0.0);vSkyTex=aTex;vRegion=ivec4(ivec2(aTex.w&255u,aTex.w>>8),ivec2(aRect));}\n";

static const char skyFragment[] = "flat in uvec4 vSkyTex;flat in ivec4 vRegion;\n" VID_PSX_SAMPLING_GLSL HD_VRAM_WORD_GLSL HD_PSX_COLOR_GLSL "void main(){if(vSkyTex.z==0u){outColor=vColor;return;}\n"
                                  "vec4 color=hdPsxColor(vUV,vSkyTex.y,vSkyTex.x,vRegion,0u,vidPsxDecode(vSkyTex.y,vSkyTex.x,ivec2(floor(vUV))));\n"
                                  "if(color.a==0.0)discard;outColor=vec4(clamp(color.rgb*vColor.rgb*(255.0/128.0),0.0,1.0),1.0);}\n";

static const char worldVertex[] = HD_WORLD_VERTEX_GLSL("layout(location=5) in uvec4 aTex;float hdWorldDepth(){return aExtra.w;}\n");
static const char worldBatchVertex[] = HD_WORLD_VERTEX_GLSL("layout(location=1) in vec2 aUV;\n#define aTex uvec4(uint(aUV.x),uint(aUV.y),uint(aN.w),uint(aExtra.w))\nfloat hdWorldDepth(){return aPos.w;}\n");

static const char worldFragment[] = "uniform vec4 uDraw[26];in vec3 vAtmospherePosition;flat in uvec4 vPmx; flat in uvec2 vWindow;flat in ivec4 vRegion;const uvec2 vWindowOrigin=uvec2(0u);\n" HD_BOUNDED_WINDOW_GLSL VID_PSX_SAMPLING_GLSL HD_VRAM_WORD_GLSL HD_PSX_COLOR_GLSL HD_FOG_BODY_GLSL HD_LIGHTING_FRAGMENT_GLSL HD_WORLD_CULL_GLSL "vec3 hdWorldShade(vec3 color){vec3 light=(vPmx.w&1u)!=0u?vec3(1.0):vColor.rgb*(255.0/128.0);return hdFog(hdLighting(clamp(color*light,0.0,1.0),color));}\n"
                                    "void main(){hdLightingPrepare();if(hdWorldBackFace(vPmx.w))discard;\n"
                                    "bool semi=(vPmx.w&2u)!=0u;uint stp=uint(uParams[0].x);\n"
                                    "if(uParams[0].z<0.5){if(semi&&stp==0u)discard;}\n"
                                    "else if(uParams[0].z<1.5){stp=semi?1u:0u;}\n"
                                    "else{if(!semi||((vPmx.y>>5)&3u)!=uint(uParams[0].w))discard;stp=2u;}\n"
                                    "ivec2 pixel=ivec2(uvec2(ivec2(floor(vUV)))&(~vWindow)&uvec2(255u));\n"
                                    "uint word=vidPsxDecode(vPmx.y,vPmx.x,pixel);"
                                    "if(uParams[1].w>0.5){bool blend=(word&32768u)!=0u;vec4 color=hdPsxColor(vUV,vPmx.y,vPmx.x,vRegion,blend?2u:1u,word);"
                                    "if(color.a==0.0)discard;outColor=vec4(hdWorldShade(color.rgb),blend?0.5:1.0);return;}"
                                    "vec4 color=hdPsxColor(vUV,vPmx.y,vPmx.x,vRegion,stp,word);\n"
                                    "if(color.a==0.0)discard;\n"
                                    "outColor=vec4(hdWorldShade(color.rgb),uParams[0].y); }\n";

static void hd_release_car(HdCarModel *car)
{
    for (sint32 part = 0; part < 57; ++part)
    {
        vid_mesh_free(car->parts[part].geometry.mesh);
        vid_index_free(car->parts[part].geometry.indices);
        free(car->parts[part].facets);
        vid_index_free(car->parts[part].shadowIndices);
        for (sint32 detail = 0; detail < 4; ++detail)
            free(car->parts[part].renderSurfaces[detail]);
    }
    memset(car, 0, sizeof(*car));
}

static void hd_hold_cars(void)
{
    for (sint32 slot = 0; slot < 9; ++slot)
    {
        HdCarModel *car = carModels + slot;
        for (sint32 corner = 0; corner < 4; ++corner)
            car->shadowPrevious[corner] = car->shadowCurrent[corner];
        for (sint32 index = 0; index < 57; ++index)
            for (sint32 variant = 0; variant < 4; ++variant)
                car->parts[index].poses[variant].previous = car->parts[index].poses[variant].current;
    }
}

void hd_capture_cars(void)
{
    Flare::CaptureHaloSource(&haloTables);
    carShadowMapValid = gShadowPixmap[0] != 0;
    if (carShadowMapValid)
    {
        carShadowMap = *gShadowPixmap[0];
        carShadowMap.tpage = (uint16)((carShadowMap.tpage & 0xFF9F) | (2 << 5));
    }
    for (sint32 slot = 0; slot < 9; ++slot)
    {
        HdCarModel *model = carModels + slot;
        Car::tObj *car = slot < Cars::gNumCars ? Cars::gList[slot] : 0;
        sint32 type = car != 0 ? car->render.currentCarType : -1;
        sint32 country = car != 0 ? (uint8)car->render.currentCountry >> 7 : 0;
        const Transformer::zScene *scene = type >= 0 && type < 50 ? R3DCar::LoadedScenePointer[country][type] : 0;
        const Transformer::zScene *fullScene = type >= 0 && type < 50 && car->render.medOnly != 0 ? R3DCar::FullDetailScene[country][type] : 0;
        if (fullScene != 0)
            scene = fullScene;
        if (model->source != car || model->scene != scene)
        {
            hd_release_car(model);
            model->source = car;
            model->scene = scene;
            model->dirty = true;
        }
        bool variantReset = model->inside != (car != 0 ? car->render.inside : 0);
        bool wasActive = model->active;
        model->active = car != 0 && car->N.active != 0 && scene != 0;
        if (!model->active)
            continue;
        float sunTarget = car != 0 && BWorldSm::TunnelFlagSm(&car->N.simRoadInfo) ? 0.0f : 1.0f;
        uint32 sunTick = (uint32)simGlobal.gameTicks;
        if (carsReset || !wasActive || model->sunTick > sunTick)
            model->sunPrevious = model->sunCurrent = sunTarget;
        else if (model->sunTick != sunTick)
        {
            model->sunPrevious = model->sunCurrent;
            float delta = sunTarget - model->sunCurrent;
            if (delta > 0.125f)
                delta = 0.125f;
            if (delta < -0.25f)
                delta = -0.25f;
            model->sunCurrent += delta;
        }
        model->sunTick = sunTick;
        sint32 start = car->render.textureStartIndex;
        if (start < 0 || start >= CarIO::carPixMapCount || CarIO::carPixMap == 0)
        {
            model->active = false;
            ++hd_car_invalid_count;
            continue;
        }
        Draw::tPixMap pixmaps[51] = {0};
        model->pixmapCount = CarIO::carPixMapCount - start;
        if (model->pixmapCount > 51)
            model->pixmapCount = 51;
        memcpy(pixmaps, CarIO::carPixMap + start, model->pixmapCount * sizeof(*pixmaps));
        uint8 offsets[6] = {(uint8)car->render.textureOffsetU, (uint8)car->render.textureOffsetV, car->render.licenseOffsetU[0], car->render.licenseOffsetV[0], car->render.licenseOffsetU[1], car->render.licenseOffsetV[1]};
        memcpy(model->pixmaps, pixmaps, sizeof(model->pixmaps));
        memcpy(model->offsets, offsets, sizeof(offsets));
        memcpy(model->overlayMaps, scene->overlay, sizeof(model->overlayMaps));
        DrawC::BuildRenderOverlays(car, 0, GameSetup::gData.mirrorTrack != 0, model->overlays);
        bool envMapEnabled = R3DCar::eMapColour.r != 0 || hd_dynamic_reflections_enabled != 0;
        if (model->type != type || model->envMapEnabled != envMapEnabled)
            model->dirty = true;
        model->type = type;
        model->inside = car->render.inside;
        model->dimensions = car->N.dimension;
        model->mediumOnly = car->render.medOnly != 0 && fullScene == 0;
        model->headlights = (uint8)car->control.lights & 6;
        model->headlightColor = TrackSpec::gSpec.nightspec.nightcolor;
        model->envMapEnabled = envMapEnabled;
        model->object = &car->N;
        model->flags = car->carFlags;
        model->light = BWorldSm::QuadLight(&car->N.simRoadInfo) & 0xFFFFFF;
        sint32 lightAverage = ((model->light & 255) + ((model->light >> 8) & 255) + ((model->light >> 16) & 255)) / 3;
        if (lightAverage < 24)
            lightAverage = 24;
        if (DrawC::gEnvMap != 0 && DrawC::gShadow != 0)
        {
            sint32 environmentIndex, shadowIndex;
            R3DCar::GetEnvironmentIndices(car, &environmentIndex, &shadowIndex);
            if (environmentIndex < DrawC::gEnvMapMax && shadowIndex < DrawC::gShadowMax)
                DrawC::ReadRenderEnvironment(car, 0, lightAverage, &model->environment, environmentIndex, shadowIndex);
            else
                memset(&model->environment, 0, sizeof(model->environment));
        }
        for (sint32 light = 0; light < 6; ++light)
            R3DCar::ReadRenderHaloType(car, light + 17, model->haloTypes + light);
        model->haloReflection = 0;
        if ((car->wheel[0].wheelInAir | car->wheel[1].wheelInAir | car->wheel[2].wheelInAir | car->wheel[3].wheelInAir) == 0)
        {
            if (GameSetup::gData.Time == 0)
                model->haloReflection = model->environment.wetRoad != 0;
            else if (GameSetup::gData.commMode == 1)
                model->haloReflection = model->environment.wetRoad != 0 ? 1 : -1;
            else if (model->environment.wetRoad != 0)
                model->haloReflection = -2;
        }
        bool shadowReset = carsReset || !wasActive || !previous.cars[slot].active || !model->shadowActive;
        model->shadowActive = R3DCar::shadowColour.r != 0 && car->N.simOptz == 0;
        if (model->shadowActive && poseCapture)
        {
            coorddef shadow[4];
            Newton::CalculateRenderShadow(car, simGlobal.gameTicks, shadow);
            for (sint32 corner = 0; corner < 4; ++corner)
            {
                model->shadowPrevious[corner] = model->shadowCurrent[corner];
                hd_position(shadow + corner, model->shadowCurrent + corner);
                if (shadowReset)
                    model->shadowPrevious[corner] = model->shadowCurrent[corner];
            }
            model->shadowColor.r = (uint8)((lightAverage * R3DCar::shadowColour.r) >> 8);
            model->shadowColor.g = (uint8)((lightAverage * R3DCar::shadowColour.g) >> 8);
            model->shadowColor.b = (uint8)((lightAverage * R3DCar::shadowColour.b) >> 8);
        }
        if (!poseCapture)
            continue;
        DRender::tView origin = {0};
        origin.cview.mrotationInv.m[0] = origin.cview.mrotationInv.m[4] = origin.cview.mrotationInv.m[8] = 65536;
        for (sint32 variant = 0; variant < 4; ++variant)
        {
            matrixtdef matrices[57] = {0};
            coorddef positions[57] = {0};
            char visible[57] = {0};
            if (variant != 3 || ((model->inside & 1) != 0 && type < 28))
                R3DCar::PrepareRenderParts(car, &origin, variant, model->inside, matrices, positions, visible, !model->mediumOnly, scene);
            for (sint32 index = 0; index < 57; ++index)
            {
                HdCarPart *part = model->parts + index;
                HdCarPartPose *pose = part->poses + variant;
                const Transformer::zObj *object = scene->obj[index];
                bool reset = carsReset || variantReset || !wasActive || !previous.cars[slot].active || part->object != object || pose->visible == 0;
                if (part->object != object)
                    model->dirty = true;
                if (variant == 3)
                    part->object = object;
                pose->previous = pose->current;
                pose->visible = (uint8)visible[index];
                pose->orderOffset = (sint16)R3DCar::ReadRenderOrderOffset(car, index, variant, visible[index]);
                if (object == 0 || object->numFacet == 0 || pose->visible == 0)
                    continue;
                hd_position(positions + index, &pose->current.position);
                bool mirrored = hd_car_part_rotation(matrices + index, &pose->current.rotation);
                if (pose->mirrored != mirrored)
                    reset = true;
                pose->mirrored = mirrored;
                if (reset)
                    pose->previous = pose->current;
            }
        }
    }
    carsReset = false;
    poseForce = false;
    ++carCaptureRevision;
}

static VEC3F hd_weather_world(const SVECTOR *particle, const HdWeatherSource *source)
{
    VEC3F result;
    float position[3] = {source->camera.x / 1024.0f, source->camera.y / 1024.0f, source->camera.z / 1024.0f};
    for (sint32 row = 0; row < 3; ++row)
        position[row] += (source->rotation.m[row] * (float)particle->vx + source->rotation.m[row + 3] * (float)particle->vy + source->rotation.m[row + 6] * (float)particle->vz) / 65536.0f;
    result.x = position[0];
    result.y = position[1];
    result.z = position[2];
    return result;
}

void hd_capture_weather(void)
{
    for (sint32 player = 0; player < 2; ++player)
    {
        HdWeatherState *state = weatherStates + player;
        HdWeatherSource source;
        memset(&source, 0, sizeof(source));
        if (player < current.viewCount)
            Weather::CaptureRenderSource(player, &source);
        bool reset = weatherReset[player] || !state->valid || state->source.owner != source.owner || state->source.type != source.type;
        sint32 processDelta = source.processTick - state->source.processTick;
        if (!reset && (processDelta < 0 || processDelta > 64))
            reset = true;
        bool processed = reset || processDelta != 0;
        if (processed)
            state->processSpan = !reset && processDelta > 0 && processDelta <= 64 ? (uint32)processDelta : 2;
        if (processed)
            weatherSnapToCurrent[player] = false;
        if (source.active)
        {
            for (sint32 slot = 0; slot < source.count; ++slot)
            {
                HdWeatherParticle *particle = state->particles + slot;
                bool reseed = reset || weatherSlotReset[player][slot] || slot >= state->source.count;
                if (processed || reseed)
                {
                    particle->previous = particle->current;
                    particle->current = hd_weather_world(source.particles + slot, &source);
                    particle->trail = !reseed;
                    if (reseed)
                    {
                        particle->previous = particle->current;
                    }
                }
            }
        }
        state->source = source;
        state->valid = source.active;
        weatherReset[player] = false;
        memset(weatherSlotReset[player], 0, sizeof(weatherSlotReset[player]));
    }
}

static void hd_road_release(void)
{
    for (sint32 chunk = 0; chunk < roadChunkCount; ++chunk)
    {
        vid_mesh_free(roadChunks[chunk].geometry.mesh);
        vid_index_free(roadChunks[chunk].geometry.indices);
        free(roadChunks[chunk].faces);
    }
    free(roadChunks);
    roadChunks = 0;
    roadChunkCount = 0;
    for (sint32 player = 0; player < 2; ++player)
    {
        free(roadDraws[player]);
        roadDraws[player] = 0;
    }
    roadDrawCapacity = 0;
    vid_mesh_free(spikeGeometry.mesh);
    vid_index_free(spikeGeometry.indices);
    memset(&spikeGeometry, 0, sizeof(spikeGeometry));
    memset(&roadPrevious, 0, sizeof(roadPrevious));
    memset(&roadCurrent, 0, sizeof(roadCurrent));
    roadReset = true;
}

static void hd_feedback_release(void)
{
    vid_mat_free(feedbackState.material);
    vid_mat_free(feedbackState.surfaceMaterial);
    vid_mat_free(feedbackState.maskMaterial);
    vid_mesh_free(feedbackState.maskMesh);
    vid_target_free(feedbackState.output);
    vid_target_free(feedbackState.history);
    for (sint32 player = 0; player < 2; ++player)
        vid_mesh_free(feedbackState.meshes[player]);
    memset(&feedbackState, 0, sizeof(feedbackState));
    memset(&feedbackSource, 0, sizeof(feedbackSource));
    feedbackState.reset = true;
}

static void hd_scene_release(void)
{
    vid_mat_free(sceneFrame.material);
    vid_mesh_free(sceneFrame.mesh);
    vid_target_free(sceneFrame.image);
    memset(&sceneFrame, 0, sizeof(sceneFrame));
}

void hd_release_level(void)
{
    hd_lighting_frame();
    cubeReady = false;
    cubeOwner = 0;
    cubeTick = -1;
    for (sint32 face = 0; face < 6; ++face)
    {
        vid_mesh_free(cubeSkyMeshes[face]);
        vid_mesh_free(cubeSkidMeshes[face]);
        vid_index_free(cubeOrderIndices[face]);
        cubeSkyMeshes[face] = cubeSkidMeshes[face] = 0;
        cubeOrderIndices[face] = 0;
        cubeOrderCapacity[face] = 0;
    }
    for (sint32 bank = 6; bank < 8; ++bank)
    {
        vid_index_free(cubeOrderIndices[bank]);
        cubeOrderIndices[bank] = 0;
        cubeOrderCapacity[bank] = cubeOrderCount[bank] = 0;
    }
    cubeSkyCapacity = 0;
    orderedCount = 0;
    worldSemiCount = 0;
    hd_scene_release();
    hd_lens_gpu_release();
    hd_feedback_release();
    for (sint32 player = 0; player < 2; ++player)
    {
        free(trackHaloStates[player].entries);
        free(trackHaloStates[player].scratch);
        free(trackHaloCommands[player]);
        trackHaloCommands[player] = 0;
        trackHaloCommandCapacities[player] = 0;
        memset(trackHaloStates + player, 0, sizeof(trackHaloStates[player]));
        trackHaloStates[player].reset = true;
        vid_mesh_free(trackHaloMeshes[player]);
        trackHaloMeshes[player] = 0;
        trackHaloCapacities[player] = 0;
    }
    memset(nightStates, 0, sizeof(nightStates));
    renderNightSource = 0;
    memset(skidStates, 0, sizeof(skidStates));
    for (sint32 player = 0; player < 2; ++player)
    {
        vid_mesh_free(skidMeshes[player]);
        skidMeshes[player] = 0;
    }
    hd_reset_lens();
    for (sint32 player = 0; player < 2; ++player)
    {
        vid_mesh_free(carHaloMeshes[player]);
        carHaloMeshes[player] = 0;
        carHaloCapacities[player] = 0;
        vid_mesh_free(celestialMeshes[player]);
        celestialMeshes[player] = 0;
        celestialCommandCount[player] = 0;
    }
    for (sint32 player = 0; player < 2; ++player)
    {
        vid_mesh_free(souffleMeshes[player]);
        souffleMeshes[player] = 0;
    }
    hd_reset_souffle();
    for (sint32 player = 0; player < 2; ++player)
    {
        vid_mesh_free(weatherMeshes[player]);
        weatherMeshes[player] = 0;
    }
    memset(weatherStates, 0, sizeof(weatherStates));
    hd_reset_weather();
    vid_mesh_free(carShadows.mesh);
    memset(&carShadows, 0, sizeof(carShadows));
    carShadowMapValid = false;
    for (sint32 slot = 0; slot < 9; ++slot)
        hd_release_car(carModels + slot);
    carsReset = true;
    hd_road_release();
    for (sint32 i = 0; i < worldChunkCount; ++i)
    {
        vid_mesh_free(worldChunks[i].mesh);
        vid_index_free(worldChunks[i].indices);
        free(worldChunks[i].faces);
        free(worldChunks[i].semiFaces);
    }
    for (sint32 i = 0; i < worldDefinitionCount; ++i)
    {
        vid_mesh_free(worldDefinitions[i].mesh);
        vid_index_free(worldDefinitions[i].indices);
        vid_index_free(worldDefinitions[i].twoSidedIndices);
        free(worldDefinitions[i].faces);
        free(worldDefinitions[i].semiFaces);
        free(worldDefinitions[i].sourceVertices);
    }
    free(worldDefinitions);
    worldDefinitions = 0;
    worldDefinitionCount = 0;
    for (uint32 i = 0; i < worldObjectCount; ++i)
        free(worldObjects[i].pieces);
    free(worldObjects);
    worldObjects = 0;
    worldObjectCount = 0;
    worldCustomObjectCount = 0;
    worldCustomObjectGroup = 0;
    worldObjectSnapshotValid = false;
    objectReset = true;
    for (sint32 player = 0; player < 2; ++player)
    {
        vid_mesh_free(skyMeshes[player]);
        skyMeshes[player] = 0;
        skyCapacities[player] = 0;
    }
    memset(&skyCurrent, 0, sizeof(skyCurrent));
    memset(&skyPrevious, 0, sizeof(skyPrevious));
    skyReset = true;
    vid_mesh_free(worldBatch.mesh);
    memset(&worldBatch, 0, sizeof(worldBatch));
    vid_mesh_free(staticWorld.mesh);
    memset(&staticWorld, 0, sizeof(staticWorld));
    free(staticWorldEntries);
    staticWorldEntries = 0;
    staticWorldEntryCount = 0;
    staticWorldReady = false;
    for (sint32 player = 0; player < 2; ++player)
    {
        vid_mesh_free(billboardBanks[player].mesh);
        free(billboardBanks[player].entries);
    }
    memset(billboardBanks, 0, sizeof(billboardBanks));
    for (sint32 bank = 0; bank < 8; ++bank)
        for (sint32 side = 0; side < 2; ++side)
        {
            vid_index_free(billboardIndices[bank][side]);
            billboardIndices[bank][side] = 0;
            billboardCounts[bank][side] = 0;
        }
    for (sint32 bank = 0; bank < 8; ++bank)
        for (sint32 side = 0; side < 2; ++side)
        {
            vid_index_free(staticWorldIndices[bank][side]);
            staticWorldIndices[bank][side] = 0;
            staticWorldCounts[bank][side] = 0;
        }
    free(worldBatchStarts);
    worldBatchStarts = 0;
    free(worldBatchTopology);
    worldBatchTopology = 0;
    worldBatchReady = false;
    for (sint32 bank = 0; bank < 8; ++bank)
    {
        vid_index_free(worldBatchIndices[bank]);
        worldBatchIndices[bank] = 0;
        worldBatchCounts[bank] = 0;
    }
    free(worldChunks);
    worldChunks = 0;
    worldChunkCount = 0;
    hd_world_quad_count = 0;
    hd_world_invalid_count = 0;
}

static void hd_free(void)
{
    hd_car_normals_forget(0);
    for (sint32 orientation = 0; orientation < 2; ++orientation)
    {
        vid_mat_free(carCubeMaterials[orientation]);
        carCubeMaterials[orientation] = 0;
        for (sint32 blend = 0; blend < 4; ++blend)
        {
            vid_mat_free(carCubeSemiMaterials[blend][orientation]);
            carCubeSemiMaterials[blend][orientation] = 0;
        }
    }
    vid_shader_free(carCubeShader);
    vid_target_free(playerCube);
    carCubeShader = 0;
    playerCube = 0;
    vid_mat_free(carDepthMaterial);
    vid_shader_free(carDepthShader);
    carDepthMaterial = 0;
    carDepthShader = 0;
    hd_texture_regions_free();
    hd_frontend_release();
    hd_ui_release();
    Flare::ResetTrackEffects();
    hd_release_level();
    free(worldSemiCommands);
    worldSemiCommands = 0;
    worldSemiCapacity = 0;
    free(orderedDraws);
    orderedDraws = 0;
    orderedCapacity = 0;
    for (sint32 blend = 0; blend < 4; ++blend)
    {
        vid_mat_free(worldSemiMaterials[blend]);
        worldSemiMaterials[blend] = 0;
    }
    vid_mesh_free(lensMesh);
    lensMesh = 0;
    vid_tex_free(carMaterials);
    carMaterials = 0;
    for (sint32 orientation = 0; orientation < 2; ++orientation)
    {
        for (sint32 effect = 0; effect < 5; ++effect)
        {
            vid_mat_free(carEffectMaterials[effect][orientation]);
            carEffectMaterials[effect][orientation] = 0;
        }
        for (sint32 blend = 0; blend < 4; ++blend)
        {
            vid_mat_free(carSemiMaterials[blend][orientation]);
            carSemiMaterials[blend][orientation] = 0;
        }
        vid_mat_free(carMaterial[orientation]);
        carMaterial[orientation] = 0;
        vid_mat_free(carShadowMaterials[orientation]);
        carShadowMaterials[orientation] = 0;
    }
    vid_shader_free(carShader);
    carShader = 0;
    vid_shader_free(sceneShader);
    sceneShader = 0;
    vid_shader_free(feedbackSurfaceShader);
    feedbackSurfaceShader = 0;
    vid_shader_free(feedbackMaskShader);
    feedbackMaskShader = 0;
    vid_shader_free(feedbackShader);
    feedbackShader = 0;
    vid_mat_free(trackHaloMaterial);
    trackHaloMaterial = 0;
    vid_mat_free(carHaloMaterial);
    carHaloMaterial = 0;
    for (sint32 i = 0; i < 5; ++i)
    {
        vid_mat_free(weatherMaterials[i]);
        weatherMaterials[i] = 0;
    }
    for (sint32 pass = 0; pass < 5; ++pass)
    {
        vid_mat_free(skidMaterials[pass]);
        skidMaterials[pass] = 0;
    }
    vid_shader_free(weatherShader);
    weatherShader = 0;
    for (sint32 i = 0; i < 6; ++i)
    {
        vid_mat_free(souffleMaterials[i]);
        souffleMaterials[i] = 0;
    }
    vid_mat_free(skyMaterial);
    vid_shader_free(skyShader);
    skyMaterial = 0;
    skyShader = 0;
    for (sint32 pass = 0; pass < 5; ++pass)
    {
        vid_mat_free(worldTwoSidedMaterials[pass]);
        worldTwoSidedMaterials[pass] = 0;
    }
    for (sint32 orientation = 0; orientation < 2; ++orientation)
    {
        vid_mat_free(worldObjectMaterials[orientation]);
        worldObjectMaterials[orientation] = 0;
        for (sint32 blend = 0; blend < 4; ++blend)
        {
            vid_mat_free(worldObjectSemiMaterials[orientation][blend]);
            worldObjectSemiMaterials[orientation][blend] = 0;
        }
    }
    vid_mat_free(worldMaterial);
    for (sint32 orientation = 0; orientation < 2; ++orientation)
        for (sint32 pass = 0; pass < 6; ++pass)
        {
            vid_mat_free(roadMaterials[orientation][pass]);
            roadMaterials[orientation][pass] = 0;
        }
    for (sint32 orientation = 0; orientation < 2; ++orientation)
    {
        vid_mat_free(roadCompositeMaterials[orientation]);
        roadCompositeMaterials[orientation] = 0;
    }
    vid_tex_free(roadMetadata);
    roadMetadata = 0;
    for (sint32 orientation = 0; orientation < 2; ++orientation)
    {
        vid_mat_free(staticWorldMaterials[orientation]);
        staticWorldMaterials[orientation] = 0;
    }
    vid_mat_free(worldBatchMaterial);
    worldBatchMaterial = 0;
    vid_shader_free(worldBatchShader);
    worldBatchShader = 0;
    vid_shader_free(worldShader);
    vid_tex_free(worldVram);
    vid_tex_free(worldMaterials);
    vid_tex_free(worldPalettes);
    vid_tex_free(nightTables);
    vid_tex_free(nightShape);
    vid_tex_free(nightColors);
    hd_lighting_free();
    nightTables = nightShape = nightColors = 0;
    worldMaterial = 0;
    worldShader = 0;
    worldVram = 0;
    worldMaterials = 0;
    worldPalettes = 0;
    worldVramValid = false;
    cleanupRegistered = false;
    XportCleanup cleanup = previousCleanup;
    previousCleanup = 0;
    if (cleanup != 0)
        cleanup();
}

void hd_register_cleanup(void)
{
    if (!cleanupRegistered)
    {
        previousCleanup = xport_get_cleanup();
        xport_set_cleanup(hd_free);
        cleanupRegistered = true;
    }
}

static uint32 hd_object_group_count(const Group *group)
{
    return group != 0 && group->m_num_elements > 0 ? group->m_num_elements : 0;
}

static void hd_object_group(const Group *group, const Group *simObjects, sint32 chunk, uint8 kind, uint32 *used)
{
    if (group == 0)
        return;
    const Trk::SimpleInst *instance = (const Trk::SimpleInst *)(group + 1);
    for (uint32 i = 0; i < hd_object_group_count(group); ++i)
    {
        if (instance->size < 8)
        {
            ++hd_world_invalid_count;
            break;
        }
        HdObjectEntry *entry = worldObjects + (*used)++;
        entry->source = instance;
        entry->simObjects = simObjects;
        entry->chunk = chunk;
        entry->kind = kind;
        entry->ordinal = i;
        instance = (const Trk::SimpleInst *)((const uint8 *)instance + instance->size);
    }
}

static bool hd_objects_init(void)
{
    uint32 customCount = hd_object_group_count(Object::customObjInst);
    if (worldObjects != 0 && worldCustomObjectGroup == Object::customObjInst && worldCustomObjectCount == customCount)
        return true;
    for (uint32 i = 0; i < worldObjectCount; ++i)
        free(worldObjects[i].pieces);
    free(worldObjects);
    worldObjects = 0;
    worldObjectCount = 0;
    if (Track::chunkList == 0 || gPersistObjDef == 0 || Track::gObjDefs == 0)
        return false;
    hd_register_cleanup();
    uint32 count = hd_object_group_count(gPersistObjInst) + hd_object_group_count(gPersistMidgroundObjInst) + customCount;
    sint32 chunks = Track::GetChunkCount();
    for (sint32 i = 0; i < chunks; ++i)
        count += hd_object_group_count(Track::chunkList[i].objInstanceBuf) + hd_object_group_count(Track::chunkList[i].objSpecialInstanceBuf);
    worldObjects = (HdObjectEntry *)calloc(count != 0 ? count : 1, sizeof(*worldObjects));
    if (worldObjects == 0)
        return false;
    uint32 used = 0;
    for (sint32 i = 0; i < chunks; ++i)
    {
        const Chunk *chunk = Track::chunkList + i;
        hd_object_group(chunk->objInstanceBuf, chunk->simObjBuf, i, 0, &used);
        hd_object_group(chunk->objSpecialInstanceBuf, chunk->simObjBuf, i, 1, &used);
    }
    hd_object_group(gPersistObjInst, 0, -1, 2, &used);
    hd_object_group(gPersistMidgroundObjInst, 0, -1, 3, &used);
    hd_object_group(Object::customObjInst, Object::customSimObjs, -1, 4, &used);
    worldObjectCount = used;
    worldCustomObjectGroup = Object::customObjInst;
    worldCustomObjectCount = customCount;
    objectReset = true;
    return true;
}

static void hd_object_state(const HdObjectEntry *entry, HdObjectState *state)
{
    memset(state, 0, sizeof(*state));
    const Trk::SimpleInst *instance = entry->source;
    state->type = instance->type;
    state->definition = instance->pad;
    state->light = -1;
    state->orderOffset = entry->kind == 1 ? 50 : entry->kind == 3 ? -1 : 125;
    if ((entry->kind == 0 || entry->kind == 4) && !DrawW::ReadRenderOrderOffset(instance->zoffset, &state->orderOffset))
        state->orderOffset = -2;
    state->scale.x = state->scale.y = state->scale.z = 1;
    state->pose.rotation.w = 1;
    if ((state->type & 128) != 0 || state->definition < 0 || state->definition >= gPersistObjDef->m_num_elements)
        return;
    if (entry->kind == 1 && (GameSetup::gData.Time != 0 || GameSetup::gData.Weather != 0))
        return;
    if (entry->kind != 0 && entry->kind != 4 && (state->type == 3 || state->type == 7))
    {
        const Trk::AnimateInst *animation = (const Trk::AnimateInst *)instance;
        if (animation->count < 2 || animation->size < (sint32)(sizeof(*animation) + animation->count * sizeof(Anim::tFrame)))
        {
            ++hd_world_invalid_count;
            return;
        }
        coorddef position;
        matrixtdef rotation;
        Trk::AnimateInst *source = const_cast<Trk::AnimateInst *>(animation);
        sint32 time = DrawW::GetAnimationTime(source);
        uint32 interval = (uint16)animation->interval;
        if (interval - 1 >= 400)
            interval = 6;
        state->animationCycle = time / (sint32)interval / (animation->count - 1);
        Anim::GetRotPos(source, 1, time, &position, &rotation);
        hd_position(&position, &state->pose.position);
        hd_rotation(&rotation, &state->pose.rotation);
        state->active = true;
        return;
    }
    if (instance->size < (sint32)sizeof(*instance))
        return;
    coorddef position = {instance->x, instance->y, instance->z};
    hd_position(&position, &state->pose.position);
    if (entry->kind != 0 && entry->kind != 4)
    {
        state->active = state->type == 1;
        return;
    }
    if ((instance->flags & 1) != 0)
    {
        state->billboard = true;
        state->active = true;
    }
    else if (state->type == 1)
    {
        state->active = true;
        return;
    }
    else if (state->type == 2 || state->type == 5)
    {
        if (instance->size < (sint32)sizeof(Trk::CollideBoomInst))
            return;
        const Trk::CollideBoomInst *collide = (const Trk::CollideBoomInst *)instance;
        if (state->type == 5 && entry->simObjects != 0 && collide->simIndex < entry->simObjects->m_num_elements)
        {
            Trk::SimObject *object = (Trk::SimObject *)(entry->simObjects + 1) + collide->simIndex;
            if (Object::GetAnim(object) != 0)
            {
                const Object::RenderPart *parts;
                if (Object::GetPreparedParts(Object::GetAnim(object), &parts) < 0)
                    ++hd_world_object_pending;
                return;
            }
        }
        QUATF rotation;
        rotation.x = collide->qx / 16384.0f;
        rotation.y = collide->qy / 16384.0f;
        rotation.z = collide->qz / 16384.0f;
        rotation.w = collide->qw / 16384.0f;
        NormalizeQuatf(&rotation, &state->pose.rotation);
        state->scale.x = collide->sx / 256.0f;
        state->scale.y = collide->sy / 256.0f;
        state->scale.z = collide->sz / 256.0f;
        if (state->type == 2)
            memcpy(&state->light, (const uint8 *)instance + 34, sizeof(state->light));
        state->active = true;
        return;
    }
    else if (state->type == 9)
    {
        if (instance->size < 28)
            return;
        sint16 angle;
        memcpy(&angle, (const uint8 *)instance + 20, sizeof(angle));
        MATRIXF rotation;
        IdentityMatrixf(&rotation);
        RotMatrixYf(angle * (6.28318530718f / 1024), &rotation);
        MatrixQuatf(&rotation, &state->pose.rotation);
        state->active = true;
    }
    if (state->type == 9 && state->active)
    {
        if (instance->size < 28)
        {
            state->active = false;
            return;
        }
        sint16 sx, sy;
        memcpy(&sx, (const uint8 *)instance + 24, sizeof(sx));
        memcpy(&sy, (const uint8 *)instance + 22, sizeof(sy));
        state->scale.x = state->scale.z = sx / 256.0f;
        state->scale.y = sy / 256.0f;
        memcpy(&state->light, (const uint8 *)instance + 26, sizeof(state->light));
    }
}

static bool hd_surface_height(const VEC3F *point, sint32 chunk, float *height)
{
    bool found = false;
    float closest = 0;
    for (sint32 neighbor = -2; neighbor <= 2; ++neighbor)
    {
        const HdWorldChunk *surface = worldChunks + ((chunk + neighbor) % worldChunkCount + worldChunkCount) % worldChunkCount;
        const uint8 triangles[2][3] = {{0, 1, 2}, {2, 1, 3}};
        for (uint32 faceIndex = 0; faceIndex < surface->surfaceCount; ++faceIndex)
        {
            const HdWorldFace *face = surface->faces + faceIndex;
            for (sint32 triangle = 0; triangle < 2; ++triangle)
            {
                const sint16 *a = face->points[triangles[triangle][0]];
                const sint16 *b = face->points[triangles[triangle][1]];
                const sint16 *c = face->points[triangles[triangle][2]];
                float bx = (float)(b[0] - a[0]), bz = (float)(b[2] - a[2]);
                float cx = (float)(c[0] - a[0]), cz = (float)(c[2] - a[2]);
                float determinant = bx * cz - bz * cx;
                if (fabsf(determinant) < 0.000001f)
                    continue;
                float x = point->x - surface->center.x - a[0];
                float z = point->z - surface->center.z - a[2];
                float u = (x * cz - z * cx) / determinant;
                float v = (bx * z - bz * x) / determinant;
                if (u < -0.00001f || v < -0.00001f || u + v > 1.00001f)
                    continue;
                float y = surface->center.y + a[1] + u * (b[1] - a[1]) + v * (c[1] - a[1]);
                float distance = fabsf(y - point->y);
                if (!found || distance < closest)
                {
                    *height = y;
                    closest = distance;
                    found = true;
                }
            }
        }
    }
    return found;
}

static float hd_sign_triangle_lift(const VEC3F *triangle, sint32 chunk)
{
    float lift = 0;
    const uint8 topology[2][3] = {{0, 1, 2}, {2, 1, 3}};
    for (sint32 neighbor = -2; neighbor <= 2; ++neighbor)
    {
        const HdWorldChunk *surface = worldChunks + ((chunk + neighbor) % worldChunkCount + worldChunkCount) % worldChunkCount;
        for (uint32 faceIndex = 0; faceIndex < surface->surfaceCount; ++faceIndex)
            for (sint32 half = 0; half < 2; ++half)
            {
                VEC3F ground[3];
                for (sint32 corner = 0; corner < 3; ++corner)
                {
                    const sint16 *point = surface->faces[faceIndex].points[topology[half][corner]];
                    ground[corner].x = surface->center.x + point[0];
                    ground[corner].y = surface->center.y + point[1];
                    ground[corner].z = surface->center.z + point[2];
                }
                float winding = (ground[1].x - ground[0].x) * (ground[2].z - ground[0].z) - (ground[1].z - ground[0].z) * (ground[2].x - ground[0].x);
                if (fabsf(winding) < 0.000001f)
                    continue;
                winding = winding > 0 ? 1.0f : -1.0f;
                VEC3F polygon[8];
                memcpy(polygon, triangle, sizeof(VEC3F) * 3);
                sint32 count = 3;
                for (sint32 edge = 0; edge < 3 && count != 0; ++edge)
                {
                    VEC3F clipped[8];
                    sint32 used = 0;
                    const VEC3F *a = ground + edge;
                    const VEC3F *b = ground + (edge + 1) % 3;
                    VEC3F previous = polygon[count - 1];
                    float previousSide = winding * ((b->x - a->x) * (previous.z - a->z) - (b->z - a->z) * (previous.x - a->x));
                    for (sint32 i = 0; i < count; ++i)
                    {
                        VEC3F current = polygon[i];
                        float side = winding * ((b->x - a->x) * (current.z - a->z) - (b->z - a->z) * (current.x - a->x));
                        if ((side >= 0) != (previousSide >= 0))
                        {
                            float phase = previousSide / (previousSide - side);
                            VEC3F crossing = {previous.x + (current.x - previous.x) * phase, previous.y + (current.y - previous.y) * phase, previous.z + (current.z - previous.z) * phase};
                            clipped[used++] = crossing;
                        }
                        if (side >= 0)
                            clipped[used++] = current;
                        previous = current;
                        previousSide = side;
                    }
                    count = used;
                    memcpy(polygon, clipped, count * sizeof(VEC3F));
                }
                for (sint32 i = 0; i < count; ++i)
                {
                    float height;
                    if (hd_surface_height(polygon + i, chunk, &height) && height + 0.25f - polygon[i].y > lift)
                        lift = height + 0.25f - polygon[i].y;
                }
            }
    }
    return lift;
}

static float hd_sign_surface_lift(const HdObjectPiece *piece, sint32 chunk)
{
    const HdWorldChunk *definition = worldDefinitions + piece->definition;
    if (chunk < 0 || chunk >= worldChunkCount)
    {
        chunk = -1;
        float closest = 0;
        for (sint32 i = 0; i < worldChunkCount; ++i)
        {
            float dx = piece->current[0][3] - worldChunks[i].center.x;
            float dz = piece->current[2][3] - worldChunks[i].center.z;
            float distance = dx * dx + dz * dz;
            if (chunk < 0 || distance < closest)
            {
                chunk = i;
                closest = distance;
            }
        }
    }
    float lift = 0;
    const uint8 topology[2][3] = {{0, 1, 2}, {2, 1, 3}};
    for (uint32 i = 0; i < definition->count / 6; ++i)
        for (sint32 half = 0; half < 2; ++half)
        {
            VEC3F triangle[3];
            for (sint32 corner = 0; corner < 3; ++corner)
            {
                const sint16 *local = definition->faces[i].points[topology[half][corner]];
                triangle[corner].x = piece->current[0][3] + piece->current[0][0] * local[0] + piece->current[0][1] * local[1] + piece->current[0][2] * local[2];
                triangle[corner].y = piece->current[1][3] + piece->current[1][0] * local[0] + piece->current[1][1] * local[1] + piece->current[1][2] * local[2];
                triangle[corner].z = piece->current[2][3] + piece->current[2][0] * local[0] + piece->current[2][1] * local[1] + piece->current[2][2] * local[2];
            }
            float clearance = hd_sign_triangle_lift(triangle, chunk);
            if (clearance > lift)
                lift = clearance;
        }
    return lift;
}

static bool hd_capture_pieces(HdObjectEntry *entry)
{
    ObjectAnim *animation = 0;
    if ((entry->kind == 0 || entry->kind == 4) && entry->source->type == 5 && (entry->source->flags & 1) == 0 && entry->simObjects != 0)
    {
        const Trk::CollideBoomInst *instance = (const Trk::CollideBoomInst *)entry->source;
        if (instance->size >= (sint32)sizeof(*instance) && instance->simIndex < entry->simObjects->m_num_elements)
            animation = Object::GetAnim((Trk::SimObject *)(entry->simObjects + 1) + instance->simIndex);
    }
    const Object::RenderPart *parts = 0;
    sint32 count = animation != 0 ? Object::GetPreparedParts(animation, &parts) : 0;
    if (count < 0)
        count = 0;
    uint32 previousCount = entry->pieceCount;
    if ((uint32)count > entry->pieceCapacity)
    {
        HdObjectPiece *pieces = (HdObjectPiece *)realloc(entry->pieces, count * sizeof(*pieces));
        if (pieces == 0)
            return false;
        entry->pieces = pieces;
        entry->pieceCapacity = count;
    }
    entry->pieceCount = 0;
    for (sint32 i = 0; i < count; ++i)
    {
        sint32 definition = -1;
        for (sint32 j = 0; j < gPersistObjDef->m_num_elements; ++j)
            if (Track::gObjDefs[j] == parts[i].definition)
            {
                definition = j;
                break;
            }
        if (definition < 0)
        {
            ++hd_world_invalid_count;
            continue;
        }
        uint32 index = entry->pieceCount++;
        HdObjectPiece *piece = entry->pieces + index;
        bool reset = objectReset || index >= previousCount || entry->animation != animation || piece->definition != definition;
        if (!reset)
        {
            memcpy(piece->previous, piece->current, sizeof(piece->previous));
            piece->previousTransform = piece->currentTransform;
        }
        for (sint32 row = 0; row < 3; ++row)
            for (sint32 column = 0; column < 3; ++column)
                piece->current[row][column] = parts[i].matrix.m[row + column * 3] / 65536.0f;
        piece->current[0][3] = parts[i].position.x / 1024.0f;
        piece->current[1][3] = parts[i].position.y / 1024.0f;
        piece->current[2][3] = parts[i].position.z / 1024.0f;
        piece->light = parts[i].light;
        piece->definition = definition;
        if (reset || !parts[i].groundedSign)
        {
            piece->surfaceReady = false;
            piece->surfaceLift = 0;
        }
        if (parts[i].groundedSign && !piece->surfaceReady && worldChunks != 0 && worldDefinitions != 0 && worldChunkCount > 0)
        {
            piece->surfaceLift = hd_sign_surface_lift(piece, entry->chunk);
            piece->surfaceReady = true;
        }
        piece->current[1][3] += piece->surfaceLift;
        hd_capture_transform(piece->current, &piece->currentTransform);
        if (reset)
        {
            memcpy(piece->previous, piece->current, sizeof(piece->previous));
            piece->previousTransform = piece->currentTransform;
        }
    }
    entry->animation = animation;
    return true;
}

static void hd_object_piece_matrix(const HdObjectPiece *piece, float phase, MATRIXF *model, float projectionScale = 1)
{
    float mixed[3][4];
    hd_mix_transform(piece->previous, piece->current, &piece->previousTransform, &piece->currentTransform, phase, mixed);
    for (sint32 row = 0; row < 3; ++row)
    {
        for (sint32 column = 0; column < 3; ++column)
            model->m[row][column] = mixed[row][column] * projectionScale;
        model->t[row] = mixed[row][3];
    }
}

void hd_capture_objects(void)
{
    worldObjectSnapshotValid = false;
    if (!hd_objects_init())
        return;
    hd_world_object_pending = 0;
    hd_world_object_active = 0;
    hd_destruction_captured = 0;
    for (uint32 i = 0; i < worldObjectCount; ++i)
    {
        HdObjectEntry *entry = worldObjects + i;
        entry->previous = entry->current;
        hd_object_state(entry, &entry->current);
        if (!hd_capture_pieces(entry))
        {
            objectReset = true;
            return;
        }
        hd_destruction_captured += entry->pieceCount;
        if (objectReset || !entry->previous.active || entry->previous.type != entry->current.type || entry->previous.definition != entry->current.definition || entry->previous.orderOffset != entry->current.orderOffset || entry->previous.animationCycle != entry->current.animationCycle)
            entry->previous = entry->current;
        if (entry->current.active)
            ++hd_world_object_active;
    }
    objectReset = false;
    worldObjectSnapshotValid = true;
}

static bool hd_world_resources(void)
{
    if (!hd_lighting_resources())
        return false;
    hd_register_cleanup();
    if (worldShader == 0)
        worldShader = vid_shader_create(worldVertex, worldFragment);
    if (worldShader == 0)
        return false;
    if (worldVram == 0)
        worldVram = vid_tex_create_fmt(1024, 512, VID_RGBA5551);
    if (worldMaterials == 0)
        worldMaterials = vid_tex_create_fmt(256, 24, VID_RGBA16UI);
    if (worldPalettes == 0)
        worldPalettes = vid_tex_create_fmt(16, 256, VID_RGBA16UI);
    if (nightTables == 0)
        nightTables = vid_tex_create_fmt(256, 16, VID_RGBA16UI);
    if (nightShape == 0)
        nightShape = vid_tex_create_fmt(64, 64, VID_R8UI);
    if (nightColors == 0)
        nightColors = vid_tex_create_fmt(256, 1, VID_RGBA8);
    if (worldVram == 0 || worldMaterials == 0 || worldPalettes == 0 || nightTables == 0 || nightShape == 0 || nightColors == 0)
        return false;
    if (worldMaterial == 0)
    {
        worldMaterial = vid_mat_create();
        if (worldMaterial == 0)
            return false;
        VID_MATERIAL_DATA *material = vid_mat_data(worldMaterial);
        material->texture = worldVram;
        material->aux[0] = worldMaterials;
        material->aux[1] = worldPalettes;
        material->aux[2] = nightTables;
        material->aux[3] = nightShape;
        material->aux[4] = nightColors;
        material->aux[5] = hd_lighting_depth();
        material->aux[6] = hd_lighting_data();
        material->shader = worldShader;
        material->params[1] = 1;
        material->params[2] = 1;
        material->depth_test = 1;
        material->depth_write = 1;
        material->blend = VID_BLEND_NONE;
        material->filter = VID_FILTER_NEAREST;
        material->cull = 0;
        vid_mat_commit(worldMaterial);
    }
    if (worldBatchShader == 0)
        worldBatchShader = vid_shader_create(worldBatchVertex, worldFragment);
    if (worldBatchShader == 0)
        return false;
    if (worldBatchMaterial == 0)
    {
        worldBatchMaterial = vid_mat_create();
        if (worldBatchMaterial == 0)
            return false;
        *vid_mat_data(worldBatchMaterial) = *vid_mat_read(worldMaterial);
        vid_mat_data(worldBatchMaterial)->shader = worldBatchShader;
        vid_mat_commit(worldBatchMaterial);
    }
    const uint32 blends[4] = {VID_BLEND_ALPHA, VID_BLEND_ADD, VID_BLEND_SUBTRACT, VID_BLEND_ADD};
    for (sint32 blend = 0; blend < 4; ++blend)
        if (worldSemiMaterials[blend] == 0)
        {
            worldSemiMaterials[blend] = vid_mat_create();
            if (worldSemiMaterials[blend] == 0)
                return false;
            VID_MATERIAL_DATA *material = vid_mat_data(worldSemiMaterials[blend]);
            *material = *vid_mat_read(worldMaterial);
            material->params[0] = 2;
            material->params[1] = blend == 0 ? 0.5f : blend == 3 ? 0.25f : 1.0f;
            material->params[2] = 2;
            material->params[3] = (float)blend;
            material->blend = blends[blend];
            material->depth_write = 0;
            vid_mat_commit(worldSemiMaterials[blend]);
        }
    for (sint32 orientation = 0; orientation < 2; ++orientation)
        for (sint32 pass = 0; pass < 5; ++pass)
        {
            VID_MATERIAL **owner = pass == 0 ? worldObjectMaterials + orientation : &worldObjectSemiMaterials[orientation][pass - 1];
            if (*owner != 0)
                continue;
            *owner = vid_mat_create();
            if (*owner == 0)
                return false;
            VID_MATERIAL_DATA *material = vid_mat_data(*owner);
            *material = *vid_mat_read(pass == 0 ? worldMaterial : worldSemiMaterials[pass - 1]);
            material->cull = orientation + 1;
            material->depth_bias_slope = -1;
            material->depth_bias_units = -1;
            vid_mat_commit(*owner);
        }
    for (sint32 pass = 0; pass < 5; ++pass)
        if (worldTwoSidedMaterials[pass] == 0)
        {
            worldTwoSidedMaterials[pass] = vid_mat_create();
            if (worldTwoSidedMaterials[pass] == 0)
                return false;
            VID_MATERIAL_DATA *material = vid_mat_data(worldTwoSidedMaterials[pass]);
            *material = *vid_mat_read(pass == 0 ? worldMaterial : worldSemiMaterials[pass - 1]);
            material->depth_bias_slope = -1;
            material->depth_bias_units = -1;
            vid_mat_commit(worldTwoSidedMaterials[pass]);
        }
    for (sint32 orientation = 0; orientation < 2; ++orientation)
        if (staticWorldMaterials[orientation] == 0)
        {
            staticWorldMaterials[orientation] = vid_mat_create();
            if (staticWorldMaterials[orientation] == 0)
                return false;
            *vid_mat_data(staticWorldMaterials[orientation]) = *vid_mat_read(worldObjectMaterials[orientation]);
            vid_mat_data(staticWorldMaterials[orientation])->shader = worldBatchShader;
            vid_mat_commit(staticWorldMaterials[orientation]);
        }
    return true;
}

static bool hd_world_textures(void)
{
    hd_world_filter_regions = 0;
    sint32 count = Track::GetMaterialCount();
    sint32 capacity = Track::GetArtCapacity();
    if (count <= 0 || count > 1024 || Track::materials == 0 || gInitialArt.pPmx == 0)
        return false;
    const HdNightSource *night = &nightStates[0].source;
    uint16 *lighting = (uint16 *)vid_tex_data(nightTables);
    for (sint32 strength = 0; strength < 16; ++strength)
        for (sint32 color = 0; color < 256; ++color)
        {
            uint16 *entry = lighting + (strength * 256 + color) * 4;
            entry[0] = night->player[color][strength];
            entry[1] = night->police[0][color][strength & 7];
            entry[2] = night->police[1][color][strength & 7];
            entry[3] = strength < 2 ? night->weather[strength][color] : 0;
        }
    vid_tex_commit(nightTables);
    memcpy(vid_tex_data(nightShape), night->shape, sizeof(night->shape));
    vid_tex_commit(nightShape);
    memcpy(vid_tex_data(nightColors), night->colors, sizeof(night->colors));
    vid_tex_commit(nightColors);
    uint64 semiSignature = 1469598103934665603ULL;
    uint16 *data = (uint16 *)vid_tex_data(worldMaterials);
    for (sint32 i = 0; i < count; ++i)
    {
        const Track::tMaterial *material = Track::materials + i;
        semiSignature = (semiSignature ^ (uint32)material->pmxIndex) * 1099511628211ULL;
        if (material->pmxIndex >= 0 && material->pmxIndex < capacity)
            semiSignature = (semiSignature ^ (gInitialArt.pPmx[material->pmxIndex].flag & 2)) * 1099511628211ULL;
        for (sint32 mip = 0; mip < 2; ++mip)
        {
            sint32 index = material->pmxIndex;
            if (mip != 0 && (material->flag & 8) != 0)
                index += (uint8)material->mipmap_offset;
            if (index < 0 || index >= capacity)
                return false;
            const Draw::tPixMap *pmx = gInitialArt.pPmx + index;
            uint16 *out = data + i * 24 + mip * 12;
            out[0] = pmx->u0 | (pmx->v0 << 8);
            out[1] = pmx->u1 | (pmx->v1 << 8);
            out[2] = pmx->u2 | (pmx->v2 << 8);
            out[3] = pmx->u3 | (pmx->v3 << 8);
            out[4] = pmx->clut;
            out[5] = pmx->tpage;
            out[6] = pmx->pad2;
            out[7] = (pmx->flag & 255) | ((uint8)material->flag << 8);
            HdTextureRegion region = {0};
            if ((hd_texture_filtering_enabled & HD_FILTER_WORLD) != 0 && hd_texture_region(pmx, &region))
                ++hd_world_filter_regions;
            memcpy(out + 8, &region, sizeof(region));
            if (pmx->clut == 65535 && pmx->pad2 >= 256)
                return false;
        }
    }
    if (semiSignature != worldSemiSignature)
    {
        worldSemiSignature = semiSignature;
        ++worldSemiRevision;
    }
    vid_tex_commit(worldMaterials);
    uint16 *palettes = (uint16 *)vid_tex_data(worldPalettes);
    for (sint32 i = 0; i < 256 * 16; ++i)
    {
        palettes[i * 4] = ((const uint16 *)gClutDepth)[i];
        palettes[i * 4 + 1] = palettes[i * 4 + 2] = palettes[i * 4 + 3] = 0;
    }
    vid_tex_commit(worldPalettes);
    return hd_vram_texture() != 0;
}

VID_TEXTURE *hd_vram_texture(void)
{
    hd_register_cleanup();
    if (worldVram == 0)
        worldVram = vid_tex_create_fmt(1024, 512, VID_RGBA5551);
    if (worldVram == 0)
        return 0;
    uint32 version = gpu_vram_version();
    if (!worldVramValid || version != worldVramVersion)
    {
        uint16 *vram = (uint16 *)vid_tex_data(worldVram);
        for (uint32 row = 0; row < 512; ++row)
        {
            uint32 rowVersion = gpu_vram_row_version(row);
            if (!worldVramValid || rowVersion != worldVramRows[row])
            {
                memcpy(vram + row * 1024, VRAM + row * 1024, 1024 * sizeof(uint16));
                vid_tex_commit_rows(worldVram, row, 1);
                worldVramRows[row] = rowVersion;
            }
        }
        worldVramVersion = version;
        worldVramValid = true;
    }
    return worldVram;
}

static void hd_face_record(HdWorldChunk *mesh, VID_VERTEX16 *vertices, uint32 index, sint16 offset, bool sourceCulling = false)
{
    HdWorldFace *face = mesh->faces + index;
    memcpy(&face->center, vertices[index * 4].extra, sizeof(face->center));
    face->material = vertices[index * 4].tex[0];
    face->offset = offset;
    for (sint32 corner = 0; corner < 4; ++corner)
    {
        memcpy(face->points[corner], vertices[index * 4 + corner].pos, sizeof(face->points[corner]));
        float squared = 0;
        for (sint32 axis = 0; axis < 3; ++axis)
            squared += (float)face->points[corner][axis] * face->points[corner][axis];
        float radius = sqrtf(squared);
        if (radius > mesh->radius)
            mesh->radius = radius;
    }
    if (sourceCulling)
    {
        float edge[2][3];
        for (sint32 axis = 0; axis < 3; ++axis)
        {
            edge[0][axis] = (float)(face->points[1][axis] - face->points[0][axis]);
            edge[1][axis] = (float)(face->points[2][axis] - face->points[0][axis]);
        }
        VEC3F normal;
        normal.x = edge[0][1] * edge[1][2] - edge[0][2] * edge[1][1];
        normal.y = edge[0][2] * edge[1][0] - edge[0][0] * edge[1][2];
        normal.z = edge[0][0] * edge[1][1] - edge[0][1] * edge[1][0];
        for (sint32 corner = 0; corner < 4; ++corner)
        {
            VID_VERTEX16 *vertex = vertices + index * 4 + corner;
            memcpy(vertex->extra, &normal, sizeof(normal));
            vertex->tex[3] |= 32768;
        }
    }
}

static bool hd_source_quad(const CCOORD16 *source, uint32 sourceCount, const Trk::Quad *quad, VID_VERTEX16 *vertices, uint32 *indices, uint32 face)
{
    if (quad->material < 0 || quad->material >= Track::GetMaterialCount() || source == 0)
        return false;
    const uint8 order[4] = {1, 0, 2, 3};
    float center[3] = {0, 0, 0};
    for (sint32 i = 0; i < 4; ++i)
    {
        uint8 index = quad->aPoints[order[i]];
        if (index >= sourceCount || source[index].light < 0 || source[index].light >= Chunk::numLight)
            return false;
        center[0] += source[index].x * 0.25f;
        center[1] += source[index].y * 0.25f;
        center[2] += source[index].z * 0.25f;
    }
    for (sint32 i = 0; i < 4; ++i)
    {
        const CCOORD16 *point = source + quad->aPoints[order[i]];
        const CVECTOR *color = Chunk::lightTable + point->light;
        VID_VERTEX16 *vertex = vertices + face * 4 + i;
        memset(vertex, 0, sizeof(*vertex));
        vertex->pos[0] = point->x;
        vertex->pos[1] = point->y;
        vertex->pos[2] = point->z;
        vertex->pos[3] = 1;
        vertex->color[0] = color->r;
        vertex->color[1] = color->g;
        vertex->color[2] = color->b;
        vertex->color[3] = 255;
        memcpy(vertex->extra, center, sizeof(center));
        vertex->tex[0] = quad->material;
        vertex->tex[1] = i;
        vertex->tex[2] = (uint16)point->light;
        vertex->n[0] = source[quad->aPoints[1]].x;
        vertex->n[1] = source[quad->aPoints[1]].y;
        vertex->n[2] = source[quad->aPoints[1]].z;
    }
    const uint32 topology[6] = {0, 1, 2, 2, 1, 3};
    for (sint32 i = 0; i < 6; ++i)
        indices[face * 6 + i] = face * 4 + topology[i];
    return true;
}

static bool hd_world_quad(const Chunk *chunk, const Trk::Quad *quad, VID_VERTEX16 *vertices, uint32 *indices, uint32 face)
{
    const CCOORD16 *source = chunk->vertexBuf != 0 ? (const CCOORD16 *)(chunk->vertexBuf + 1) : 0;
    return hd_source_quad(source, chunk->renderVertexCount, quad, vertices, indices, face);
}

static bool hd_object_faces_overlap(const HdWorldFace *a, const HdWorldFace *b)
{
    float edge[2][3], normal[3];
    for (sint32 axis = 0; axis < 3; ++axis)
    {
        edge[0][axis] = (float)(a->points[1][axis] - a->points[0][axis]);
        edge[1][axis] = (float)(a->points[2][axis] - a->points[0][axis]);
    }
    for (sint32 axis = 0; axis < 3; ++axis)
        normal[axis] = edge[0][(axis + 1) % 3] * edge[1][(axis + 2) % 3] - edge[0][(axis + 2) % 3] * edge[1][(axis + 1) % 3];
    float length = sqrtf(normal[0] * normal[0] + normal[1] * normal[1] + normal[2] * normal[2]);
    if (!(length > 0))
        return false;
    sint32 dominant = 0;
    for (sint32 axis = 1; axis < 3; ++axis)
        if (fabsf(normal[axis]) > fabsf(normal[dominant]))
            dominant = axis;
    for (sint32 corner = 0; corner < 4; ++corner)
    {
        float distance = 0;
        for (sint32 axis = 0; axis < 3; ++axis)
            distance += normal[axis] * (b->points[corner][axis] - a->points[0][axis]);
        if (fabsf(distance) > length * 0.01f)
            return false;
    }
    for (sint32 axis = 0; axis < 3; ++axis)
        if (axis != dominant)
        {
            sint16 low[2] = {a->points[0][axis], b->points[0][axis]};
            sint16 high[2] = {low[0], low[1]};
            for (sint32 corner = 1; corner < 4; ++corner)
            {
                if (a->points[corner][axis] < low[0])
                    low[0] = a->points[corner][axis];
                if (a->points[corner][axis] > high[0])
                    high[0] = a->points[corner][axis];
                if (b->points[corner][axis] < low[1])
                    low[1] = b->points[corner][axis];
                if (b->points[corner][axis] > high[1])
                    high[1] = b->points[corner][axis];
            }
            if (low[0] >= high[1] || low[1] >= high[0])
                return false;
        }
    return true;
}

static bool hd_object_face_layers(HdWorldChunk *mesh, VID_VERTEX16 *vertices, const uint32 *indices, uint32 faces)
{
    uint32 layers[256] = {0};
    for (sint32 i = (sint32)faces - 1; i >= 0; --i)
    {
        HdWorldFace *face = mesh->faces + i;
        face->twoSided = (Track::materials[face->material].flag & 16) != 0;
        if (face->twoSided)
            mesh->twoSidedCount += 6;
        for (uint32 j = i + 1; j < faces; ++j)
            if (hd_object_faces_overlap(face, mesh->faces + j) && layers[i] <= layers[j])
                layers[i] = layers[j] + 1;
        for (sint32 corner = 0; corner < 4; ++corner)
            vertices[i * 4 + corner].extra[3] = (layers[i] < 8 ? layers[i] : 8) * 0.25f;
    }
    if (mesh->twoSidedCount == 0)
        return true;
    mesh->twoSidedIndices = vid_index_create(mesh->twoSidedCount);
    if (mesh->twoSidedIndices == 0)
        return false;
    uint32 *out = vid_index_data(mesh->twoSidedIndices);
    for (sint32 i = (sint32)faces - 1; i >= 0; --i)
        if (mesh->faces[i].twoSided)
        {
            memcpy(out, indices + i * 6, 6 * sizeof(*out));
            out += 6;
        }
    vid_index_commit(mesh->twoSidedIndices, mesh->twoSidedCount);
    vid_index_static(mesh->twoSidedIndices);
    return true;
}

static bool hd_definition_build(HdWorldChunk *mesh, const Trk::ObjectDef *definition)
{
    uint32 faces = definition->quadCount;
    if (faces == 0)
        return true;
    const CCOORD16 *source = (const CCOORD16 *)(definition + 1);
    const Trk::Quad *quads = (const Trk::Quad *)(source + definition->vertexCount);
    mesh->quadTopology = true;
    mesh->mesh = vid_mesh_create_16(faces * 4);
    mesh->indices = vid_index_create(faces * 6);
    mesh->faces = (HdWorldFace *)calloc(faces, sizeof(*mesh->faces));
    if (mesh->mesh == 0 || mesh->indices == 0 || mesh->faces == 0)
        return false;
    VID_VERTEX16 *vertices = vid_mesh_data_16(mesh->mesh);
    uint32 *indices = vid_index_data(mesh->indices);
    uint32 used = 0;
    for (uint32 i = 0; i < faces; ++i)
        if (hd_source_quad(source, definition->vertexCount, quads + i, vertices, indices, used))
        {
            hd_face_record(mesh, vertices, used, 0);
            ++used;
        }
        else
            ++hd_world_invalid_count;
    if (!hd_object_face_layers(mesh, vertices, indices, used))
        return false;
    mesh->count = used * 6;
    if (used != 0)
    {
        mesh->sourceVertices = (VID_VERTEX16 *)malloc(used * 4 * sizeof(VID_VERTEX16));
        if (mesh->sourceVertices == 0)
            return false;
        memcpy(mesh->sourceVertices, vertices, used * 4 * sizeof(VID_VERTEX16));
    }
    vid_mesh_commit(mesh->mesh);
    vid_index_commit(mesh->indices, mesh->count);
    vid_index_static(mesh->indices);
    return true;
}

static bool hd_world_build(void)
{
    if (worldChunks != 0)
        return true;
    sint32 count = Track::GetChunkCount();
    if (count <= 0 || Track::chunkList == 0 || Chunk::chunkCenters == 0 || Chunk::lightTable == 0)
        return false;
    worldChunks = (HdWorldChunk *)calloc(count, sizeof(*worldChunks));
    if (worldChunks == 0)
        return false;
    worldChunkCount = count;
    for (sint32 i = 0; i < count; ++i)
    {
        const Chunk *chunk = Track::chunkList + i;
        uint32 faces = chunk->quadCounts[5] + chunk->quadCounts[2] + chunk->quadCounts[3];
        const Trk::NewStrip *strip = chunk->stripBuf != 0 ? (const Trk::NewStrip *)(chunk->stripBuf + 1) : 0;
        sint32 strips = chunk->stripBuf != 0 ? chunk->stripBuf->m_num_elements : 0;
        const Trk::NewStrip *cursor = strip;
        for (sint32 j = 0; j < strips; ++j)
        {
            if ((uint8)cursor->size < sizeof(*cursor) + (uint8)cursor->quadCount * 2)
            {
                ++hd_world_invalid_count;
                hd_release_level();
                return false;
            }
            faces += (uint8)cursor->quadCount;
            cursor = (const Trk::NewStrip *)((const uint8 *)cursor + (uint8)cursor->size);
        }
        HdWorldChunk *world = worldChunks + i;
        world->terrain = true;
        world->quadTopology = true;
        hd_position(Chunk::chunkCenters + i, &world->center);
        if (faces == 0)
            continue;
        world->mesh = vid_mesh_create_16(faces * 4);
        world->indices = vid_index_create(faces * 6);
        world->faces = (HdWorldFace *)calloc(faces, sizeof(*world->faces));
        if (world->mesh == 0 || world->indices == 0 || world->faces == 0)
        {
            hd_release_level();
            return false;
        }
        VID_VERTEX16 *vertices = vid_mesh_data_16(world->mesh);
        uint32 *indices = vid_index_data(world->indices);
        uint32 used = 0;
        cursor = strip;
        for (sint32 j = 0; j < strips; ++j)
        {
            for (sint32 k = 0; k < (uint8)cursor->quadCount; ++k)
            {
                Trk::Quad quad;
                memcpy(&quad.material, (const uint8 *)(cursor + 1) + k * 2, sizeof(quad.material));
                quad.aPoints[0] = (uint8)(cursor->topVert + k + 1);
                quad.aPoints[1] = (uint8)(cursor->topVert + k);
                quad.aPoints[2] = (uint8)(cursor->botVert + k);
                quad.aPoints[3] = (uint8)(cursor->botVert + k + 1);
                if (hd_world_quad(chunk, &quad, vertices, indices, used))
                {
                    hd_face_record(world, vertices, used, 125);
                    ++used;
                }
                else
                    ++hd_world_invalid_count;
            }
            cursor = (const Trk::NewStrip *)((const uint8 *)cursor + (uint8)cursor->size);
        }
        for (uint32 j = 0; j < chunk->quadCounts[5]; ++j)
            if (hd_world_quad(chunk, chunk->renderQuads[3] + j, vertices, indices, used))
            {
                hd_face_record(world, vertices, used, 30);
                ++used;
            }
            else
                ++hd_world_invalid_count;
        world->surfaceCount = used;
        const Group *objectGroups[2] = {chunk->objQuadBuf, chunk->objQuadInstanceBuf};
        const CCOORD16 *objectVertices = chunk->objVertexBuf != 0 ? (const CCOORD16 *)(chunk->objVertexBuf + 1) : 0;
        for (sint32 group = 0; group < 2; ++group)
            for (uint32 j = 0; j < chunk->quadCounts[2 + group]; ++j)
            {
                const Trk::Quad *quad = objectGroups[group] != 0 ? (const Trk::Quad *)(objectGroups[group] + 1) + j : 0;
                if (quad != 0 && hd_source_quad(objectVertices, chunk->renderObjectVertexCount, quad, vertices, indices, used))
                {
                    hd_face_record(world, vertices, used, group == 0 ? 125 : 50, true);
                    ++used;
                }
                else
                    ++hd_world_invalid_count;
            }
        world->count = used * 6;
        hd_world_quad_count += used;
        vid_mesh_commit(world->mesh);
        vid_index_commit(world->indices, world->count);
    }
    worldDefinitionCount = gPersistObjDef != 0 ? gPersistObjDef->m_num_elements : 0;
    if (worldDefinitionCount > 0)
    {
        worldDefinitions = (HdWorldChunk *)calloc(worldDefinitionCount, sizeof(*worldDefinitions));
        if (worldDefinitions == 0)
        {
            worldDefinitionCount = 0;
            hd_release_level();
            return false;
        }
        for (sint32 i = 0; i < worldDefinitionCount; ++i)
            if (!hd_definition_build(worldDefinitions + i, Track::gObjDefs[i]))
            {
                hd_release_level();
                return false;
            }
    }
    return true;
}

static void hd_world_batch_vertex(VID_VERTEX3D *out, const VID_VERTEX16 *in, const VEC3F *offset)
{
    memset(out, 0, sizeof(*out));
    for (sint32 axis = 0; axis < 3; ++axis)
    {
        float translation = offset->v[axis];
        out->pos[axis] = in->pos[axis] + translation;
        out->n[axis] = in->n[axis] + translation;
        out->extra[axis] = in->extra[axis] + ((in->tex[3] & 32768) != 0 ? 0 : translation);
    }
    out->pos[3] = in->extra[3];
    for (sint32 channel = 0; channel < 4; ++channel)
        out->color[channel] = in->color[channel];
    out->uv[0] = (float)in->tex[0];
    out->uv[1] = (float)in->tex[1];
    out->n[3] = (float)in->tex[2];
    out->extra[3] = (float)in->tex[3];
}

static void hd_world_instance_vertex(VID_VERTEX3D *out, const VID_VERTEX16 *in, const MATRIXF *model, const VEC3F *origin, sint32 light, const CVECTOR *color)
{
    VEC3F zero = {0, 0, 0};
    hd_world_batch_vertex(out, in, &zero);
    for (sint32 row = 0; row < 3; ++row)
    {
        out->pos[row] = out->n[row] = model->t[row] - origin->v[row];
        out->extra[row] = (in->tex[3] & 32768) != 0 ? 0 : out->pos[row];
        for (sint32 axis = 0; axis < 3; ++axis)
        {
            out->pos[row] += model->m[row][axis] * in->pos[axis];
            out->n[row] += model->m[row][axis] * in->n[axis];
            float transform = model->m[row][axis];
            if ((in->tex[3] & 32768) != 0)
                transform = model->m[(row + 1) % 3][(axis + 1) % 3] * model->m[(row + 2) % 3][(axis + 2) % 3] - model->m[(row + 1) % 3][(axis + 2) % 3] * model->m[(row + 2) % 3][(axis + 1) % 3];
            out->extra[row] += transform * in->extra[axis];
        }
    }
    if (color != 0)
    {
        out->color[0] = color->r;
        out->color[1] = color->g;
        out->color[2] = color->b;
        out->n[3] = (float)light;
        out->extra[3] += 65536;
    }
}

static bool hd_world_batch_build(void)
{
    if (worldBatch.mesh != 0)
        return worldBatchReady;
    uint32 vertices = 0;
    for (sint32 chunk = 0; chunk < worldChunkCount; ++chunk)
    {
        uint32 count = worldChunks[chunk].count / 6 * 4;
        if (count > 0xffffffff - vertices)
            return false;
        vertices += count;
    }
    if (vertices == 0)
        return false;
    worldBatchStarts = (uint32 *)malloc(worldChunkCount * sizeof(*worldBatchStarts));
    worldBatch.mesh = vid_mesh_create_3d(vertices);
    worldBatchTopology = (uint32 *)malloc(vertices / 4 * 6 * sizeof(uint32));
    if (worldBatch.mesh == 0 || worldBatchStarts == 0 || worldBatchTopology == 0)
        return false;
    worldBatch.terrain = true;
    worldBatchOrigin = worldChunks[0].center;
    VID_VERTEX3D *target = vid_mesh_data_3d(worldBatch.mesh);
    uint32 used = 0;
    for (sint32 chunk = 0; chunk < worldChunkCount; ++chunk)
    {
        const HdWorldChunk *mesh = worldChunks + chunk;
        const VID_VERTEX16 *source = mesh->mesh != 0 ? vid_mesh_data_16(mesh->mesh) : 0;
        worldBatchStarts[chunk] = used;
        for (uint32 vertex = 0; vertex < mesh->count / 6 * 4; ++vertex)
        {
            VID_VERTEX3D *out = target + used++;
            const VID_VERTEX16 *in = source + vertex;
            VEC3F offset;
            for (sint32 axis = 0; axis < 3; ++axis)
                offset.v[axis] = mesh->center.v[axis] - worldBatchOrigin.v[axis];
            hd_world_batch_vertex(out, in, &offset);
        }
        if (mesh->count != 0)
        {
            const uint32 *indices = vid_index_data(mesh->indices);
            for (uint32 index = 0; index < mesh->count; ++index)
                worldBatchTopology[worldBatch.count + index] = indices[index] + worldBatchStarts[chunk];
            vid_mesh_static(mesh->mesh);
            vid_index_static(mesh->indices);
        }
        worldBatch.count += mesh->count;
    }
    vid_mesh_commit(worldBatch.mesh);
    vid_mesh_static(worldBatch.mesh);
    for (sint32 bank = 0; bank < 8; ++bank)
    {
        worldBatchIndices[bank] = vid_index_create(worldBatch.count);
        if (worldBatchIndices[bank] == 0)
            return false;
    }
    worldBatchReady = true;
    return true;
}

static float effectFogUniforms[18][4];
static bool effectFogActive;

static void hd_fog_uniforms(float uniforms[18][4], const HdViewPose *view, VID_RECT viewport, float logicalHeight, VID_MATERIAL *material = 0)
{
    for (sint32 axis = 0; axis < 3; ++axis)
        uniforms[15][axis] = view->fogColor[axis];
    float strength = view->fogStrength;
    if (!(strength > 0))
        strength = 0;
    if (strength > 1)
        strength = 1;
    uniforms[15][3] = strength;
    uniforms[16][0] = view->fogStart >= 0 ? view->fogStart * 8 : 0;
    uniforms[16][1] = view->fogScale / 120;
    uniforms[16][2] = view->fogEnabled && strength > 0 && view->fogScale > 0 ? 1.0f : 0.0f;
    const VID_MATERIAL_DATA *data = material != 0 ? vid_mat_read(material) : 0;
    uniforms[16][3] = data != 0 && (data->blend == VID_BLEND_ADD || data->blend == VID_BLEND_SUBTRACT) ? 1.0f : 0.0f;
    uniforms[17][0] = 2 * view->projection / (logicalHeight * viewport.w / viewport.h) * (view->mirror ? -1 : 1);
    uniforms[17][1] = 2 * view->projection * (56320.0f / 65536) / logicalHeight;
}

static void hd_world_facing_uniforms(float uniforms[18][4])
{
    float cofactors[3][3];
    for (sint32 row = 0; row < 3; ++row)
        for (sint32 axis = 0; axis < 3; ++axis)
            cofactors[row][axis] = uniforms[(row + 1) % 3][(axis + 1) % 3] * uniforms[(row + 2) % 3][(axis + 2) % 3] - uniforms[(row + 1) % 3][(axis + 2) % 3] * uniforms[(row + 2) % 3][(axis + 1) % 3];
    float determinant = 0;
    for (sint32 axis = 0; axis < 3; ++axis)
        determinant += uniforms[0][axis] * cofactors[0][axis];
    for (sint32 axis = 0; axis < 3; ++axis)
    {
        float offset = 0;
        for (sint32 row = 0; row < 3; ++row)
            offset += cofactors[row][axis] * uniforms[row][3];
        uniforms[17][axis] = fabsf(determinant) > 0.000000000001f ? offset / determinant : 0;
    }
    uniforms[17][3] = fabsf(determinant) <= 0.000000000001f ? 0 : determinant > 0 ? 1.0f : -1.0f;
}

static void hd_effect_uniforms(VID_MATERIAL *material, float uniforms[18][4])
{
    memcpy(uniforms, effectFogUniforms, sizeof(effectFogUniforms));
    if (!effectFogActive)
        uniforms[16][2] = 0;
    const VID_MATERIAL_DATA *data = vid_mat_read(material);
    uniforms[16][3] = data->blend == VID_BLEND_ADD || data->blend == VID_BLEND_SUBTRACT ? 1.0f : 0.0f;
}

static void hd_effect_pass(VID_MATERIAL *material)
{
    vid_pass(material);
    if (weatherShader != 0 && vid_mat_read(material)->shader == weatherShader)
    {
        float uniforms[18][4];
        hd_effect_uniforms(material, uniforms);
        vid_uniforms(&uniforms[0][0], 18);
    }
    else
        vid_uniforms(0, 0);
}

static void hd_order_draw(const HdOrderKey *key, VID_MATERIAL *material, VID_GEOMETRY *mesh, VID_INDEX *indices, uint32 first, uint32 count, const float *uniforms = 0, uint32 uniformCount = 0, bool quadTopology = false)
{
    if (!worldSubmissionValid)
        return;
    float effectUniforms[18][4];
    if (uniformCount == 0 && material != 0 && weatherShader != 0 && vid_mat_read(material)->shader == weatherShader)
    {
        hd_effect_uniforms(material, effectUniforms);
        uniforms = &effectUniforms[0][0];
        uniformCount = 18;
    }
    if (uniformCount > 32 || (uniformCount != 0 && uniforms == 0))
    {
        worldSubmissionValid = false;
        return;
    }
    if (orderedCount == orderedCapacity)
    {
        uint32 capacity = orderedCapacity != 0 ? orderedCapacity * 2 : 256;
        if (capacity < orderedCapacity || capacity > 0xffffffff / sizeof(*orderedDraws))
        {
            worldSubmissionValid = false;
            return;
        }
        HdOrderedDraw *draws = (HdOrderedDraw *)realloc(orderedDraws, capacity * sizeof(*draws));
        if (draws == 0)
        {
            worldSubmissionValid = false;
            return;
        }
        orderedDraws = draws;
        orderedCapacity = capacity;
    }
    HdOrderedDraw *draw = orderedDraws + orderedCount++;
    draw->key = *key;
    draw->material = material;
    draw->mesh = mesh;
    draw->indices = indices;
    draw->first = first;
    draw->count = count;
    draw->uniformCount = uniformCount;
    draw->sequence = orderedCount - 1;
    draw->captureFirst = 0;
    draw->quadTopology = quadTopology;
    if (uniformCount != 0)
        memcpy(draw->uniforms, uniforms, uniformCount * sizeof(draw->uniforms[0]));
}

static int hd_order_compare(const void *left, const void *right)
{
    const HdOrderedDraw *a = (const HdOrderedDraw *)left;
    const HdOrderedDraw *b = (const HdOrderedDraw *)right;
    if (a->key.bucket != b->key.bucket)
        return a->key.bucket > b->key.bucket ? -1 : 1;
    if (a->key.family != b->key.family)
        return a->key.family > b->key.family ? -1 : 1;
    if (a->key.group != b->key.group)
        return a->key.group > b->key.group ? -1 : 1;
    if (a->key.object != b->key.object)
        return a->key.object > b->key.object ? -1 : 1;
    if (a->key.piece != b->key.piece)
        return a->key.piece > b->key.piece ? -1 : 1;
    if (a->key.face != b->key.face)
        return a->key.face > b->key.face ? -1 : 1;
    if (a->key.pass != b->key.pass)
        return a->key.pass < b->key.pass ? -1 : 1;
    return a->sequence < b->sequence ? -1 : a->sequence > b->sequence ? 1 : 0;
}

static void hd_order_flush(void)
{
    if (worldSubmissionValid)
    {
        if (orderedCount > 1)
            qsortPSX(orderedDraws, orderedCount, sizeof(*orderedDraws), hd_order_compare);
        uint32 bank = cubeRendering ? cubeFace : 6 + lightingView;
        {
            uint32 needed = 0;
            for (uint32 index = 0; index < orderedCount; ++index)
                if ((orderedDraws[index].indices == 0 || orderedDraws[index].quadTopology) && orderedDraws[index].count % 3 == 0)
                {
                    if (orderedDraws[index].count > 0xffffffff - needed || orderedDraws[index].count > 0xffffffff - orderedDraws[index].first)
                    {
                        worldSubmissionValid = false;
                        break;
                    }
                    needed += orderedDraws[index].count;
                }
            if (needed > cubeOrderCapacity[bank])
            {
                vid_index_free(cubeOrderIndices[bank]);
                cubeOrderIndices[bank] = vid_index_create(needed);
                cubeOrderCapacity[bank] = cubeOrderIndices[bank] != 0 ? needed : 0;
                cubeOrderCount[bank] = 0;
            }
            uint32 *indices = needed != 0 ? vid_index_data(cubeOrderIndices[bank]) : 0;
            if (needed != 0 && indices == 0)
                worldSubmissionValid = false;
            uint32 used = 0;
            bool changed = false;
            for (uint32 index = 0; index < orderedCount && worldSubmissionValid; ++index)
            {
                HdOrderedDraw *draw = orderedDraws + index;
                if ((draw->indices != 0 && !draw->quadTopology) || draw->count % 3 != 0)
                    continue;
                draw->captureFirst = used;
                for (uint32 vertex = 0; vertex < draw->count; ++vertex)
                {
                    uint32 source = draw->first + vertex;
                    const uint32 topology[6] = {0, 1, 2, 2, 1, 3};
                    uint32 value = draw->quadTopology ? source / 6 * 4 + topology[source % 6] : source;
                    if (indices[used] != value)
                        changed = true;
                    indices[used++] = value;
                }
            }
            if (used != 0 && (changed || used != cubeOrderCount[bank]))
            {
                vid_index_commit(cubeOrderIndices[bank], used);
                cubeOrderCount[bank] = used;
            }
        }
        for (uint32 index = 0; index < orderedCount && worldSubmissionValid; ++index)
        {
            const HdOrderedDraw *draw = orderedDraws + index;
            uint32 count = draw->count;
            bool captured = (draw->indices == 0 || draw->quadTopology) && count % 3 == 0;
            VID_INDEX *indices = captured ? cubeOrderIndices[bank] : draw->indices;
            uint32 first = captured ? draw->captureFirst : draw->first;
            while (index + 1 < orderedCount && count % 3 == 0 && count <= 0xffffffff - first)
            {
                const HdOrderedDraw *next = orderedDraws + index + 1;
                uint32 nextFirst = captured ? next->captureFirst : next->first;
                if (next->count % 3 != 0 || next->quadTopology != draw->quadTopology || next->material != draw->material || next->mesh != draw->mesh || next->indices != draw->indices || nextFirst != first + count || next->count > 0xffffffff - count || next->uniformCount != draw->uniformCount || memcmp(next->uniforms, draw->uniforms, draw->uniformCount * 4 * sizeof(float)) != 0)
                    break;
                count += next->count;
                ++index;
            }
            vid_pass(draw->material);
            vid_uniforms(&draw->uniforms[0][0], draw->uniformCount);
            worldSubmissionValid = vid_submit(draw->mesh, indices, first, count) != 0;
            ++hd_ordered_draws;
        }
    }
    orderedCount = 0;
}

static void hd_submit(VID_GEOMETRY *mesh, VID_INDEX *indices, uint32 first, uint32 count)
{
    worldSubmissionValid = vid_submit(mesh, indices, first, count) != 0 && worldSubmissionValid;
}

static float hd_car_sun_fade(sint32 slot, float phase)
{
    if (slot < 0 || slot >= 9 || !carModels[slot].active)
        return 1;
    const HdCarModel *car = carModels + slot;
    return car->sunPrevious + (car->sunCurrent - car->sunPrevious) * phase;
}

static void hd_world_submit(const HdWorldChunk *mesh, const MATRIXF *model, const MATRIXF *camera, const HdViewPose *view, VID_RECT viewport, float logicalHeight, sint16 light, sint32 rgb = -1, sint32 materialRow = 0, sint32 effect = -1, float yawCos = 1, float yawSin = 0, uint32 first = 0, uint32 materialFlags = 0, float mipScale = 1, const HdOrderKey *order = 0, VID_MATERIAL *material = 0, bool shadowReceiver = true)
{
    if (mesh->count == 0)
        return;
    float uniforms[32][4] = {0};
    hd_fog_uniforms(uniforms, view, viewport, logicalHeight, material);
    const VID_MATERIAL_DATA *lightingMaterial = material != 0 ? vid_mat_read(material) : 0;
    bool receiver = effect <= 0 && (lightingMaterial == 0 || lightingMaterial->blend == VID_BLEND_NONE || lightingMaterial->blend == VID_BLEND_ALPHA);
    hd_lighting_uniforms(uniforms, model, lightingView, receiver);
    uniforms[21][2] = shadowReceiver ? 0.0f : 1.0f;
    bool carSurface = lightingMaterial != 0 && (lightingMaterial->shader == carShader || lightingMaterial->shader == carCubeShader || lightingMaterial->shader == carDepthShader);
    float sunFade = carSurface ? hd_car_sun_fade(materialRow, carRenderPhase) : 1;
    memcpy(uniforms[26], camera->t, 3 * sizeof(float));
    memcpy(uniforms[29], &cubeOrigin, 3 * sizeof(float));
    uniforms[29][3] = 2.0f / 256;
    uniforms[30][0] = 1;
    uniforms[30][1] = 65535;
    uniforms[31][0] = 1;
    uniforms[31][1] = 0.25f;
    uniforms[31][2] = 0.03f;
    uniforms[31][3] = 0.0001f;
    float sunLength = sqrtf((float)skyCurrent.sun.vx * skyCurrent.sun.vx + (float)skyCurrent.sun.vy * skyCurrent.sun.vy + (float)skyCurrent.sun.vz * skyCurrent.sun.vz);
    if (sunLength > 0 && GameSetup::gData.Time == 0)
    {
        uniforms[27][0] = skyCurrent.sun.vx / sunLength;
        uniforms[27][1] = skyCurrent.sun.vy / sunLength;
        uniforms[27][2] = skyCurrent.sun.vz / sunLength;
        uniforms[27][3] = effect == 0 ? sunFade : 0.0f;
        uniforms[28][0] = skyCurrent.sunBeamColor.r / 128.0f;
        uniforms[28][1] = skyCurrent.sunBeamColor.g / 128.0f;
        uniforms[28][2] = skyCurrent.sunBeamColor.b / 128.0f;
    }
    if (lightingCapture != 0)
    {
        uniforms[21][0] = 0;
        uniforms[21][3] = 1;
        memcpy(uniforms + 22, lightingCapture->rows, sizeof(lightingCapture->rows));
    }
    for (sint32 row = 0; row < 3; ++row)
    {
        for (sint32 column = 0; column < 3; ++column)
            for (sint32 axis = 0; axis < 3; ++axis)
                uniforms[row][column] += camera->m[axis][row] * model->m[axis][column];
        for (sint32 axis = 0; axis < 3; ++axis)
            uniforms[row][3] += camera->m[axis][row] * (model->t[axis] - camera->t[axis]);
    }
    hd_world_facing_uniforms(uniforms);
    uniforms[3][0] = 2 * view->projection / (logicalHeight * viewport.w / viewport.h);
    if (view->mirror)
        uniforms[3][0] = -uniforms[3][0];
    uniforms[3][1] = 2 * view->projection * (56320.0f / 65536) / logicalHeight;
    uniforms[3][2] = 65536.0f / 65534;
    uniforms[3][3] = -131070.0f / 65534;
    if (cubeRendering)
    {
        uniforms[3][0] = 1;
        uniforms[3][1] = -1;
    }
    uniforms[4][0] = view->fogStart;
    uniforms[4][1] = view->fogScale;
    uniforms[4][2] = view->fogEnabled ? 1.0f : 0.0f;
    uniforms[4][3] = (float)materialRow;
    uniforms[14][3] = mipScale;
    if (light >= 0 && light < Chunk::numLight)
    {
        const CVECTOR *color = Chunk::lightTable + light;
        uniforms[5][0] = color->r / 255.0f;
        uniforms[5][1] = color->g / 255.0f;
        uniforms[5][2] = color->b / 255.0f;
        uniforms[5][3] = 1;
    }
    if (rgb >= 0)
    {
        uniforms[5][0] = (rgb & 255) / 255.0f;
        uniforms[5][1] = ((rgb >> 8) & 255) / 255.0f;
        uniforms[5][2] = ((rgb >> 16) & 255) / 255.0f;
        uniforms[5][3] = 1;
    }
    uniforms[6][0] = (float)effect;
    uniforms[6][1] = yawCos;
    uniforms[6][2] = yawSin;
    uniforms[6][3] = (float)materialFlags;
    uniforms[14][2] = mesh->line ? 1.0f : 0.0f;
    if (effect < 0 && !mesh->noNight && renderNightSource != 0 && hd_night_world_enabled != 0)
    {
        const HdNightSource *source = renderNightSource;
        uniforms[6][0] = (float)light;
        uniforms[6][1] = source->headlights && hd_headlights_enabled == 0 ? 1.0f : 0.0f;
        uniforms[6][2] = source->cop != 0 && hd_night_police_enabled != 0 ? 1.0f : 0.0f;
        for (sint32 which = 0; which < 2; ++which)
        {
            const MATRIXF *basis = which == 0 ? &renderNightTarget : &renderNightCop;
            if (which == 1 && source->cop == 0)
                continue;
            float distance = 0;
            for (sint32 axis = 0; axis < 3; ++axis)
            {
                float delta = model->t[axis] - basis->t[axis];
                if (axis != 1)
                    distance += delta * delta;
                for (sint32 row = 0; row < 3; ++row)
                {
                    uniforms[7 + which * 3 + row][axis] = basis->m[row][axis];
                    uniforms[7 + which * 3 + row][3] += basis->m[row][axis] * delta;
                }
            }
            if (mesh->terrain && distance >= (float)0x47E0000)
                uniforms[6][1 + which] = 0;
        }
        uniforms[13][0] = (float)source->nearZ;
        uniforms[13][1] = (float)((uint32)1 << source->xShift);
        uniforms[13][2] = (float)((uint32)1 << source->zShift);
        uniforms[13][3] = source->lightning && hd_night_lightning_enabled != 0 ? 1.0f : 0.0f;
        uniforms[14][0] = (float)source->lightningType;
        uniforms[14][1] = 1;
        if (uniforms[6][1] != 0 || uniforms[6][2] != 0)
            ++hd_night_draws;
    }
    else if (effect < 0)
        uniforms[6][0] = (float)light;
    if (order != 0)
        hd_order_draw(order, material, mesh->mesh, mesh->indices, first, mesh->count, &uniforms[0][0], 32, mesh->quadTopology);
    else
    {
        vid_uniforms(&uniforms[0][0], 32);
        hd_submit(mesh->mesh, mesh->indices, first, mesh->count);
    }
}

static float hd_face_depth(const sint16 point[3], const MATRIXF *model, const MATRIXF *camera)
{
    float depth = 0;
    for (sint32 axis = 0; axis < 3; ++axis)
    {
        float position = model->t[axis] - camera->t[axis];
        for (sint32 column = 0; column < 3; ++column)
            position += model->m[axis][column] * point[column];
        depth += camera->m[axis][2] * position;
    }
    return depth;
}

static bool hd_cube_mesh_visible(const HdWorldChunk *mesh, const MATRIXF *model, const MATRIXF *camera)
{
    float scale = 0;
    for (sint32 row = 0; row < 3; ++row)
        for (sint32 axis = 0; axis < 3; ++axis)
            scale += model->m[row][axis] * model->m[row][axis];
    float radius = (mesh->radius > 0 ? mesh->radius : 1024) * sqrtf(scale);
    float center[3] = {0};
    for (sint32 row = 0; row < 3; ++row)
        for (sint32 axis = 0; axis < 3; ++axis)
            center[row] += camera->m[axis][row] * (model->t[axis] - camera->t[axis]);
    return center[2] + radius >= 1 && center[2] - radius <= 65535 && fabsf(center[0]) <= center[2] + radius * 1.414214f && fabsf(center[1]) <= center[2] + radius * 1.414214f;
}

static bool hd_world_geometry(const HdWorldChunk *mesh, const MATRIXF *model, const MATRIXF *camera, const HdViewPose *view, VID_RECT viewport, float logicalHeight, sint16 light, sint32 offset, uint32 group, uint32 object, uint32 piece, bool shadowReceiver = true, bool opaque = true)
{
    if (cubeRendering && !hd_cube_mesh_visible(mesh, model, camera))
        return true;
    float mipScale = offset == -1 ? 0.25f : 1.0f;
    if (opaque)
    {
        vid_pass(mesh->terrain ? worldMaterial : worldObjectMaterials[view->mirror != 0 ? 1 : 0]);
        hd_world_submit(mesh, model, camera, view, viewport, logicalHeight, light, -1, 0, -1, 1, 0, 0, 0, mipScale, 0, 0, shadowReceiver);
        if (!mesh->terrain && mesh->twoSidedCount != 0)
        {
            HdWorldChunk sheets = *mesh;
            sheets.indices = mesh->twoSidedIndices;
            sheets.count = mesh->twoSidedCount;
            vid_pass(worldObjectMaterials[view->mirror != 0 ? 0 : 1]);
            hd_world_submit(&sheets, model, camera, view, viewport, logicalHeight, light, -1, 0, -1, 1, 0, 0, 0, mipScale, 0, 0, shadowReceiver);
        }
    }
    if (hd_world_semi_enabled == 0)
        return true;
    HdWorldChunk *cache = const_cast<HdWorldChunk *>(mesh);
    if (cache->semiRevision != worldSemiRevision)
    {
        if (cache->semiFaces == 0 && mesh->count != 0)
            cache->semiFaces = (uint32 *)malloc(mesh->count / 6 * sizeof(uint32));
        if (cache->semiFaces == 0 && mesh->count != 0)
            return false;
        cache->semiCount = 0;
        for (uint32 index = 0; index < mesh->count / 6; ++index)
        {
            const HdWorldFace *face = mesh->faces + index;
            if (face->material >= Track::GetMaterialCount())
                return false;
            sint32 pmx = Track::materials[face->material].pmxIndex;
            if (pmx < 0 || pmx >= Track::GetArtCapacity())
                return false;
            if ((gInitialArt.pPmx[pmx].flag & 2) != 0)
                cache->semiFaces[cache->semiCount++] = index;
        }
        cache->semiRevision = worldSemiRevision;
    }
    for (uint32 item = 0; item < cache->semiCount; ++item)
    {
        uint32 index = cache->semiFaces[item];
        const HdWorldFace *face = mesh->faces + index;
        if (face->material >= Track::GetMaterialCount())
            return false;
        const Track::tMaterial *material = Track::materials + face->material;
        sint32 pmxIndex = material->pmxIndex;
        if (pmxIndex < 0 || pmxIndex >= Track::GetArtCapacity())
            return false;
        if ((gInitialArt.pPmx[pmxIndex].flag & 2) == 0)
            continue;
        float z = 0;
        float center[3] = {face->center.x, face->center.y, face->center.z};
        for (sint32 axis = 0; axis < 3; ++axis)
        {
            float position = model->t[axis] - camera->t[axis];
            for (sint32 column = 0; column < 3; ++column)
                position += model->m[axis][column] * center[column];
            z += camera->m[axis][2] * position;
        }
        const Draw::tPixMap *pmx = gInitialArt.pPmx + pmxIndex;
        if ((pmx->flag & 2) == 0)
            continue;
        sint32 orderOffset = mesh->terrain ? face->offset : offset;
        if (orderOffset == -2)
            return false;
        float sum = 0;
        for (sint32 corner = 0; corner < 4; ++corner)
        {
            float depth = hd_face_depth(face->points[corner], model, camera) * mipScale;
            if (!(depth >= 0))
                depth = 0;
            if (depth > 65535)
                depth = 65535;
            sum += floorf(depth);
        }
        sint32 otz = (sint32)floorf(sum / 16);
        sint32 bucket = orderOffset == -1 ? (otz >> 4) + view->orderTableSize * 7 / 8 : (otz >> 1) + orderOffset;
        if (bucket <= 0 || bucket > view->orderTableSize - 3)
            continue;
        if (worldSemiCount == worldSemiCapacity)
        {
            uint32 capacity = worldSemiCapacity != 0 ? worldSemiCapacity * 2 : 256;
            if (capacity < worldSemiCapacity || capacity > 0xffffffff / sizeof(*worldSemiCommands))
                return false;
            HdWorldSemiCommand *commands = (HdWorldSemiCommand *)realloc(worldSemiCommands, capacity * sizeof(*commands));
            if (commands == 0)
                return false;
            worldSemiCommands = commands;
            worldSemiCapacity = capacity;
        }
        HdWorldSemiCommand *command = worldSemiCommands + worldSemiCount++;
        command->mesh = mesh;
        command->model = *model;
        command->face = index;
        command->group = group;
        command->object = object;
        command->piece = piece;
        command->order = worldSemiCount - 1;
        command->bucket = bucket;
        command->light = light;
        command->blend = (pmx->tpage >> 5) & 3;
        command->midground = offset == -1;
        command->shadowReceiver = shadowReceiver;
    }
    return true;
}

static int hd_world_semi_compare(const void *left, const void *right)
{
    const HdWorldSemiCommand *a = (const HdWorldSemiCommand *)left;
    const HdWorldSemiCommand *b = (const HdWorldSemiCommand *)right;
    if (a->bucket != b->bucket)
        return a->bucket > b->bucket ? -1 : 1;
    if (a->group != b->group)
        return a->group > b->group ? -1 : 1;
    if (a->object != b->object)
        return a->object > b->object ? -1 : 1;
    if (a->piece != b->piece)
        return a->piece > b->piece ? -1 : 1;
    if (a->face != b->face)
        return a->face > b->face ? -1 : 1;
    return a->order > b->order ? -1 : a->order < b->order ? 1 : 0;
}

static void hd_world_semi_view(const MATRIXF *camera, const HdViewPose *view, VID_RECT viewport, float logicalHeight)
{
    if (worldSemiCount > 1)
        qsortPSX(worldSemiCommands, worldSemiCount, sizeof(*worldSemiCommands), hd_world_semi_compare);
    for (uint32 index = 0; index < worldSemiCount; ++index)
    {
        const HdWorldSemiCommand *command = worldSemiCommands + index;
        HdWorldChunk mesh = *command->mesh;
        mesh.count = 6;
        HdOrderKey order = {command->bucket, mesh.terrain ? HD_ORDER_TERRAIN : HD_ORDER_OBJECTS, command->group, command->object, command->piece, command->face, 0};
        hd_world_submit(&mesh, &command->model, camera, view, viewport, logicalHeight, command->light, -1, 0, -1, 1, 0, command->face * 6, 0, command->midground ? 0.25f : 1, &order, mesh.terrain ? worldSemiMaterials[command->blend] : mesh.faces[command->face].twoSided ? worldTwoSidedMaterials[command->blend + 1] : worldObjectSemiMaterials[view->mirror != 0 ? 1 : 0][command->blend], command->shadowReceiver);
        ++hd_world_semi_draws;
    }
}

static uint32 hd_world_group(const HdViewPose *view, sint32 chunk)
{
    sint32 source = view->world.chunk;
    if (source >= 0 && source < worldChunkCount && Track::gInViewCount != 0 && Track::gInViewList != 0 && Track::gInViewCount[source] <= 36)
        for (uint32 index = 0; index < Track::gInViewCount[source]; ++index)
            if ((((const sint16 *)Track::gInViewList)[source * 32 + index] & 1023) == chunk)
                return index;
    return 36 + chunk;
}

static const char carVertex[] = "layout(location=0) in vec4 aPos; layout(location=1) in vec2 aUV; layout(location=2) in vec4 aColor;\n"
                                "layout(location=3) in vec4 aN; layout(location=4) in vec4 aExtra; layout(location=5) in uvec4 aTex; uniform vec4 uDraw[32]; uniform highp usampler2D uAux0;\n" HD_LIGHTING_VERTEX_GLSL HD_CAR_VERTEX_OUTPUT_GLSL "out vec2 vUV; out vec4 vColor; out float vDepth; out vec2 vFog;out vec3 vAtmospherePosition; flat out uvec4 vCarTex; flat out uvec2 vCarWindow;flat out ivec4 vRegion;\n"
                                "uvec4 meta(uint id){return texelFetch(uAux0,ivec2(int(id),int(uDraw[4].w)),0); }\n"
                                "void main(){vec4 p=vec4(aPos.xyz,1.0);vec3 c=vec3(dot(uDraw[0],p),dot(uDraw[1],p),dot(uDraw[2],p));\n"
                                "uint index=aTex.x;uint mode=aTex.z&9u;uint flags=0u;vec2 offset=vec2(0.0);uvec4 settings=meta(193u);\n"
                                "if((mode&8u)!=0u){uint word=meta(141u+index).x;flags=(aTex.y&32768u)!=0u?word>>8:word&255u;\n"
                                "if((aTex.y&1008u)!=0u){flags&=(aTex.y&4095u)>>4;if(flags!=0u){while((flags&3u)==0u)flags>>=2;}}}\n"
                                "uint which=flags&3u;if(which!=0u){uint map=index*3u+which-1u;if(map<90u){uvec4 o=meta(51u+map);\n"
                                "int signedOffset=int(o.x);if(signedOffset>=32768)signedOffset-=65536;index=uint(int(index)+signedOffset);offset=vec2(o.yz); }else index=65535u;}\n"
                                "uvec4 info=index<settings.z?meta(index):uvec4(0u);\n"
                                "if(which==0u&&mode==0u){uvec4 uv=meta(192u);if((aTex.z&2u)!=0u){if((aTex.y&1u)==0u||uDraw[6].w>0.5)offset=vec2(uv.xy); }\n"
                                "else if((info.z&127u)!=0u)offset=(info.z&2u)!=0u?vec2(settings.xy):vec2(uv.zw); }\n"
                                "vUV=vec2(uvec2(aUV+offset)&uvec2(255u))+vec2(0.5);vColor=vec4(mode==8u&&(flags&128u)!=0u?vec3(128.0/255.0):uDraw[5].rgb,1.0);\n"
                                "uint semi=mode==0u?(aTex.z&2u)!=0u?aTex.y&1u:0u:(mode&8u)!=0u?aTex.y&1u:0u;\n"
                                "if(uDraw[6].w>0.5&&mode==0u)semi=0u;vReflective=(aTex.z&1u)!=0u&&(info.z&3u)==0u?1.0:0.0;vLocalNormal=aExtra.xyz;vEnvironmentNormal=aN.xyz;vCarFlags=uvec3(mode,flags,aTex.y);\n"
                                "vDepth=c.z;vFog=vec2(0.0);vAtmospherePosition=c;vCarTex=uvec4(info.xy,semi,index<settings.z?1u:0u);vRegion=index<settings.z?ivec4(meta(198u+index)):ivec4(0);\n"
                                "if(uDraw[6].x>2.5){uvec4 map=meta(197u);vUV=aUV+vec2(0.5);vColor=aColor/255.0;vCarTex=uvec4(map.xy,2u,1u); }\n"
                                "float decal=uDraw[6].w>0.5?0.0:max(aExtra.w,(info.z&3u)!=0u?0.25:0.0);float biased=max(c.z-decal,0.0001);"
                                "float clipZ=decal>0.0?(biased*uDraw[3].z+uDraw[3].w)*c.z/biased:c.z*uDraw[3].z+uDraw[3].w;"
                                "gl_Position=vec4(c.x*uDraw[3].x,c.y*uDraw[3].y,clipZ,c.z);hdLightingVertex(aPos.xyz); }\n";

#define HD_CAR_FRAGMENT_GLSL(environment) \
    "flat in uvec4 vCarTex;flat in ivec4 vRegion;flat in uvec3 vCarFlags;smooth in vec3 vEnvironmentNormal;uvec2 carWindow,carOrigin;\n" \
    "#define vWindow carWindow\n#define vWindowOrigin carOrigin\n" HD_BOUNDED_WINDOW_GLSL VID_PSX_SAMPLING_GLSL HD_VRAM_WORD_GLSL HD_PSX_COLOR_GLSL "uniform vec4 uDraw[32];uniform highp usampler2D uAux0;in vec3 vAtmospherePosition;\n" HD_FOG_BODY_GLSL HD_LIGHTING_FRAGMENT_GLSL HD_CAR_SUN_GLSL environment "uvec4 hdCarMeta(uint id){return texelFetch(uAux0,ivec2(int(id),int(uDraw[4].w)),0);}\n" \
    "void main(){carWindow=uvec2(0u);carOrigin=uvec2(0u);hdLightingPrepareNormal(hdCarNormal());uvec4 tex=vCarTex;vec2 uv=vUV;vec3 tint=vColor.rgb;ivec4 region=vRegion;\n" \
    "if(uDraw[6].x>0.5){uint pass=uint(uDraw[6].x);region=ivec4(hdCarMeta(249u+pass-1u));if(pass<=2u){uvec4 map=hdCarMeta(pass==1u?194u:195u),colors=hdCarMeta(196u);" \
    "uint mode=vCarFlags.x,flags=vCarFlags.y,facet=vCarFlags.z;bool enabled=(mode&1u)!=0u&&(map.x!=0u||map.z!=0u||map.w!=0u);uint light=colors.x;" \
    "if(pass==1u){vec2 n=vec2(vEnvironmentNormal.x*uDraw[6].y+vEnvironmentNormal.z*uDraw[6].z,-vEnvironmentNormal.x*uDraw[6].z+vEnvironmentNormal.z*uDraw[6].y);" \
    "uv=vec2(n.x,abs(n.y))+vec2(map.z+64u,map.w)+vec2(0.5);if(mode!=1u)light=(flags&1u)!=0u?colors.z:(facet&4u)!=0u?colors.y:colors.x;}" \
    "else{enabled=enabled&&(facet&1011u)!=0u;uv=vEnvironmentNormal.yz+vec2(map.z+64u,map.w+colors.w)+vec2(0.5);carWindow=uvec2(128u,192u);}" \
    "carOrigin=map.zw;tint=vec3(float(light)/255.0);tex=uvec4(map.xy,2u,enabled?1u:0u);}}" \
    "if((tex.w&1u)==0u)discard;bool semi=tex.z!=0u;uint stp=uint(uParams[0].x);\n" \
    "if(uParams[0].z>1.5){if(!semi||((tex.y>>5)&3u)!=uint(uParams[0].w))discard;stp=2u;}\n" \
    "else if(uParams[0].z>0.5)stp=semi?1u:0u;else if(!semi)stp=0u;\n" \
    "uvec2 pixel=uvec2(ivec2(floor(uv)));pixel=(pixel&(~carWindow))|(carOrigin&carWindow);\n" \
    "vec4 c=hdPsxColor(uv,tex.y,tex.x,region,stp,vidPsxDecode(tex.y,tex.x,ivec2(pixel)));\n" \
    "vec3 lit=hdCarSun(hdCarEnvironment(hdLighting(clamp(c.rgb*tint*(255.0/128.0),0.0,1.0),c.rgb)));if(c.a==0.0)discard;outColor=vec4(hdFog(lit),uParams[0].y); }\n"

static const char carFragment[] = HD_CAR_FRAGMENT_GLSL("vec3 hdCarEnvironment(vec3 base){return base;}\n");
static const char carCompositeFragment[] = HD_CAR_FRAGMENT_GLSL(HD_CAR_ENVIRONMENT_GLSL);

static VID_MATERIAL *hd_car_material(uint32 blend, bool depthWrite, float stpClass, uint32 cull, float alpha = 1, float mode = 0, float abr = 0)
{
    VID_MATERIAL *material = vid_mat_create();
    if (material == 0)
        return 0;
    VID_MATERIAL_DATA *data = vid_mat_data(material);
    data->shader = carShader;
    data->texture = worldVram;
    data->aux[0] = carMaterials;
    data->aux[5] = hd_lighting_depth();
    data->aux[6] = hd_lighting_data();
    data->depth_test = 1;
    data->depth_write = depthWrite ? 1 : 0;
    data->blend = blend;
    data->params[0] = stpClass;
    data->params[1] = alpha;
    data->params[2] = mode;
    data->params[3] = abr;
    data->filter = VID_FILTER_NEAREST;
    data->cull = cull;
    vid_mat_commit(material);
    return material;
}

static bool hd_car_resources(void)
{
    if (carShader == 0)
        carShader = vid_shader_create(carVertex, carFragment);
    if (carShader == 0)
        return false;
    if (carMaterials == 0)
    {
        carMaterials = vid_tex_create_fmt(256, 9, VID_RGBA16UI);
        carUploadRevision = ~carCaptureRevision;
    }
    if (carMaterials == 0)
        return false;
    for (sint32 orientation = 0; orientation < 2; ++orientation)
    {
        if (carMaterial[orientation] == 0)
            carMaterial[orientation] = hd_car_material(VID_BLEND_NONE, true, 0, orientation + 1, 1, 1);
        if (carShadowMaterials[orientation] == 0)
            carShadowMaterials[orientation] = hd_car_material(orientation == 0 ? VID_BLEND_NONE : VID_BLEND_SUBTRACT, false, orientation == 0 ? 1.0f : 2.0f, 0);
        if (carMaterial[orientation] == 0 || carShadowMaterials[orientation] == 0)
            return false;
        const uint32 blends[5] = {VID_BLEND_NONE, VID_BLEND_ALPHA, VID_BLEND_ADD, VID_BLEND_SUBTRACT, VID_BLEND_ADD};
        const float alpha[5] = {1, 0.5f, 1, 1, 0.25f};
        for (sint32 effect = 0; effect < 5; ++effect)
        {
            if (carEffectMaterials[effect][orientation] == 0)
                carEffectMaterials[effect][orientation] = hd_car_material(blends[effect], false, effect == 0 ? 1.0f : 2.0f, orientation + 1, alpha[effect]);
            if (carEffectMaterials[effect][orientation] == 0)
                return false;
        }
        for (sint32 blend = 0; blend < 4; ++blend)
        {
            if (carSemiMaterials[blend][orientation] == 0)
                carSemiMaterials[blend][orientation] = hd_car_material(blends[blend + 1], false, 2, orientation + 1, alpha[blend + 1], 2, (float)blend);
            if (carSemiMaterials[blend][orientation] == 0)
                return false;
        }
    }
    return true;
}

struct HdCarNormals
{
    const Transformer::zScene *scene;
    const Transformer::zObj *objects[57];
    VEC3F *normals[57];
    HdCarNormals *next;
};

static HdCarNormals *carNormals;
uint32 hd_car_authored_vertices;
uint32 hd_car_geometric_vertices;

void hd_car_normals_forget(const Transformer::zScene *scene)
{
    HdCarNormals **link = &carNormals;
    while (*link != 0)
    {
        HdCarNormals *entry = *link;
        if (scene == 0 || entry->scene == scene)
        {
            *link = entry->next;
            for (sint32 part = 0; part < 57; ++part)
                free(entry->normals[part]);
            free(entry);
            if (scene != 0)
                return;
        }
        else
            link = &entry->next;
    }
}

void hd_car_normals_capture(const Transformer::zScene *scene, sint32 part, const Transformer::zObj *object)
{
    if (scene == 0 || part < 0 || part >= 57 || object == 0 || object->Nvertex == 0 || object->numVertex == 0)
        return;
    hd_register_cleanup();
    HdCarNormals *entry = carNormals;
    while (entry != 0 && entry->scene != scene)
        entry = entry->next;
    if (entry == 0)
    {
        entry = (HdCarNormals *)calloc(1, sizeof(*entry));
        if (entry == 0)
            return;
        entry->scene = scene;
        entry->next = carNormals;
        carNormals = entry;
    }
    free(entry->normals[part]);
    entry->objects[part] = object;
    entry->normals[part] = (VEC3F *)calloc(object->numVertex, sizeof(VEC3F));
    if (entry->normals[part] == 0)
        return;
    for (sint32 vertex = 0; vertex < object->numVertex; ++vertex)
    {
        const COORD16 *normal = object->Nvertex + vertex;
        float length = sqrtf((float)normal->x * normal->x + (float)normal->y * normal->y + (float)normal->z * normal->z);
        if (length > 0)
        {
            entry->normals[part][vertex].x = normal->x / length;
            entry->normals[part][vertex].y = normal->y / length;
            entry->normals[part][vertex].z = normal->z / length;
        }
    }
}

const VEC3F *hd_car_normals_read(const Transformer::zObj *object)
{
    for (const HdCarNormals *entry = carNormals; entry != 0; entry = entry->next)
        for (sint32 part = 0; part < 57; ++part)
            if (entry->objects[part] == object)
                return entry->normals[part];
    return 0;
}

static void hd_car_vertex_normals(const Transformer::zObj *object, VEC3F *normals)
{
    for (sint32 face = 0; face < object->numFacet; ++face)
    {
        const Transformer::zFacet *facet = object->facet + face;
        const uint8 ids[3] = {facet->vertexId0, facet->vertexId1, facet->vertexId2};
        if (facet->textureIndex >= 51 || ids[0] >= object->numVertex || ids[1] >= object->numVertex || ids[2] >= object->numVertex)
            continue;
        const COORD16 *a = object->vertex + ids[0], *b = object->vertex + ids[1], *c = object->vertex + ids[2];
        VEC3F ab = {(float)b->x - a->x, (float)b->y - a->y, (float)b->z - a->z};
        VEC3F ac = {(float)c->x - a->x, (float)c->y - a->y, (float)c->z - a->z};
        VEC3F normal = {ab.y * ac.z - ab.z * ac.y, ab.z * ac.x - ab.x * ac.z, ab.x * ac.y - ab.y * ac.x};
        for (sint32 corner = 0; corner < 3; ++corner)
            for (sint32 axis = 0; axis < 3; ++axis)
                normals[ids[corner]].v[axis] += normal.v[axis];
    }
    const VEC3F *authored = hd_car_normals_read(object);
    for (sint32 vertex = 0; vertex < object->numVertex; ++vertex)
    {
        VEC3F *normal = normals + vertex;
        if (authored != 0 && authored[vertex].x * authored[vertex].x + authored[vertex].y * authored[vertex].y + authored[vertex].z * authored[vertex].z > 0.5f)
        {
            *normal = authored[vertex];
            ++hd_car_authored_vertices;
            continue;
        }
        ++hd_car_geometric_vertices;
        float length = sqrtf(normal->x * normal->x + normal->y * normal->y + normal->z * normal->z);
        if (length > 0)
            for (sint32 axis = 0; axis < 3; ++axis)
                normal->v[axis] /= length;
    }
}

static bool hd_car_layered_part(const HdCarModel *car, sint32 part)
{
    return car->type < 28 && (part == 12 || part == 13 || part == 31 || part == 45 || part == 46 || ((uint8)R3DCar::ObjectInfo[part][1] & 9) == 8);
}

static bool hd_car_build(HdCarModel *car)
{
    if (!car->dirty)
        return true;
    for (sint32 index = 0; index < 57; ++index)
    {
        HdCarPart *part = car->parts + index;
        vid_mesh_free(part->geometry.mesh);
        vid_index_free(part->geometry.indices);
        free(part->facets);
        vid_index_free(part->shadowIndices);
        part->shadowIndices = 0;
        part->shadowCount = 0;
        part->shadowGeneration = 0;
        for (sint32 detail = 0; detail < 4; ++detail)
        {
            free(part->renderSurfaces[detail]);
            part->renderSurfaces[detail] = 0;
            part->surfaceGeneration[detail] = 0;
        }
        part->facets = 0;
        memset(&part->geometry, 0, sizeof(part->geometry));
        const Transformer::zObj *object = part->object;
        if (object == 0 || object->numFacet == 0 || object->vertex == 0 || object->facet == 0)
            continue;
        VID_GEOMETRY *mesh = vid_mesh_create_16(object->numFacet * 3);
        if (mesh == 0)
            return false;
        part->geometry.mesh = mesh;
        part->facets = (uint32 *)calloc(object->numFacet, sizeof(*part->facets));
        if (part->facets == 0)
            return false;
        VEC3F *normals = (VEC3F *)calloc(object->numVertex, sizeof(*normals));
        if (normals == 0)
            return false;
        hd_car_vertex_normals(object, normals);
        VID_VERTEX16 *vertices = vid_mesh_data_16(mesh);
        uint32 used = 0;
        for (sint32 face = object->numFacet - 1; face >= 0; --face)
        {
            const Transformer::zFacet *facet = object->facet + face;
            uint8 ids[3] = {facet->vertexId0, facet->vertexId1, facet->vertexId2};
            const Transformer::zUV uv[3] = {facet->uv0, facet->uv1, facet->uv2};
            if (facet->textureIndex >= 51 || ids[0] >= object->numVertex || ids[1] >= object->numVertex || ids[2] >= object->numVertex)
            {
                ++hd_car_invalid_count;
                continue;
            }
            sint32 envmap = car->type >= 28 ? 0x22 : (uint8)R3DCar::ObjectInfo[index][1];
            if (car->type < 28 && !car->envMapEnabled)
                envmap &= 0xFE;
            part->facets[used / 3] = face;
            for (sint32 corner = 0; corner < 3; ++corner)
            {
                VID_VERTEX16 *vertex = vertices + used++;
                memset(vertex, 0, sizeof(*vertex));
                vertex->pos[0] = object->vertex[ids[corner]].x;
                vertex->pos[1] = object->vertex[ids[corner]].y;
                vertex->pos[2] = object->vertex[ids[corner]].z;
                float radius2 = 0;
                for (sint32 axis = 0; axis < 3; ++axis)
                    radius2 += (float)vertex->pos[axis] * vertex->pos[axis];
                if (radius2 > part->geometry.radius * part->geometry.radius)
                    part->geometry.radius = sqrtf(radius2);
                if (object->Nvertex != 0)
                {
                    vertex->n[0] = object->Nvertex[ids[corner]].x;
                    vertex->n[1] = object->Nvertex[ids[corner]].y;
                    vertex->n[2] = object->Nvertex[ids[corner]].z;
                }
                memcpy(vertex->extra, normals[ids[corner]].v, 3 * sizeof(float));
                sint32 authoredBias = -(sint8)R3DCar::ObjectInfo[index][0];
                vertex->extra[3] = hd_car_layered_part(car, index) && authoredBias > 0 ? (float)authoredBias : 0;
                vertex->uv[0] = uv[corner].u;
                vertex->uv[1] = uv[corner].v;
                vertex->tex[0] = facet->textureIndex;
                vertex->tex[1] = (uint16)facet->flag;
                vertex->tex[2] = (uint16)envmap;
            }
        }
        free(normals);
        vid_mesh_commit(mesh);
        vid_mesh_static(mesh);
        part->geometry.mesh = mesh;
        part->geometry.count = used;
    }
    car->dirty = false;
    return true;
}

static void hd_car_part_model(HdCarPart *part, sint32 detail, float phase, MATRIXF *model)
{
    if (part->renderGeneration[detail] != renderGeneration)
    {
        const HdCarPartPose *pose = part->poses + detail;
        MATRIXF *cached = part->renderPose + detail;
        hd_mix_pose(&pose->previous, &pose->current, poseRenderPhase, cached, 0);
        for (sint32 row = 0; row < 3; ++row)
            for (sint32 column = 0; column < 3; ++column)
                cached->m[row][column] *= column == 0 && pose->mirrored ? -0.25f : 0.25f;
        part->renderGeneration[detail] = renderGeneration;
    }
    *model = part->renderPose[detail];
}

static bool hd_cars_prepare(void)
{
    for (sint32 slot = 0; slot < 9; ++slot)
        if (carModels[slot].active && !hd_car_build(carModels + slot))
            return false;
    return true;
}

static void hd_car_upload(void)
{
    if (carUploadRevision == carCaptureRevision)
        return;
    hd_car_filter_regions = 0;
    uint16 *metadata = (uint16 *)vid_tex_data(carMaterials);
    memset(metadata, 0, 256 * 9 * 4 * sizeof(*metadata));
    for (sint32 slot = 0; slot < 9; ++slot)
    {
        const HdCarModel *car = carModels + slot;
        uint16 *row = metadata + slot * 256 * 4;
        for (sint32 index = 0; index < 51; ++index)
        {
            row[index * 4] = car->pixmaps[index].clut;
            row[index * 4 + 1] = car->pixmaps[index].tpage;
            row[index * 4 + 2] = car->pixmaps[index].flag;
            HdTextureRegion region = {0};
            if ((hd_texture_filtering_enabled & HD_FILTER_CAR) != 0 && hd_texture_region(car->pixmaps + index, &region))
                ++hd_car_filter_regions;
            memcpy(row + (198 + index) * 4, &region, sizeof(region));
            row[(141 + index) * 4] = (uint16)car->overlays[index];
        }
        for (sint32 index = 0; index < 90; ++index)
        {
            row[(51 + index) * 4] = (uint16)car->overlayMaps[index].offset;
            row[(51 + index) * 4 + 1] = car->overlayMaps[index].u;
            row[(51 + index) * 4 + 2] = car->overlayMaps[index].v;
        }
        for (sint32 index = 0; index < 6; ++index)
            row[192 * 4 + index] = car->offsets[index];
        row[193 * 4 + 2] = (uint16)car->pixmapCount;
        for (sint32 index = 0; index < 2; ++index)
        {
            const Draw::tPixMap *map = car->environment.environment + index;
            row[(194 + index) * 4] = map->clut;
            row[(194 + index) * 4 + 1] = map->tpage;
            row[(194 + index) * 4 + 2] = map->u0;
            row[(194 + index) * 4 + 3] = map->v0;
        }
        for (sint32 index = 0; index < 3; ++index)
            row[196 * 4 + index] = (uint16)(car->environment.colors[index] & 255);
        row[196 * 4 + 3] = (uint8)car->environment.scroll;
        row[197 * 4] = carShadowMap.clut;
        row[197 * 4 + 1] = carShadowMap.tpage;
        for (sint32 index = 0; index < 3; ++index)
        {
            HdTextureRegion region = {0};
            const Draw::tPixMap *map = index < 2 ? car->environment.environment + index : &carShadowMap;
            if ((hd_texture_filtering_enabled & HD_FILTER_CAR) != 0)
                hd_texture_region(map, &region);
            memcpy(row + (249 + index) * 4, &region, sizeof(region));
        }
    }
    vid_tex_commit(carMaterials);
    carUploadRevision = carCaptureRevision;
}

static bool hd_shadow_build(float phase)
{
    phase = poseRenderPhase;
    if (carShadows.mesh == 0)
        carShadows.mesh = vid_mesh_create_2d(9 * 24);
    if (carShadows.mesh == 0)
        return false;
    VID_VERTEX2D *vertices = vid_mesh_data_2d(carShadows.mesh);
    static const uint8 order[6] = {0, 1, 3, 3, 1, 4};
    static const uint8 corners[4] = {0, 2, 6, 8};
    static const uint8 middle[5][3] = {{1, 0, 2}, {3, 0, 6}, {5, 2, 8}, {7, 6, 8}, {4, 0, 8}};
    const uint8 sourceU[4] = {carShadowMap.u0, carShadowMap.u1, carShadowMap.u2, carShadowMap.u3};
    const uint8 sourceV[4] = {carShadowMap.v0, carShadowMap.v1, carShadowMap.v2, carShadowMap.v3};
    for (sint32 slot = 0; slot < 9; ++slot)
    {
        const HdCarModel *car = carModels + slot;
        VEC3F points[9];
        uint16 u[9], v[9];
        for (sint32 corner = 0; corner < 4; ++corner)
        {
            for (sint32 axis = 0; axis < 3; ++axis)
                points[corners[corner]].v[axis] = car->shadowPrevious[corner].v[axis] + (car->shadowCurrent[corner].v[axis] - car->shadowPrevious[corner].v[axis]) * phase;
            u[corners[corner]] = sourceU[corner];
            v[corners[corner]] = sourceV[corner];
        }
        for (sint32 i = 0; i < 5; ++i)
        {
            sint32 at = middle[i][0], left = middle[i][1], right = middle[i][2];
            for (sint32 axis = 0; axis < 3; ++axis)
                points[at].v[axis] = (points[left].v[axis] + points[right].v[axis]) * 0.5f;
            u[at] = (u[left] + u[right] + 1) / 2;
            v[at] = (v[left] + v[right] + 1) / 2;
        }
        for (sint32 cell = 0; cell < 4; ++cell)
            for (sint32 i = 0; i < 6; ++i)
            {
                sint32 corner = (cell & 1) + (cell >> 1) * 3 + order[i];
                VID_VERTEX2D *vertex = vertices + slot * 24 + cell * 6 + i;
                memset(vertex, 0, sizeof(*vertex));
                memcpy(vertex->pos, points + corner, sizeof(points[corner]));
                vertex->pos[3] = 1;
                vertex->uv[0] = (float)u[corner];
                vertex->uv[1] = (float)v[corner];
                vertex->color[0] = car->shadowColor.r;
                vertex->color[1] = car->shadowColor.g;
                vertex->color[2] = car->shadowColor.b;
                vertex->color[3] = 255;
            }
    }
    carShadows.count = 24;
    vid_mesh_commit(carShadows.mesh);
    return true;
}

static sint32 hd_car_detail(const HdCarModel *car, const MATRIXF *root, const MATRIXF *camera, const HdViewPose *view)
{
    if (!car->active)
        return -1;
    bool interior = false;
    if (view->anchor == car->object && (car->flags & 4) != 0 && view->inCar)
    {
        if (view->mode == 0)
            return -1;
        if (view->mode == 1)
        {
            if ((car->inside & 1) == 0)
                return -1;
            interior = true;
        }
    }
    float depth = 0;
    float horizontal = 0;
    for (sint32 axis = 0; axis < 3; ++axis)
    {
        depth += camera->m[axis][2] * (root->t[axis] - camera->t[axis]);
        horizontal += camera->m[axis][0] * (root->t[axis] - camera->t[axis]);
    }
    if (depth < -192 || depth > 12800)
        return -1;
    float halfWidth = car->dimensions.x / 1024.0f;
    float halfLength = car->dimensions.z / 1024.0f;
    float left = fabsf(horizontal - halfWidth), right = fabsf(horizontal + halfWidth);
    if (depth + halfLength < left && depth + halfLength < right && depth - halfLength < left && depth - halfLength < right)
        return -1;
    if (interior)
        return 3;
    return car->type >= 28 || car->mediumOnly ? 1 : 0;
}

static void hd_car_order(float phase, const MATRIXF *camera, const HdViewPose *view, HdCarOrder orders[9])
{
    sint32 lower[9], upper[9], center[9];
    for (sint32 slot = 0; slot < 9; ++slot)
    {
        HdCarOrder *order = orders + slot;
        memset(order, 0, sizeof(*order));
        order->root = slot;
        order->group = slot * 2;
        order->detail = -1;
        lower[slot] = center[slot] = 0;
        upper[slot] = carModels[slot].type == 28 ? 416 : carModels[slot].dimensions.z >> 9;
        MATRIXF root;
        if (!hd_render_car_pose(slot, phase, &root))
            continue;
        order->detail = hd_car_detail(carModels + slot, &root, camera, view);
        if (order->detail < 0)
            continue;
        if (hd_car_variant_override >= 0 && hd_car_variant_override < 4)
            order->detail = hd_car_variant_override;
        for (sint32 axis = 0; axis < 3; ++axis)
            order->depth += camera->m[axis][2] * (root.t[axis] - camera->t[axis]);
        float z = order->depth;
        if (z < 0)
            z = 0;
        if (z > 65535)
            z = 65535;
        center[slot] = (sint32)floorf(z);
        order->bucket = center[slot] >> 3;
        if (order->bucket > view->orderTableSize - 3)
            order->detail = -1;
    }
    for (sint32 slot = 0; slot < 8; ++slot)
        if (orders[slot].detail >= 0 && orders[slot].root == slot)
            for (sint32 other = slot + 1; other < 9; ++other)
            {
                if (orders[other].detail < 0 || orders[other].root != other)
                    continue;
                float difference = (orders[slot].depth - orders[other].depth) * 1024;
                if (fabsf(difference) >= (float)carModels[slot].dimensions.z + carModels[other].dimensions.z)
                    continue;
                orders[other].root = slot;
                orders[slot].group = other * 2;
                if (difference < 0)
                    lower[slot] -= carModels[other].dimensions.z >> 9;
                else
                    upper[slot] += carModels[other].dimensions.z >> 9;
            }
    for (sint32 slot = 0; slot < 9; ++slot)
    {
        HdCarOrder *order = orders + slot;
        if (order->detail < 0)
            continue;
        sint32 root = order->root;
        order->bias = (order->bucket - orders[root].bucket) * 8 + upper[root] + 48 - (carModels[root].dimensions.z >> 10) - ((center[slot] >> 2) * 4);
        order->bucket = orders[root].bucket;
        order->group = orders[root].group;
        order->size = upper[root] - lower[root] + 96;
    }
}

static bool hd_car_surface(const HdCarModel *car, sint32 part, const Transformer::zFacet *facet, sint32 detail, HdCarSurface *surface)
{
    sint32 envmap = car->type >= 28 ? 0x22 : (uint8)R3DCar::ObjectInfo[part][1];
    if (!car->envMapEnabled && car->type < 28)
        envmap &= 0xFE;
    sint32 mode = envmap & 9;
    uint16 flags = (uint16)facet->flag;
    sint32 texture = facet->textureIndex;
    if (texture >= 51)
        return false;
    uint32 overlay = 0;
    if ((mode & 8) != 0)
        overlay = (flags & 0x8000) != 0 ? (uint16)car->overlays[texture] >> 8 : (uint16)car->overlays[texture] & 255;
    surface->bias = mode == 8 && (overlay & 0x40) != 0 ? -2 : 0;
    if ((flags & 0x3F0) != 0)
    {
        overlay &= (flags & 0xFFF) >> 4;
        if (overlay != 0)
            while ((overlay & 3) == 0)
                overlay >>= 2;
    }
    sint32 which = overlay & 3;
    if (which != 0)
    {
        sint32 map = texture * 3 + which - 1;
        if (map >= 90)
            return false;
        texture += car->overlayMaps[map].offset;
    }
    if (texture < 0 || texture >= car->pixmapCount)
        return false;
    surface->castsShadow = (car->pixmaps[texture].flag & 3) == 0;
    surface->blend = (car->pixmaps[texture].tpage >> 5) & 3;
    surface->semi = (flags & 1) != 0 && ((mode == 0 && (envmap & 2) != 0 && detail != 3) || (mode & 8) != 0);
    return true;
}

static const HdCarSurface *hd_car_surfaces(HdCarModel *car, sint32 partIndex, sint32 detail)
{
    HdCarPart *part = car->parts + partIndex;
    if (part->renderSurfaces[detail] == 0)
        part->renderSurfaces[detail] = (HdCarSurface *)calloc(part->geometry.count / 3, sizeof(HdCarSurface));
    if (part->renderSurfaces[detail] == 0)
        return 0;
    if (part->surfaceGeneration[detail] != renderGeneration)
    {
        for (uint32 face = 0; face < part->geometry.count / 3; ++face)
        {
            HdCarSurface *surface = part->renderSurfaces[detail] + face;
            surface->valid = hd_car_surface(car, partIndex, part->object->facet + part->facets[face], detail, surface);
        }
        part->surfaceGeneration[detail] = renderGeneration;
    }
    return part->renderSurfaces[detail];
}

static sint32 hd_car_facet_bucket(const Transformer::zObj *object, const Transformer::zFacet *facet, const MATRIXF *model, const MATRIXF *camera, sint32 bias)
{
    const uint8 ids[3] = {facet->vertexId0, facet->vertexId1, facet->vertexId2};
    uint32 sum = 0;
    for (sint32 corner = 0; corner < 3; ++corner)
    {
        const COORD16 *point = object->vertex + ids[corner];
        const sint16 position[3] = {point->x, point->y, point->z};
        float depth = hd_face_depth(position, model, camera) * 4;
        if (!(depth >= 0))
            depth = 0;
        if (depth > 65535)
            depth = 65535;
        sum += (uint32)floorf(depth);
    }
    return (sint32)((sum * 341) >> 12) + bias;
}

static sint32 hd_shadow_bucket(const HdCarModel *car, float phase, const MATRIXF *camera)
{
    phase = poseRenderPhase;
    float sum = 0;
    for (sint32 corner = 0; corner < 4; ++corner)
    {
        float depth = 0;
        for (sint32 axis = 0; axis < 3; ++axis)
        {
            float position = car->shadowPrevious[corner].v[axis] + (car->shadowCurrent[corner].v[axis] - car->shadowPrevious[corner].v[axis]) * phase;
            depth += camera->m[axis][2] * (position - camera->t[axis]);
        }
        if (!(depth >= 0))
            depth = 0;
        if (depth > 65535)
            depth = 65535;
        sum += floorf(depth);
    }
    return ((sint32)floorf(sum / 16) >> 1) + 40;
}

static void hd_shadows_view(float phase, const MATRIXF *camera, const HdViewPose *view, VID_RECT viewport, float logicalHeight)
{
    if (!carShadowMapValid || hd_car_shadows_enabled == 0)
        return;
    MATRIXF identity;
    IdentityMatrixf(&identity);
    for (sint32 slot = 0; slot < 9; ++slot)
    {
        const HdCarModel *car = carModels + slot;
        if (!car->active || !car->shadowActive)
            continue;
        MATRIXF root;
        if (!hd_render_car_pose(slot, phase, &root) || hd_car_detail(car, &root, camera, view) < 0)
            continue;
        sint32 bucket = hd_shadow_bucket(car, phase, camera);
        if (bucket > view->orderTableSize - 3)
            continue;
        for (sint32 pass = 0; pass < 2; ++pass)
        {
            HdOrderKey order = {bucket, HD_ORDER_CARS, (uint32)(slot * 2 + 1), 0, 0, 0, (uint32)pass};
            hd_world_submit(&carShadows, &identity, camera, view, viewport, logicalHeight, -1, -1, slot, 3, 1, 0, slot * 24, 0, 1, &order, carShadowMaterials[pass]);
            ++hd_car_shadow_draws;
        }
    }
}

static void hd_cars_view(float phase, const MATRIXF *camera, const HdViewPose *view, VID_RECT viewport, float logicalHeight, const HdCarOrder orders[9])
{
    hd_car_upload();
    if (hd_gpu_shadows_enabled == 0 || !hd_lighting_ready())
        hd_shadows_view(phase, camera, view, viewport, logicalHeight);
    for (sint32 slot = 0; slot < 9; ++slot)
    {
        HdCarModel *car = carModels + slot;
        const HdCarOrder *order = orders + slot;
        sint32 detail = order->detail;
        if (detail < 0 || (cubeRendering && car->source == cubeOwner))
            continue;
        MATRIXF carPose;
        if (!hd_render_car_pose(slot, phase, &carPose))
            continue;
        ++hd_car_variant_draws[detail];
        float relative[3] = {carPose.t[0] - camera->t[0], carPose.t[1] - camera->t[1], carPose.t[2] - camera->t[2]};
        float localX = 0, localZ = 0;
        for (sint32 axis = 0; axis < 3; ++axis)
        {
            localX += relative[axis] * carPose.m[axis][0];
            localZ += relative[axis] * carPose.m[axis][2];
        }
        float length = sqrtf(localX * localX + localZ * localZ);
        float yawCos = length > 0 ? localZ / length : 1;
        float yawSin = length > 0 ? -localX / length : 0;
        uint32 rgb = hd_headlights_enabled == 0 ? hd_night_car_color(car->light, &carPose) : car->light;
        sint32 average = ((rgb & 255) + ((rgb >> 8) & 255) + ((rgb >> 16) & 255)) / 3;
        if (average < 24)
            average = 24;
        for (sint32 index = 0; index < 57; ++index)
        {
            const HdCarPart *part = car->parts + index;
            const HdCarPartPose *pose = part->poses + detail;
            if (pose->visible == 0 || pose->visible >= 17 || part->geometry.count == 0)
                continue;
            MATRIXF model;
            hd_car_part_model(car->parts + index, detail, phase, &model);
            if (cubeRendering && !hd_cube_mesh_visible(&part->geometry, &model, camera))
                continue;
            sint32 orientation = (view->mirror != 0) != pose->mirrored ? 1 : 0;
            bool combined = !cubeRendering && cubeReady && hd_dynamic_reflections_enabled != 0 && hd_car_reflections_enabled != 0;
            VID_MATERIAL *baseMaterial = combined ? carCubeMaterials[orientation] : carMaterial[orientation];
            const HdCarSurface *surfaces = hd_car_surfaces(car, index, detail);
            if (surfaces == 0)
            {
                worldSubmissionValid = false;
                return;
            }
            bool layered = !cubeRendering && hd_car_layered_part(car, index);
            if (!layered)
            {
                uint32 first = 0;
                while (first < part->geometry.count / 3)
                {
                    if (!surfaces[first].valid)
                    {
                        ++first;
                        continue;
                    }
                    uint32 end = first + 1;
                    while (end < part->geometry.count / 3 && surfaces[end].valid)
                        ++end;
                    HdWorldChunk mesh = part->geometry;
                    mesh.count = (end - first) * 3;
                    vid_pass(baseMaterial);
                    hd_world_submit(&mesh, &model, camera, view, viewport, logicalHeight, -1, average * 0x10101, slot, 0, 1, 0, first * 3, detail == 3 ? 1 : 0, 1, 0, baseMaterial);
                    first = end;
                }
                if (cubeRendering && hd_car_semi_enabled != 0)
                    for (uint32 face = 0; face < part->geometry.count / 3; ++face)
                    {
                        const HdCarSurface *surface = surfaces + face;
                        if (!surface->valid || !surface->semi)
                            continue;
                        const Transformer::zFacet *source = part->object->facet + part->facets[face];
                        sint32 bucket = hd_car_facet_bucket(part->object, source, &model, camera, 0);
                        HdOrderKey key = {bucket, HD_ORDER_CARS, (uint32)slot, 0, (uint32)(slot * 57 + index), part->object->numFacet - 1 - part->facets[face], 1};
                        HdWorldChunk mesh = part->geometry;
                        mesh.count = 3;
                        hd_world_submit(&mesh, &model, camera, view, viewport, logicalHeight, -1, average * 0x10101, slot, 0, 1, 0, face * 3, 0, 1, &key, carSemiMaterials[surface->blend][orientation]);
                    }
                if (cubeRendering)
                    continue;
            }
            for (uint32 face = 0; face < part->geometry.count / 3; ++face)
            {
                const HdCarSurface &surface = surfaces[face];
                const Transformer::zFacet *source = part->object->facet + part->facets[face];
                if (!surface.valid)
                    continue;
                sint32 bucket = hd_car_facet_bucket(part->object, source, &model, camera, order->bias + pose->orderOffset + surface.bias);
                if (bucket < 0 || bucket >= order->size)
                    continue;
                HdWorldChunk mesh = part->geometry;
                mesh.count = 3;
                HdOrderKey key = {order->bucket, HD_ORDER_CARS, (uint32)order->group, (uint32)bucket, (uint32)(slot * 57 + index), part->object->numFacet - 1 - part->facets[face], 0};
                if (layered)
                    hd_world_submit(&mesh, &model, camera, view, viewport, logicalHeight, -1, average * 0x10101, slot, 0, 1, 0, face * 3, detail == 3 ? 1 : 0, 1, &key, baseMaterial);
                key.pass = 1;
                if (surface.semi && hd_car_semi_enabled != 0)
                {
                    hd_world_submit(&mesh, &model, camera, view, viewport, logicalHeight, -1, average * 0x10101, slot, 0, 1, 0, face * 3, detail == 3 ? 1 : 0, 1, &key, combined ? carCubeSemiMaterials[surface.blend][orientation] : carSemiMaterials[surface.blend][orientation]);
                    ++hd_car_semi_draws;
                }
                sint32 envmap = car->type >= 28 ? 0x22 : (uint8)R3DCar::ObjectInfo[index][1];
                if (cubeRendering || hd_car_reflections_enabled == 0 || !car->envMapEnabled || (envmap & 1) == 0)
                    continue;
                if (cubeReady && hd_dynamic_reflections_enabled != 0)
                {
                    ++hd_cube_car_draws;
                    continue;
                }
                for (sint32 effect = 1; effect <= 2; ++effect)
                {
                    const Draw::tPixMap *map = car->environment.environment + effect - 1;
                    if ((map->u0 == 0 && map->v0 == 0 && map->clut == 0) || (effect == 2 && ((uint16)source->flag & 0x3F3) == 0))
                        continue;
                    sint32 blend = (map->tpage >> 5) & 3;
                    for (sint32 pass = 0; pass < 2; ++pass)
                    {
                        key.pass = effect * 2 + pass;
                        hd_world_submit(&mesh, &model, camera, view, viewport, logicalHeight, -1, average * 0x10101, slot, effect, yawCos, yawSin, face * 3, detail == 3 ? 1 : 0, 1, &key, carEffectMaterials[pass == 0 ? 0 : blend + 1][orientation]);
                        ++hd_car_reflection_draws;
                    }
                }
            }
            if (detail == 3 && index >= 35 && index <= 40)
                ++hd_car_interior_parts;
            ++hd_car_part_draws;
            hd_car_triangle_count += part->geometry.count / 3;
        }
    }
}

static bool hd_chunk_visible(const HdViewPose *view, sint32 chunk, bool objects, uint16 rejectMask = 0)
{
    if (cubeRendering)
    {
        if (chunk < 0)
            return true;
        if (chunk >= worldChunkCount)
            return false;
        float dx = worldChunks[chunk].center.x - cubeOrigin.x;
        float dz = worldChunks[chunk].center.z - cubeOrigin.z;
        return dx * dx + dz * dz <= view->world.chunkFarSquared * 4;
    }
    sint32 source = view->world.chunk;
    if (source < 0 || source >= worldChunkCount || chunk < 0 || chunk >= worldChunkCount || Track::gInViewCount == 0 || Track::gInViewList == 0)
        return false;
    const VEC3F *center = &worldChunks[chunk].center;
    float dx = center->x - view->pose.position.x;
    float dz = center->z - view->pose.position.z;
    if (dx * dx + dz * dz > view->world.chunkFarSquared * 4)
        return false;
    for (sint32 distance = 0; distance <= 4; ++distance)
        for (sint32 direction = -1; direction <= 1; direction += 2)
        {
            if (distance == 0 && direction == 1)
                continue;
            sint32 neighbor = (source + direction * (distance % worldChunkCount) + worldChunkCount) % worldChunkCount;
            sint32 count = Track::gInViewCount[neighbor];
            if (count > 36)
                continue;
            for (sint32 i = 0; i < count; ++i)
            {
                uint16 entry = ((const sint16 *)Track::gInViewList)[neighbor * 32 + i];
                if ((entry & 1023) == chunk && (entry & (rejectMask != 0 ? rejectMask : objects ? 0x4800 : 0x2800)) == 0)
                    return true;
            }
        }
    return false;
}

static bool hd_world_batch_view(sint32 player, const MATRIXF *camera, const HdViewPose *view, VID_RECT viewport, float logicalHeight)
{
    sint32 bank = cubeRendering ? (sint32)cubeFace + 2 : player;
    uint32 *indices = vid_index_data(worldBatchIndices[bank]);
    uint32 used = 0;
    bool changed = false;
    hd_world_batch_draws[bank] = hd_world_batch_chunks[bank] = 0;
    for (sint32 chunk = 0; chunk < worldChunkCount; ++chunk)
    {
        const HdWorldChunk *mesh = worldChunks + chunk;
        if (!hd_chunk_visible(view, chunk, false) || mesh->count == 0)
            continue;
        MATRIXF model;
        IdentityMatrixf(&model);
        memcpy(model.t, &mesh->center, sizeof(model.t));
        if (cubeRendering && !hd_cube_mesh_visible(mesh, &model, camera))
            continue;
        const uint32 *source = worldBatchTopology + worldBatchStarts[chunk] / 4 * 6;
        ++hd_world_batch_chunks[bank];
        for (uint32 index = 0; index < mesh->count; ++index)
        {
            uint32 value = source[index];
            if (indices[used] != value)
                changed = true;
            indices[used++] = value;
        }
    }
    if (used == 0)
        return true;
    if (changed || used != worldBatchCounts[bank])
    {
        vid_index_commit(worldBatchIndices[bank], used);
        worldBatchCounts[bank] = used;
    }
    HdWorldChunk mesh = worldBatch;
    mesh.indices = worldBatchIndices[bank];
    mesh.count = used;
    MATRIXF model;
    IdentityMatrixf(&model);
    memcpy(model.t, &worldBatchOrigin, sizeof(model.t));
    vid_pass(worldBatchMaterial);
    hd_world_submit(&mesh, &model, camera, view, viewport, logicalHeight, -1, -1, 0, -1, 1, 0, 0, 0, 1, 0, worldBatchMaterial);
    hd_world_batch_draws[bank] = 1;
    return worldSubmissionValid;
}

static bool hd_object_visible(const HdObjectEntry *entry, const HdViewPose *view)
{
    if (cubeRendering)
    {
        if (!entry->current.active || entry->current.definition < 0 || entry->current.definition >= worldDefinitionCount)
            return false;
        float dx = entry->current.pose.position.x - cubeOrigin.x;
        float dz = entry->current.pose.position.z - cubeOrigin.z;
        return dx * dx + dz * dz <= view->world.chunkFarSquared * 4;
    }
    if (!entry->current.active || entry->current.definition < 0 || entry->current.definition >= worldDefinitionCount)
        return false;
    if (entry->kind < 2)
        return hd_chunk_visible(view, entry->chunk, true);
    if (entry->kind == 4)
    {
        float dx = entry->current.pose.position.x - view->pose.position.x;
        float dz = entry->current.pose.position.z - view->pose.position.z;
        return dx * dx + dz * dz < view->world.chunkFarSquared * 4;
    }
    uint32 chunk = view->world.chunk;
    if (chunk >= (uint32)worldChunkCount)
        return false;
    if (entry->kind == 3)
        return entry->ordinal < 32 && Track::gInViewList != 0 && (Track::gInViewList[chunk][entry->ordinal] & 0x1000) != 0;
    sint32 track = view->world.track;
    if ((track == 4 && (chunk - 1 < 40 || chunk - 61 < 30 || chunk - 108 < 9)) || (track == 0 && (chunk - 1 < 53 || chunk - 68 < 28 || chunk - 108 < 20)))
        return false;
    float dx = entry->current.pose.position.x - view->pose.position.x;
    float dz = entry->current.pose.position.z - view->pose.position.z;
    return dx * dx + dz * dz < view->world.chunkFarSquared * 4;
}

static void hd_object_matrix_uncached(const HdObjectEntry *entry, float phase, const MATRIXF *camera, MATRIXF *model)
{
    if (objectReset)
        phase = 1;
    if (!(phase >= 0))
        phase = 0;
    if (phase > 1)
        phase = 1;
    HdPose pose;
    hd_mix_pose(&entry->previous.pose, &entry->current.pose, phase, model, &pose);
    if (entry->current.billboard)
    {
        IdentityMatrixf(model);
        RotMatrixYf(atan2f(pose.position.x - camera->t[0], pose.position.z - camera->t[2]), model);
        model->t[0] = pose.position.x;
        model->t[1] = pose.position.y;
        model->t[2] = pose.position.z;
    }
    const float previousScale[3] = {entry->previous.scale.x, entry->previous.scale.y, entry->previous.scale.z};
    const float currentScale[3] = {entry->current.scale.x, entry->current.scale.y, entry->current.scale.z};
    for (sint32 row = 0; row < 3; ++row)
    {
        float scale = previousScale[row] + (currentScale[row] - previousScale[row]) * phase;
        if (entry->current.orderOffset == -1)
            scale *= 4;
        for (sint32 column = 0; column < 3; ++column)
            model->m[row][column] *= scale;
    }
}

static void hd_object_matrix(const HdObjectEntry *entry, float phase, const MATRIXF *camera, MATRIXF *model)
{
    sint32 player = lightingView == 1 ? 1 : 0;
    HdObjectEntry *cache = const_cast<HdObjectEntry *>(entry);
    if (cache->renderModelGeneration[player] != renderGeneration)
    {
        hd_object_matrix_uncached(entry, phase, camera, cache->renderModel + player);
        cache->renderModelGeneration[player] = renderGeneration;
    }
    *model = cache->renderModel[player];
}

static bool hd_static_world_candidate(const HdObjectEntry *entry)
{
    return entry->kind <= 2 && entry->current.active && (entry->current.type == 1 || entry->current.type == 2 || entry->current.type == 5) && !entry->current.billboard && entry->pieceCount == 0 && entry->current.orderOffset != -1 && entry->current.definition >= 0 && entry->current.definition < worldDefinitionCount;
}

static bool hd_static_world_match(uint32 index, const MATRIXF *model)
{
    if (index >= staticWorldEntryCount || !hd_static_world_candidate(worldObjects + index))
        return false;
    const HdStaticWorldEntry *saved = staticWorldEntries + index;
    const HdObjectEntry *entry = worldObjects + index;
    if (saved->light >= 0 && saved->light < Chunk::numLight && memcmp(&saved->color, Chunk::lightTable + saved->light, sizeof(saved->color)) != 0)
        return false;
    return saved->count != 0 && saved->source == entry->source && saved->definition == entry->current.definition && saved->light == entry->current.light && memcmp(&saved->model, model, sizeof(*model)) == 0;
}

static bool hd_static_world_build(float phase)
{
    if (staticWorldReady)
        return true;
    if (staticWorldEntries != 0)
        return false;
    staticWorldEntryCount = worldObjectCount;
    staticWorldEntries = (HdStaticWorldEntry *)calloc(worldObjectCount != 0 ? worldObjectCount : 1, sizeof(*staticWorldEntries));
    if (staticWorldEntries == 0)
        return false;
    uint32 vertices = 0;
    for (uint32 index = 0; index < worldObjectCount; ++index)
        if (hd_static_world_candidate(worldObjects + index))
        {
            const HdWorldChunk *mesh = worldDefinitions + worldObjects[index].current.definition;
            uint32 count = mesh->count / 6 * 4;
            if (count > 0xffffffff - vertices)
                return false;
            vertices += count;
        }
    if (vertices != 0)
    {
        staticWorld.mesh = vid_mesh_create_3d(vertices);
        if (staticWorld.mesh == 0)
            return false;
        VID_VERTEX3D *target = vid_mesh_data_3d(staticWorld.mesh);
        uint32 used = 0;
        for (uint32 index = 0; index < worldObjectCount; ++index)
        {
            const HdObjectEntry *entry = worldObjects + index;
            if (!hd_static_world_candidate(entry))
                continue;
            const HdWorldChunk *mesh = worldDefinitions + entry->current.definition;
            if (mesh->count == 0)
                continue;
            const VID_VERTEX16 *source = mesh->sourceVertices;
            if (source == 0)
                return false;
            HdStaticWorldEntry *saved = staticWorldEntries + index;
            saved->source = entry->source;
            saved->first = used;
            saved->count = mesh->count;
            saved->definition = entry->current.definition;
            saved->light = entry->current.light;
            if (saved->light >= 0 && saved->light < Chunk::numLight)
                saved->color = Chunk::lightTable[saved->light];
            hd_object_matrix_uncached(entry, phase, 0, &saved->model);
            for (uint32 vertex = 0; vertex < mesh->count / 6 * 4; ++vertex)
            {
                const VID_VERTEX16 *in = source + vertex;
                VID_VERTEX3D *out = target + used++;
                const CVECTOR *color = saved->light >= 0 && saved->light < Chunk::numLight ? Chunk::lightTable + saved->light : 0;
                hd_world_instance_vertex(out, in, &saved->model, &worldBatchOrigin, saved->light, color);
            }
            staticWorld.count += mesh->count;
        }
        vid_mesh_commit(staticWorld.mesh);
        vid_mesh_static(staticWorld.mesh);
        for (sint32 bank = 0; bank < 8; ++bank)
            for (sint32 side = 0; side < 2; ++side)
            {
                staticWorldIndices[bank][side] = vid_index_create(staticWorld.count);
                if (staticWorldIndices[bank][side] == 0)
                    return false;
            }
    }
    for (sint32 definition = 0; definition < worldDefinitionCount; ++definition)
        if (worldDefinitions[definition].mesh != 0)
            vid_mesh_static(worldDefinitions[definition].mesh);
    staticWorldReady = true;
    return true;
}

static bool hd_static_world_view(sint32 player, float phase, const MATRIXF *camera, const HdViewPose *view, VID_RECT viewport, float logicalHeight)
{
    sint32 bank = cubeRendering ? (sint32)cubeFace + 2 : player;
    hd_static_batch_draws[bank] = hd_static_batch_objects[bank] = 0;
    if (staticWorld.mesh == 0)
        return true;
    uint32 *indices[2] = {vid_index_data(staticWorldIndices[bank][0]), vid_index_data(staticWorldIndices[bank][1])};
    uint32 used[2] = {0, 0};
    bool changed[2] = {false, false};
    const uint32 topology[6] = {0, 1, 2, 2, 1, 3};
    for (uint32 index = 0; index < worldObjectCount; ++index)
    {
        const HdObjectEntry *entry = worldObjects + index;
        if (!hd_static_world_candidate(entry) || !hd_object_visible(entry, view))
            continue;
        MATRIXF model;
        hd_object_matrix(entry, phase, camera, &model);
        if (!hd_static_world_match(index, &model))
            continue;
        const HdWorldChunk *mesh = worldDefinitions + entry->current.definition;
        if (cubeRendering && !hd_cube_mesh_visible(mesh, &model, camera))
            continue;
        ++hd_static_batch_objects[bank];
        for (sint32 side = 0; side < 2; ++side)
            for (uint32 face = 0; face < mesh->count / 6; ++face)
            {
                uint32 sourceFace = side == 0 ? face : mesh->count / 6 - 1 - face;
                if (side != 0 && !mesh->faces[sourceFace].twoSided)
                    continue;
                for (sint32 corner = 0; corner < 6; ++corner)
                {
                    uint32 value = staticWorldEntries[index].first + sourceFace * 4 + topology[corner];
                    if (indices[side][used[side]] != value)
                        changed[side] = true;
                    indices[side][used[side]++] = value;
                }
            }
    }
    MATRIXF model;
    IdentityMatrixf(&model);
    memcpy(model.t, &worldBatchOrigin, sizeof(model.t));
    for (sint32 side = 0; side < 2; ++side)
    {
        if (used[side] == 0)
            continue;
        if (changed[side] || used[side] != staticWorldCounts[bank][side])
        {
            vid_index_commit(staticWorldIndices[bank][side], used[side]);
            staticWorldCounts[bank][side] = used[side];
        }
        HdWorldChunk mesh = staticWorld;
        mesh.indices = staticWorldIndices[bank][side];
        mesh.count = used[side];
        VID_MATERIAL *material = staticWorldMaterials[(view->mirror != 0) != (side != 0) ? 1 : 0];
        vid_pass(material);
        hd_world_submit(&mesh, &model, camera, view, viewport, logicalHeight, -1, -1, 0, -1, 1, 0, 0, 0, 1, 0, material);
        ++hd_static_batch_draws[bank];
    }
    return worldSubmissionValid;
}

static bool hd_billboard_candidate(const HdObjectEntry *entry)
{
    return entry->current.active && entry->current.billboard && entry->pieceCount == 0 && entry->current.orderOffset != -1 && entry->current.definition >= 0 && entry->current.definition < worldDefinitionCount;
}

static bool hd_billboard_prepare(sint32 player, float phase)
{
    HdBillboardBank *bank = billboardBanks + player;
    MATRIXF camera;
    HdViewPose view;
    if (!hd_render_view_pose(player, phase, &camera, &view))
        return false;
    if (bank->entryCapacity < worldObjectCount)
    {
        HdStaticWorldEntry *entries = (HdStaticWorldEntry *)realloc(bank->entries, worldObjectCount * sizeof(*entries));
        if (entries == 0)
            return false;
        bank->entries = entries;
        bank->entryCapacity = worldObjectCount;
    }
    if (worldObjectCount != 0)
        memset(bank->entries, 0, worldObjectCount * sizeof(*bank->entries));
    uint32 vertices = 0;
    for (uint32 index = 0; index < worldObjectCount; ++index)
        if (hd_billboard_candidate(worldObjects + index))
        {
            uint32 count = worldDefinitions[worldObjects[index].current.definition].count / 6 * 4;
            if (count > 0xffffffff - vertices)
                return false;
            vertices += count;
        }
    if (vertices == 0)
    {
        bank->generation = renderGeneration;
        return true;
    }
    bool changed = false;
    if (vertices > bank->vertexCapacity)
    {
        vid_mesh_free(bank->mesh);
        bank->mesh = vid_mesh_create_3d(vertices);
        bank->vertexCapacity = bank->mesh != 0 ? vertices : 0;
        changed = true;
    }
    if (bank->mesh == 0)
        return false;
    if (vertices / 4 * 6 > bank->indexCapacity)
    {
        uint32 capacity = vertices / 4 * 6;
        for (sint32 target = 0; target < 8; ++target)
            if (target == player || (player == 0 && target >= 2))
                for (sint32 side = 0; side < 2; ++side)
                {
                    vid_index_free(billboardIndices[target][side]);
                    billboardIndices[target][side] = vid_index_create(capacity);
                    billboardCounts[target][side] = 0;
                    if (billboardIndices[target][side] == 0)
                        return false;
                }
        bank->indexCapacity = capacity;
    }
    VID_VERTEX3D *target = vid_mesh_data_3d(bank->mesh);
    uint32 used = 0;
    for (uint32 index = 0; index < worldObjectCount; ++index)
    {
        const HdObjectEntry *entry = worldObjects + index;
        if (!hd_billboard_candidate(entry))
            continue;
        const HdWorldChunk *mesh = worldDefinitions + entry->current.definition;
        HdStaticWorldEntry *saved = bank->entries + index;
        saved->source = entry->source;
        saved->first = used;
        saved->count = mesh->count;
        saved->definition = entry->current.definition;
        saved->light = entry->current.light;
        hd_object_matrix_uncached(entry, phase, &camera, &saved->model);
        HdObjectEntry *cache = worldObjects + index;
        cache->renderModel[player] = saved->model;
        cache->renderModelGeneration[player] = renderGeneration;
        const CVECTOR *color = saved->light >= 0 && saved->light < Chunk::numLight ? Chunk::lightTable + saved->light : 0;
        for (uint32 vertex = 0; vertex < mesh->count / 6 * 4; ++vertex)
        {
            VID_VERTEX3D value;
            hd_world_instance_vertex(&value, mesh->sourceVertices + vertex, &saved->model, &worldBatchOrigin, saved->light, color);
            if (memcmp(target + used, &value, sizeof(value)) != 0)
                changed = true;
            target[used++] = value;
        }
    }
    if (changed)
        vid_mesh_commit(bank->mesh);
    bank->generation = renderGeneration;
    return true;
}

static bool hd_billboard_match(uint32 index, sint32 player, const MATRIXF *model)
{
    const HdBillboardBank *bank = billboardBanks + player;
    if (bank->generation != renderGeneration || index >= bank->entryCapacity || !hd_billboard_candidate(worldObjects + index))
        return false;
    const HdStaticWorldEntry *saved = bank->entries + index;
    return saved->count != 0 && saved->source == worldObjects[index].source && saved->definition == worldObjects[index].current.definition && memcmp(&saved->model, model, sizeof(*model)) == 0;
}

static bool hd_billboard_view(sint32 player, float phase, const MATRIXF *camera, const HdViewPose *view, VID_RECT viewport, float logicalHeight)
{
    sint32 bankIndex = cubeRendering ? (sint32)cubeFace + 2 : player;
    const HdBillboardBank *bank = billboardBanks + (cubeRendering ? 0 : player);
    hd_billboard_batch_draws[bankIndex] = hd_billboard_batch_objects[bankIndex] = 0;
    if (bank->mesh == 0 || bank->generation != renderGeneration)
        return true;
    uint32 *indices[2] = {vid_index_data(billboardIndices[bankIndex][0]), vid_index_data(billboardIndices[bankIndex][1])};
    uint32 used[2] = {0, 0};
    bool changed[2] = {false, false};
    const uint32 topology[6] = {0, 1, 2, 2, 1, 3};
    for (uint32 index = 0; index < worldObjectCount; ++index)
    {
        const HdObjectEntry *entry = worldObjects + index;
        const HdStaticWorldEntry *saved = bank->entries + index;
        if (saved->count == 0 || !hd_object_visible(entry, view))
            continue;
        const HdWorldChunk *mesh = worldDefinitions + saved->definition;
        if (cubeRendering && !hd_cube_mesh_visible(mesh, &saved->model, camera))
            continue;
        ++hd_billboard_batch_objects[bankIndex];
        for (sint32 side = 0; side < 2; ++side)
            for (uint32 face = 0; face < mesh->count / 6; ++face)
            {
                uint32 source = side == 0 ? face : mesh->count / 6 - 1 - face;
                if (side != 0 && !mesh->faces[source].twoSided)
                    continue;
                for (sint32 corner = 0; corner < 6; ++corner)
                {
                    uint32 value = saved->first + source * 4 + topology[corner];
                    if (indices[side][used[side]] != value)
                        changed[side] = true;
                    indices[side][used[side]++] = value;
                }
            }
    }
    MATRIXF model;
    IdentityMatrixf(&model);
    memcpy(model.t, &worldBatchOrigin, sizeof(model.t));
    for (sint32 side = 0; side < 2; ++side)
    {
        if (used[side] == 0)
            continue;
        if (changed[side] || used[side] != billboardCounts[bankIndex][side])
        {
            vid_index_commit(billboardIndices[bankIndex][side], used[side]);
            billboardCounts[bankIndex][side] = used[side];
        }
        HdWorldChunk mesh = {};
        mesh.mesh = bank->mesh;
        mesh.indices = billboardIndices[bankIndex][side];
        mesh.count = used[side];
        VID_MATERIAL *material = staticWorldMaterials[(view->mirror != 0) != (side != 0) ? 1 : 0];
        vid_pass(material);
        hd_world_submit(&mesh, &model, camera, view, viewport, logicalHeight, -1, -1, 0, -1, 1, 0, 0, 0, 1, 0, material, false);
        ++hd_billboard_batch_draws[bankIndex];
    }
    return worldSubmissionValid;
}

struct HdSkyPoint
{
    float x;
    float y;
    float z;
    float w;
};

static HdSkyPoint hd_sky_project(const SVECTOR *source, const MATRIXF *camera, const HdViewPose *view, float aspect, float logicalHeight, bool perspective)
{
    const float position[3] = {(float)source->vx, (float)source->vy, (float)source->vz};
    float transformed[3] = {0, 0, 0};
    for (sint32 row = 0; row < 3; ++row)
        for (sint32 axis = 0; axis < 3; ++axis)
            transformed[row] += camera->m[axis][row] * position[axis];
    float projection = view->projection < 700 ? view->projection : 700;
    HdSkyPoint point;
    point.x = transformed[0] * 2 * projection / (logicalHeight * aspect);
    if (view->mirror)
        point.x = -point.x;
    point.y = transformed[1] * (56320.0f / 65536) * (perspective ? 2 * projection : transformed[2] * 0.5f) / logicalHeight;
    point.z = transformed[2] * 0.9999f;
    point.w = transformed[2];
    if (cubeRendering)
    {
        point.x = transformed[0];
        point.y = -transformed[1];
    }
    return point;
}

static CVECTOR hd_sky_color(const CVECTOR *previousColor, const CVECTOR *currentColor, float phase)
{
    CVECTOR color;
    color.r = (uint8)(previousColor->r + (currentColor->r - previousColor->r) * phase);
    color.g = (uint8)(previousColor->g + (currentColor->g - previousColor->g) * phase);
    color.b = (uint8)(previousColor->b + (currentColor->b - previousColor->b) * phase);
    color.cd = 0;
    return color;
}

static void hd_sky_quad(VID_VERTEX2D *vertices, uint32 *used, const HdSkyPoint *points, const CVECTOR *colors, const Draw::tPixMap *pixmap, const HdTextureRegion *region = 0)
{
    const uint8 order[6] = {0, 1, 2, 2, 1, 3};
    const uint8 u[4] = {pixmap != 0 ? pixmap->u0 : (uint8)0, pixmap != 0 ? pixmap->u1 : (uint8)0, pixmap != 0 ? pixmap->u2 : (uint8)0, pixmap != 0 ? pixmap->u3 : (uint8)0};
    const uint8 v[4] = {pixmap != 0 ? pixmap->v0 : (uint8)0, pixmap != 0 ? pixmap->v1 : (uint8)0, pixmap != 0 ? pixmap->v2 : (uint8)0, pixmap != 0 ? pixmap->v3 : (uint8)0};
    for (sint32 i = 0; i < 6; ++i)
    {
        sint32 corner = order[i];
        VID_VERTEX2D *vertex = vertices + (*used)++;
        memset(vertex, 0, sizeof(*vertex));
        vertex->pos[0] = points[corner].x;
        vertex->pos[1] = points[corner].y;
        vertex->pos[2] = points[corner].z;
        vertex->pos[3] = points[corner].w;
        vertex->color[0] = colors[corner].r;
        vertex->color[1] = colors[corner].g;
        vertex->color[2] = colors[corner].b;
        vertex->color[3] = 255;
        vertex->alpha = 1;
        if (pixmap != 0)
        {
            vertex->uv[0] = u[corner] + 0.5f;
            vertex->uv[1] = v[corner] + 0.5f;
            vertex->tex[0] = pixmap->clut;
            vertex->tex[1] = pixmap->tpage;
            vertex->tex[2] = 1;
            if (region != 0)
            {
                vertex->tex[3] = region->x | (region->y << 8);
                vertex->rect[0] = region->width;
                vertex->rect[1] = region->height;
            }
        }
    }
}

static bool hd_sky_resources(void)
{
    if (skyShader == 0)
        skyShader = vid_shader_create(skyVertex, skyFragment);
    if (skyShader == 0)
        return false;
    if (skyMaterial == 0)
    {
        skyMaterial = vid_mat_create();
        if (skyMaterial == 0)
            return false;
        VID_MATERIAL_DATA *material = vid_mat_data(skyMaterial);
        material->texture = worldVram;
        material->shader = skyShader;
        material->blend = VID_BLEND_NONE;
        material->depth_test = 0;
        material->depth_write = 0;
        material->filter = VID_FILTER_NEAREST;
        material->cull = 0;
        vid_mat_commit(skyMaterial);
    }
    uint32 stars = skyCurrent.starCount > 0 ? skyCurrent.starCount : 0;
    if (stars > (0xFFFFFFFF / 6) - 80)
        return false;
    uint32 capacity = (80 + stars) * 6;
    for (sint32 player = 0; player < 2; ++player)
        if (skyCapacities[player] < capacity)
        {
            vid_mesh_free(skyMeshes[player]);
            skyMeshes[player] = vid_mesh_create_2d(capacity);
            if (skyMeshes[player] == 0)
            {
                skyCapacities[player] = 0;
                return false;
            }
            skyCapacities[player] = capacity;
        }
    return true;
}

static void hd_celestial_build(sint32 player, float phase, const MATRIXF *camera, const HdViewPose *view, float aspect, float logicalHeight);
static void hd_celestial_submit(sint32 player, bool sun);

static void hd_sky_view(sint32 player, float phase, const MATRIXF *camera, const HdViewPose *view, VID_RECT viewport, float logicalHeight)
{
    if (skyCurrent.mesh == 0 || skyCurrent.ring == 0)
        return;
    if (skyReset)
        phase = 1;
    if (!(phase >= 0))
        phase = 0;
    if (phase > 1)
        phase = 1;
    float aspect = (float)viewport.w / viewport.h;
    if (!cubeRendering)
        hd_celestial_build(player, phase, camera, view, aspect, logicalHeight);
    VID_VERTEX2D *vertices = vid_mesh_data_2d(skyMeshes[player]);
    uint32 used = 0;
    if (skyCurrent.skyEnabled)
    {
        HdSkyPoint projected[85];
        for (sint32 i = 0; i < 85; ++i)
            projected[i] = hd_sky_project(skyCurrent.mesh + i, camera, view, aspect, logicalHeight, true);
        for (sint32 face = 63; face >= 0; --face)
        {
            sint32 first = (face * 17) / 16;
            const sint32 corners[4] = {first + 17, first + 18, first, first + 1};
            HdSkyPoint points[4];
            CVECTOR colors[4];
            const Draw::tPixMap *pixmap = 0;
            if (skyCurrent.type == 1)
            {
                uint8 index = skyCurrent.cloudIndices[face];
                if (index >= 16)
                {
                    ++hd_world_invalid_count;
                    continue;
                }
                pixmap = skyCurrent.pixmaps + index;
            }
            for (sint32 corner = 0; corner < 4; ++corner)
            {
                sint32 index = corners[corner];
                points[corner] = projected[index];
                colors[corner] = skyCurrent.type != 1 || (skyCurrent.flags & 0x20) != 0 ? hd_sky_color(skyPrevious.colors + index, skyCurrent.colors + index, phase) : hd_sky_color(&skyPrevious.frontColor, &skyCurrent.frontColor, phase);
            }
            HdTextureRegion region = {0};
            if ((hd_texture_filtering_enabled & HD_FILTER_SKY) != 0)
                hd_texture_region(pixmap, &region);
            hd_sky_quad(vertices, &used, points, colors, pixmap, &region);
            ++hd_sky_quads;
        }
    }
    uint32 skyEnd = used;
    if ((skyCurrent.flags & 0x10) != 0 && skyCurrent.stars != 0 && skyCurrent.starColors != 0)
        for (sint32 i = skyCurrent.starCount - 1; i >= 0; --i)
        {
            HdSkyPoint point = hd_sky_project(skyCurrent.stars + i, camera, view, aspect, logicalHeight, false);
            if (point.w <= 0)
                continue;
            HdSkyPoint points[4] = {point, point, point, point};
            points[1].x += 2 * point.w / (logicalHeight * aspect);
            points[2].y -= 2 * point.w / logicalHeight;
            points[3].x = points[1].x;
            points[3].y = points[2].y;
            uint32 color = skyCurrent.starColors[i];
            CVECTOR colorValue = {(uint8)color, (uint8)(color >> 8), (uint8)(color >> 16), 0};
            CVECTOR colors[4] = {colorValue, colorValue, colorValue, colorValue};
            hd_sky_quad(vertices, &used, points, colors, 0);
            ++hd_star_quads;
        }
    uint32 starEnd = used;
    if (skyCurrent.ringEnabled)
    {
        for (sint32 face = 15; face >= 0; --face)
        {
            uint8 image = skyCurrent.ringIndices[face];
            if (image >= 16)
                continue;
            float height = skyCurrent.ringPercent[face] / 65536.0f;
            sint16 top = (sint16)(skyCurrent.ringOffset + skyCurrent.ringHeight * height);
            sint16 bottom = skyCurrent.ringOffset;
            SVECTOR corners[4] = {skyCurrent.ring[face], skyCurrent.ring[face + 1], skyCurrent.ring[face], skyCurrent.ring[face + 1]};
            corners[0].vy = corners[1].vy = top;
            corners[2].vy = corners[3].vy = bottom;
            HdSkyPoint points[4];
            for (sint32 corner = 0; corner < 4; ++corner)
                points[corner] = hd_sky_project(corners + corner, camera, view, aspect, logicalHeight, true);
            CVECTOR colors[4] = {skyCurrent.ringColors[1][face], skyCurrent.ringColors[1][face + 1], skyCurrent.ringColors[0][face], skyCurrent.ringColors[0][face + 1]};
            if (image >= 8)
                for (sint32 row = 0; row < 2; ++row)
                {
                    HdSkyPoint point = points[row * 2];
                    points[row * 2] = points[row * 2 + 1];
                    points[row * 2 + 1] = point;
                    CVECTOR color = colors[row * 2];
                    colors[row * 2] = colors[row * 2 + 1];
                    colors[row * 2 + 1] = color;
                }
            HdTextureRegion region = {0};
            if ((hd_texture_filtering_enabled & HD_FILTER_SKY) != 0)
                hd_texture_region(skyCurrent.ringPixmaps + face, &region);
            hd_sky_quad(vertices, &used, points, colors, skyCurrent.ringPixmaps + face, &region);
            ++hd_ring_quads;
        }
    }
    if (used != 0)
        vid_mesh_commit(skyMeshes[player]);
    if (skyEnd != 0)
    {
        vid_pass(skyMaterial);
        hd_submit(skyMeshes[player], 0, 0, skyEnd);
    }
    if (!cubeRendering)
        hd_celestial_submit(player, true);
    if (starEnd > skyEnd)
    {
        vid_pass(skyMaterial);
        hd_submit(skyMeshes[player], 0, skyEnd, starEnd - skyEnd);
    }
    if (!cubeRendering)
        hd_celestial_submit(player, false);
    if (used > starEnd)
    {
        vid_pass(skyMaterial);
        hd_submit(skyMeshes[player], 0, starEnd, used - starEnd);
    }
}

static const char effectVertex[] = "layout(location=0) in vec4 aPos;layout(location=1) in vec2 aUV;layout(location=2) in vec4 aColor;layout(location=5) in uvec4 aTex;layout(location=6) in uvec2 aRect;\n"
                                   "out vec2 vUV;out vec4 vColor;out float vDepth;out vec2 vFog;out vec3 vAtmospherePosition;flat out uvec4 vSkyTex;flat out ivec4 vEffectRegion;uniform vec4 uDraw[18];\n"
                                   "void main(){gl_Position=aPos;vUV=aUV;vColor=aColor/255.0;vDepth=aPos.w;vFog=vec2(0.0);vSkyTex=aTex;vEffectRegion=ivec4(ivec2(aTex.w&255u,aTex.w>>8),ivec2(aRect));\n"
                                   "vAtmospherePosition=vec3(abs(uDraw[17].x)>0.000001?aPos.x/uDraw[17].x:0.0,abs(uDraw[17].y)>0.000001?aPos.y/uDraw[17].y:0.0,aPos.w); }\n";

static const char weatherFragment[] = "flat in uvec4 vSkyTex;flat in ivec4 vEffectRegion;\n" VID_PSX_SAMPLING_GLSL HD_VRAM_WORD_GLSL HD_PSX_COLOR_GLSL HD_FOG_GLSL "void main(){if(vSkyTex.z==0u){outColor=vec4(hdFog(vColor.rgb),1.0);return;}\n"
                                      "vec4 color=hdPsxColor(vUV,vSkyTex.y,vSkyTex.x,vEffectRegion,uint(uParams[0].x),vidPsxDecode(vSkyTex.y,vSkyTex.x,ivec2(floor(vUV))));\n"
                                      "if(color.a==0.0)discard;vec3 rgb=(vSkyTex.z&2u)!=0u?color.rgb:color.rgb*vColor.rgb*(255.0/128.0);\n"
                                      "outColor=vec4(hdFog(clamp(rgb,0.0,1.0)),uParams[0].y); }\n";

static bool hd_souffle_resources(void)
{
    bool active = false;
    for (sint32 slot = 0; slot < HD_SOUFFLE_PARTICLES; ++slot)
        active = active || souffleStates[slot].current.active;
    if (!active)
        return true;
    if (weatherShader == 0)
        weatherShader = vid_shader_create(effectVertex, weatherFragment);
    if (weatherShader == 0)
        return false;
    for (sint32 pass = 0; pass < 6; ++pass)
        if (souffleMaterials[pass] == 0)
        {
            souffleMaterials[pass] = vid_mat_create();
            if (souffleMaterials[pass] == 0)
                return false;
            VID_MATERIAL_DATA *material = vid_mat_data(souffleMaterials[pass]);
            material->texture = worldVram;
            material->shader = weatherShader;
            material->params[0] = pass == 5 ? 0.0f : pass == 0 ? 1.0f : 2.0f;
            material->params[1] = pass == 1 ? 0.5f : pass == 4 ? 0.25f : 1.0f;
            material->blend = pass == 0 || pass == 5 ? VID_BLEND_NONE : pass == 1 ? VID_BLEND_ALPHA : pass == 3 ? VID_BLEND_SUBTRACT : VID_BLEND_ADD;
            material->depth_test = 1;
            material->depth_write = 0;
            material->filter = VID_FILTER_NEAREST;
            vid_mat_commit(souffleMaterials[pass]);
        }
    for (sint32 player = 0; player < 2; ++player)
        if (souffleMeshes[player] == 0)
        {
            souffleMeshes[player] = vid_mesh_create_2d(HD_SOUFFLE_PARTICLES * 6);
            if (souffleMeshes[player] == 0)
                return false;
        }
    return true;
}

static VEC3F hd_souffle_camera(const VEC3F *position, const MATRIXF *camera)
{
    float relative[3] = {position->x - camera->t[0], position->y - camera->t[1], position->z - camera->t[2]};
    float local[3] = {0};
    for (sint32 row = 0; row < 3; ++row)
        for (sint32 axis = 0; axis < 3; ++axis)
            local[row] += camera->m[axis][row] * relative[axis];
    VEC3F result = {local[0], local[1] * (56320.0f / 65536), local[2]};
    return result;
}

static bool hd_souffle_quad(const HdSouffleState *state, float phase, const MATRIXF *camera, const HdViewPose *view, float logicalWidth, float logicalHeight, HdSkyPoint *points, sint32 *bucket)
{
    if (!state->current.active)
        return false;
    if (!(phase >= 0))
        phase = 0;
    if (phase > 1 || state->reset)
        phase = 1;
    const HdSouffleSource *from = &state->previous, *to = &state->current;
    VEC3F world[4], local[4];
    for (sint32 corner = 0; corner < 4; ++corner)
    {
        world[corner].x = from->points[corner].x + (to->points[corner].x - from->points[corner].x) * phase;
        world[corner].y = from->points[corner].y + (to->points[corner].y - from->points[corner].y) * phase;
        world[corner].z = from->points[corner].z + (to->points[corner].z - from->points[corner].z) * phase;
        local[corner] = hd_souffle_camera(world + corner, camera);
        if (view->mirror)
            local[corner].x = -local[corner].x;
    }
    float dx = fabsf(world[0].x - camera->t[0]), dz = fabsf(world[0].z - camera->t[2]);
    if ((dx > dz ? dx + dz * 0.25f : dz + dx * 0.25f) >= 3200)
        return false;
    if (to->kind == 1)
    {
        float angle = to->angle - from->angle;
        if (angle > 512)
            angle -= 1024;
        if (angle < -512)
            angle += 1024;
        angle = (from->angle + angle * phase) * (6.28318530718f / 1024);
        float radius = from->radius + (to->radius - from->radius) * phase;
        float sine = sinf(angle) * radius, cosine = cosf(angle) * radius;
        VEC3F center = local[0];
        const float x[4] = {-sine, cosine, -cosine, sine};
        const float y[4] = {-cosine, -sine, sine, cosine};
        for (sint32 corner = 0; corner < 4; ++corner)
        {
            local[corner] = center;
            local[corner].x += x[corner];
            local[corner].y += y[corner];
        }
    }
    else if (to->kind == 2)
    {
        if (local[1].z < 64)
            return false;
        if (fabsf(local[1].x - local[0].x) < 32)
            local[1].x = local[0].x + (local[1].x > local[0].x ? 32 : -32);
        local[2] = local[0];
        local[3] = local[1];
        local[0].y -= 32;
        local[1].y -= 32;
        local[2].y += 32;
        local[3].y += 32;
    }
    else if (local[1].z < 32)
        return false;
    float sumZ = 0;
    for (sint32 corner = 0; corner < 4; ++corner)
    {
        sumZ += local[corner].z;
        points[corner].x = local[corner].x * 2 * view->projection / logicalWidth;
        points[corner].y = local[corner].y * 2 * view->projection / logicalHeight;
        points[corner].z = local[corner].z * (65536.0f / 65534) - 131070.0f / 65534;
        points[corner].w = local[corner].z;
    }
    *bucket = (sint32)floorf(sumZ / 32) + to->offset;
    return *bucket >= 0 && *bucket <= view->orderTableSize - 3;
}

static void hd_souffle_view(sint32 player, float phase, const MATRIXF *camera, const HdViewPose *view, VID_RECT viewport, float logicalHeight)
{
    if (souffleMeshes[player] == 0 || hd_souffle_enabled == 0)
        return;

    struct Command
    {
        uint32 first;
        sint32 bucket;
        sint32 blend;
    } commands[HD_SOUFFLE_PARTICLES];

    uint32 count = 0, used = 0;
    VID_VERTEX2D *vertices = vid_mesh_data_2d(souffleMeshes[player]);
    for (sint32 slot = 0; slot < HD_SOUFFLE_PARTICLES; ++slot)
    {
        const HdSouffleState *state = souffleStates + slot;
        HdSkyPoint points[4];
        sint32 bucket;
        if (!hd_souffle_quad(state, phase, camera, view, logicalHeight * viewport.w / viewport.h, logicalHeight, points, &bucket))
            continue;
        CVECTOR color = state->current.color;
        CVECTOR colors[4] = {color, color, color, color};
        uint32 first = used;
        HdTextureRegion region = {0};
        if ((state->current.type == 1 || state->current.type == 2 || state->current.type == 3 || state->current.type == 6 || state->current.type == 11) && (hd_texture_filtering_enabled & HD_FILTER_SMOKE) != 0)
            hd_texture_region(&state->current.pixmap, &region);
        if (region.width != 0 && region.height != 0)
            ++hd_souffle_filtered_quads;
        hd_sky_quad(vertices, &used, points, colors, &state->current.pixmap, &region);
        if ((color.cd & 1) != 0)
            for (uint32 vertex = first; vertex < used; ++vertex)
                vertices[vertex].tex[2] |= 2;
        Command draw = {first, bucket, (color.cd & 2) != 0 ? (state->current.pixmap.tpage >> 5) & 3 : -1};
        uint32 index = count++;
        while (index > 0 && commands[index - 1].bucket <= bucket)
        {
            commands[index] = commands[index - 1];
            --index;
        }
        commands[index] = draw;
        ++hd_souffle_quads;
    }
    if (used == 0)
        return;
    vid_mesh_commit(souffleMeshes[player]);
    for (uint32 index = 0; index < count; ++index)
    {
        const Command *draw = commands + index;
        if (draw->blend < 0)
        {
            HdOrderKey order = {draw->bucket, HD_ORDER_SOUFFLE, 0, 0, 0, draw->first / 6, 0};
            hd_order_draw(&order, souffleMaterials[5], souffleMeshes[player], 0, draw->first, 6);
        }
        else
            for (sint32 pass = 0; pass < 2; ++pass)
            {
                HdOrderKey order = {draw->bucket, HD_ORDER_SOUFFLE, 0, 0, 0, draw->first / 6, (uint32)pass};
                hd_order_draw(&order, souffleMaterials[pass == 0 ? 0 : draw->blend + 1], souffleMeshes[player], 0, draw->first, 6);
            }
    }
}

static bool hd_sprite_materials(void)
{
    if (weatherShader == 0)
        weatherShader = vid_shader_create(effectVertex, weatherFragment);
    if (weatherShader == 0)
        return false;
    for (sint32 pass = 0; pass < 5; ++pass)
        if (weatherMaterials[pass] == 0)
        {
            weatherMaterials[pass] = vid_mat_create();
            if (weatherMaterials[pass] == 0)
                return false;
            VID_MATERIAL_DATA *material = vid_mat_data(weatherMaterials[pass]);
            material->texture = worldVram;
            material->shader = weatherShader;
            material->params[0] = pass == 0 ? 1.0f : 2.0f;
            material->params[1] = pass == 1 ? 0.5f : pass == 4 ? 0.25f : 1.0f;
            material->blend = pass == 0 ? VID_BLEND_NONE : pass == 1 ? VID_BLEND_ALPHA : pass == 3 ? VID_BLEND_SUBTRACT : VID_BLEND_ADD;
            material->depth_test = 0;
            material->depth_write = 0;
            material->filter = VID_FILTER_NEAREST;
            vid_mat_commit(weatherMaterials[pass]);
        }
    return true;
}

static bool hd_weather_resources(void)
{
    if (!weatherStates[0].valid && !weatherStates[1].valid)
        return true;
    if (!hd_sprite_materials())
        return false;
    for (sint32 player = 0; player < 2; ++player)
        if (weatherMeshes[player] == 0)
        {
            weatherMeshes[player] = vid_mesh_create_2d((HD_WEATHER_PARTICLES + HD_WEATHER_SPLATS) * 6);
            if (weatherMeshes[player] == 0)
                return false;
        }
    return true;
}

static bool hd_lens_eligible(void)
{
    return current.viewCount == 1 && skyCurrent.skyEnabled && (skyCurrent.celestialFlags & 260) == 4 && skyCurrent.lensSize > 0;
}

static bool hd_lens_resources(void)
{
    if (!hd_lens_eligible())
    {
        hd_reset_lens();
        return true;
    }
    if (!hd_sprite_materials())
        return false;
    if (lensMesh == 0)
        lensMesh = vid_mesh_create_2d(88 * 6);
    if (lensMesh == 0)
        return false;
    return hd_lens_enabled == 0 || hd_celestial_enabled == 0 || hd_lens_gpu_prepare(worldVram);
}

static bool hd_celestial_resources(void)
{
    if ((skyCurrent.celestialFlags & 12) == 0 && !skyCurrent.lightningActive)
        return true;
    if (!hd_sprite_materials())
        return false;
    for (sint32 player = 0; player < 2; ++player)
        if (celestialMeshes[player] == 0)
        {
            celestialMeshes[player] = vid_mesh_create_2d(8 * 3 + 18 * 6);
            if (celestialMeshes[player] == 0)
                return false;
        }
    return true;
}

static void hd_celestial_rect(VID_VERTEX2D *vertices, uint32 *used, float left, float firstY, float right, float lastY, float width, float height, CVECTOR color, const Draw::tPixMap *pixmap)
{
    HdSkyPoint points[4] = {{2 * left / width, 2 * firstY / height, 0, 1}, {2 * right / width, 2 * firstY / height, 0, 1}, {2 * left / width, 2 * lastY / height, 0, 1}, {2 * right / width, 2 * lastY / height, 0, 1}};
    CVECTOR colors[4] = {color, color, color, color};
    hd_sky_quad(vertices, used, points, colors, pixmap);
}

static bool hd_celestial_center(const SVECTOR *source, const MATRIXF *camera, const HdViewPose *view, float aspect, float logicalHeight, bool perspective, float *x, float *y)
{
    HdSkyPoint point = hd_sky_project(source, camera, view, aspect, logicalHeight, perspective);
    if (point.w <= 0)
        return false;
    *x = point.x * logicalHeight * aspect / (2 * point.w);
    *y = point.y * logicalHeight / (2 * point.w);
    return true;
}

static void hd_celestial_build(sint32 player, float phase, const MATRIXF *camera, const HdViewPose *view, float aspect, float logicalHeight)
{
    celestialCommandCount[player] = 0;
    if (player == 0)
        lensMarkerValid = false;
    if (celestialMeshes[player] == 0 || hd_celestial_enabled == 0)
        return;
    if (!(phase >= 0))
        phase = 0;
    if (phase > 1 || skyReset)
        phase = 1;
    VID_VERTEX2D *vertices = vid_mesh_data_2d(celestialMeshes[player]);
    HdCelestialCommand *commands = celestialCommands[player];
    uint32 count = 0, used = 0;
    float x, y, width = logicalHeight * aspect;
    CVECTOR neutral = {128, 128, 128, 0};
    if ((skyCurrent.celestialFlags & 4) != 0 && skyCurrent.skyEnabled && hd_celestial_center(&skyCurrent.sun, camera, view, aspect, logicalHeight, true, &x, &y))
    {
        uint32 first = used;
        sint32 blend = 1;
        if ((skyCurrent.celestialFlags & 256) != 0)
        {
            if (skyCurrent.celestialPixmapValid)
            {
                hd_celestial_rect(vertices, &used, x - 16, y - 16, x + 16, y + 16, width, logicalHeight, skyCurrent.sunColor, &skyCurrent.celestialPixmap);
                blend = (skyCurrent.celestialPixmap.tpage >> 5) & 3;
                hd_sun_triangles += 2;
            }
        }
        else
        {
            float originalProjection = view->projection < 724 ? view->projection : 724;
            float activeProjection = view->projection < 700 ? view->projection : 700;
            float radius = originalProjection > 0 ? 16 * activeProjection / originalProjection : 0;
            for (sint32 edge = 0; edge < 8; ++edge)
            {
                const SVECTOR *from = skyCurrent.sunOctagon + edge;
                const SVECTOR *to = skyCurrent.sunOctagon + ((edge + 1) & 7);
                float centerScale = originalProjection > 0 ? activeProjection / originalProjection : 0;
                HdSkyPoint points[4] = {{2 * (x * centerScale + to->vx * radius / 256) / width, 2 * (y * centerScale - to->vy * radius / 256) / logicalHeight, 0, 1}, {2 * x / width, 2 * y / logicalHeight, 0, 1}, {2 * (x * centerScale + from->vx * radius / 256) / width, 2 * (y * centerScale - from->vy * radius / 256) / logicalHeight, 0, 1}, {0, 0, 0, 1}};
                CVECTOR black = {0, 0, 0, 0};
                CVECTOR colors[4] = {black, neutral, black, black};
                VID_VERTEX2D triangle[6];
                uint32 triangleCount = 0;
                hd_sky_quad(triangle, &triangleCount, points, colors, 0);
                memcpy(vertices + used, triangle, 3 * sizeof(*vertices));
                used += 3;
                ++hd_sun_triangles;
            }
            blend = -1;
        }
        if (used != first)
        {
            HdCelestialCommand command = {first, used - first, blend, true};
            commands[count++] = command;
        }
        if (player == 0 && current.viewCount == 1 && (skyCurrent.celestialFlags & 256) == 0 && skyCurrent.lensSize > 0 && x < width * 0.5f - 3)
        {
            lensMarkerValid = true;
            lensCenterX = x;
            lensCenterY = y;
            uint32 first = used;
            CVECTOR white = {255, 255, 255, 0};
            hd_celestial_rect(vertices, &used, x - 2, y + 2, x + 3, y - 3, width, logicalHeight, white, 0);
            HdCelestialCommand command = {first, 6, -2, true};
            commands[count++] = command;
        }
    }
    if ((skyCurrent.celestialFlags & 8) != 0 && skyCurrent.celestialPixmapValid && hd_celestial_center(&skyCurrent.moon, camera, view, aspect, logicalHeight, false, &x, &y))
    {
        uint32 first = used;
        hd_celestial_rect(vertices, &used, x - 16, y - 16, x + 16, y + 16, width, logicalHeight, neutral, &skyCurrent.celestialPixmap);
        HdCelestialCommand command = {first, 6, (skyCurrent.celestialPixmap.tpage >> 5) & 3, false};
        commands[count++] = command;
        ++hd_moon_quads;
    }
    if (skyCurrent.lightningActive && hd_celestial_center(&skyCurrent.lightning, camera, view, aspect, logicalHeight, true, &x, &y))
    {
        float brightness = (float)skyCurrent.lightningBrightness;
        if (skyPrevious.lightningActive && skyPrevious.lightningCount == skyCurrent.lightningCount && memcmp(&skyPrevious.lightning, &skyCurrent.lightning, sizeof(skyCurrent.lightning)) == 0 && memcmp(skyPrevious.lightningOffsets, skyCurrent.lightningOffsets, sizeof(skyCurrent.lightningOffsets)) == 0)
            brightness = skyPrevious.lightningBrightness + (skyCurrent.lightningBrightness - skyPrevious.lightningBrightness) * phase;
        if (brightness < 0)
            brightness = 0;
        if (brightness > 255)
            brightness = 255;
        CVECTOR color = {(uint8)brightness, (uint8)brightness, (uint8)brightness, 0};
        for (sint32 fork = skyCurrent.lightningCount - 1; fork >= 0; --fork)
        {
            uint32 first = used;
            float left = x + skyCurrent.lightningOffsets[fork].vx;
            float top = y - skyCurrent.lightningOffsets[fork].vy;
            hd_celestial_rect(vertices, &used, left, top, left + 16, top - 16, width, logicalHeight, color, skyCurrent.lightningPixmaps + fork);
            HdCelestialCommand command = {first, 6, (skyCurrent.lightningPixmaps[fork].tpage >> 5) & 3, false};
            commands[count++] = command;
            ++hd_lightning_quads;
        }
    }
    celestialCommandCount[player] = count;
    if (used != 0)
        vid_mesh_commit(celestialMeshes[player]);
}

static void hd_celestial_submit(sint32 player, bool sun)
{
    for (uint32 index = 0; index < celestialCommandCount[player]; ++index)
    {
        const HdCelestialCommand *command = celestialCommands[player] + index;
        if (command->sun != sun)
            continue;
        if (command->blend < 0)
        {
            hd_effect_pass(command->blend == -2 ? skyMaterial : weatherMaterials[2]);
            hd_submit(celestialMeshes[player], 0, command->first, command->count);
        }
        else
            for (sint32 pass = 0; pass < 2; ++pass)
            {
                hd_effect_pass(weatherMaterials[pass == 0 ? 0 : command->blend + 1]);
                hd_submit(celestialMeshes[player], 0, command->first, command->count);
            }
    }
}

struct HdLensPoint
{
    float x;
    float y;
};

struct HdLensCommand
{
    uint32 first;
    sint32 blend;
};

static void hd_lens_quad(VID_VERTEX2D *vertices, uint32 *used, HdLensCommand *commands, uint32 *count, const HdLensPoint *positions, const CVECTOR *colors, const Draw::tPixMap *pixmap, float width, float height)
{
    HdSkyPoint points[4];
    for (sint32 i = 0; i < 4; ++i)
    {
        points[i].x = positions[i].x * 2 / width;
        points[i].y = positions[i].y * 2 / height;
        points[i].z = 0;
        points[i].w = 1;
    }
    HdLensCommand command = {*used, pixmap != 0 ? (pixmap->tpage >> 5) & 3 : -1};
    commands[(*count)++] = command;
    hd_sky_quad(vertices, used, points, colors, pixmap);
}

static uint32 hd_lens_geometry(VID_VERTEX2D *vertices, HdLensCommand *commands, const HdViewPose *view, float width, float height)
{
    uint32 used = 0, count = 0;
    float visibility = 25;
    CVECTOR black = {0, 0, 0, 0};
    for (sint32 part = 8; part >= 0; --part)
    {
        const HdLensPiece *piece = skyCurrent.lensPieces + part;
        float radius = floorf(skyCurrent.lensSize * (piece->size / 65536.0f));
        if (radius < 4)
            continue;
        float distance = piece->distance / 65536.0f;
        float x = lensCenterX * (1 - 2 * distance), y = lensCenterY * (1 - 2 * distance);
        CVECTOR color = {(uint8)(piece->color.r * visibility / 25), (uint8)(piece->color.g * visibility / 25), (uint8)(piece->color.b * visibility / 25), 0};
        CVECTOR colors[4] = {color, color, color, color};
        if (piece->type <= 1)
        {
            if (!skyCurrent.lensPixmapValid[piece->type])
                continue;
            HdLensPoint points[4] = {{x - radius, y - radius}, {x + radius, y - radius}, {x - radius, y + radius}, {x + radius, y + radius}};
            hd_lens_quad(vertices, &used, commands, &count, points, colors, skyCurrent.lensPixmaps + piece->type, width, height);
        }
        else if (piece->type == 2)
        {
            HdLensPoint points[6] = {{x - radius * 0.25f, y - radius * 0.5f}, {x + radius * 0.25f, y - radius * 0.5f}, {x - radius * 0.5f, y}, {x + radius * 0.5f, y}, {x - radius * 0.25f, y + radius * 0.5f}, {x + radius * 0.25f, y + radius * 0.5f}};
            for (sint32 quad = 1; quad >= 0; --quad)
                hd_lens_quad(vertices, &used, commands, &count, points + quad * 2, colors, 0, width, height);
        }
        else if (piece->type == 3)
        {
            HdLensPoint points[8];
            for (sint32 i = 0; i < 8; ++i)
            {
                points[i].x = x + skyCurrent.lensOctagon[i].vx * radius / 256;
                points[i].y = y - skyCurrent.lensOctagon[i].vy * radius / 256;
            }
            for (sint32 quad = 2; quad >= 0; --quad)
                hd_lens_quad(vertices, &used, commands, &count, points + quad * 2, colors, 0, width, height);
        }
        else if (piece->type == 4)
        {
            CVECTOR ringColors[4] = {color, black, color, black};
            for (sint32 edge = 7; edge >= 0; --edge)
            {
                const SVECTOR *from = skyCurrent.sunOctagon + edge, *to = skyCurrent.sunOctagon + ((edge + 1) & 7);
                HdLensPoint points[4] = {{x + from->vx * radius / 256, y - from->vy * radius / 256}, {x + from->vx * (radius - 5) / 256, y - from->vy * (radius - 5) / 256}, {x + to->vx * radius / 256, y - to->vy * radius / 256}, {x + to->vx * (radius - 5) / 256, y - to->vy * (radius - 5) / 256}};
                hd_lens_quad(vertices, &used, commands, &count, points, ringColors, 0, width, height);
            }
        }
    }
    float activeProjection = view->projection < 700 ? view->projection : 700;
    float originalProjection = view->projection < 724 ? view->projection : 724;
    float projection = originalProjection > 0 ? activeProjection / (4 * originalProjection) : 0;
    float sx = lensCenterX + width * 0.5f, sy = height * 0.5f - lensCenterY;
    uint32 spikeStart = used;
    for (sint32 pass = 1; pass >= 0; --pass)
    {
        float angle = (sx + sy) * (pass == 0 ? 8 : 6) * (6.28318530718f / 4096);
        float sine = sinf(angle), cosine = cosf(angle);
        float scaleX = visibility / (pass == 0 ? 32 : 64), scaleY = visibility / 64;
        HdLensPoint projected[12];
        for (sint32 i = 0; i < 12; ++i)
        {
            const SVECTOR *point = i < 4 ? skyCurrent.lensSpikes + i : skyCurrent.sunOctagon + i - 4;
            projected[i].x = lensCenterX * projection * 4 + (point->vx * cosine - point->vy * sine) * scaleX * projection;
            projected[i].y = lensCenterY * projection * 4 - (point->vx * sine + point->vy * cosine) * scaleY * projection;
        }
        for (sint32 edge = 0; edge < 8; ++edge)
        {
            uint8 from = skyCurrent.lensSpikeFrom[edge], to = skyCurrent.lensSpikeTo[edge];
            if (from >= 4 || to >= 4)
                continue;
            HdLensPoint points[4] = {projected[from], {lensCenterX, lensCenterY}, projected[edge + 4], projected[to]};
            CVECTOR colors[4] = {black, skyCurrent.sunBeamColor, black, black};
            hd_lens_quad(vertices, &used, commands, &count, points, colors, 0, width, height);
        }
    }
    for (uint32 vertex = spikeStart; vertex < used; ++vertex)
    {
        uint32 corner = (vertex - spikeStart) % 6;
        vertices[vertex].tex[3] |= corner == 1 || corner == 4 ? 8192 : 24576;
    }
    return count;
}

static void hd_lens_view(const HdViewPose *view, VID_RECT viewport, float logicalHeight, float phase)
{
    if (!hd_lens_eligible() || !lensMarkerValid || hd_celestial_enabled == 0 || hd_lens_enabled == 0 || lensMesh == 0)
        return;
    HdLensCommand commands[88];
    uint32 count = hd_lens_geometry(vid_mesh_data_2d(lensMesh), commands, view, logicalHeight * viewport.w / viewport.h, logicalHeight);
    if (count == 0)
        return;
    vid_mesh_commit(lensMesh);
    float activeProjection = view->projection < 700 ? view->projection : 700;
    float originalProjection = view->projection < 724 ? view->projection : 724;
    float ratio = originalProjection > 0 ? activeProjection / originalProjection : 0;
    float centerX = lensCenterX * 2 * ratio / (logicalHeight * viewport.w / viewport.h);
    float centerY = lensCenterY * 2 * ratio / logicalHeight;
    for (uint32 index = 0; index < count; ++index)
    {
        const HdLensCommand *command = commands + index;
        if (command->blend < 0)
        {
            hd_lens_gpu_pass(2, phase, centerX, centerY);
            hd_submit(lensMesh, 0, command->first, 6);
        }
        else
            for (sint32 pass = 0; pass < 2; ++pass)
            {
                hd_lens_gpu_pass(pass == 0 ? 0 : command->blend + 1, phase, centerX, centerY);
                hd_submit(lensMesh, 0, command->first, 6);
            }
    }
    hd_lens_quads += count;
}

static bool hd_car_halo_resources(void)
{
    uint32 faces = 0;
    for (sint32 slot = 0; slot < 9; ++slot)
        if (carModels[slot].active)
            for (sint32 part = 0; part < 57; ++part)
            {
                const HdCarPart *source = carModels[slot].parts + part;
                bool halo = false;
                for (sint32 detail = 0; detail < 4; ++detail)
                    halo = halo || source->poses[detail].visible >= 17;
                if (halo && source->object != 0 && source->object->numFacet > 0)
                {
                    uint32 count = source->object->numFacet;
                    if (count > 0xffffffff / 90 - faces)
                        return false;
                    faces += count;
                }
            }
    if (faces == 0)
        return true;
    if (!hd_sprite_materials())
        return false;
    if (carHaloMaterial == 0)
    {
        carHaloMaterial = vid_mat_create();
        if (carHaloMaterial == 0)
            return false;
        VID_MATERIAL_DATA *material = vid_mat_data(carHaloMaterial);
        material->shader = weatherShader;
        material->texture = worldVram;
        material->blend = VID_BLEND_ADD;
        material->depth_test = 1;
        material->depth_write = 0;
        vid_mat_commit(carHaloMaterial);
    }
    for (sint32 player = 0; player < 2; ++player)
        if (carHaloCapacities[player] < faces * 90)
        {
            vid_mesh_free(carHaloMeshes[player]);
            carHaloMeshes[player] = vid_mesh_create_2d(faces * 90);
            carHaloCapacities[player] = carHaloMeshes[player] != 0 ? faces * 90 : 0;
            if (carHaloMeshes[player] == 0)
                return false;
        }
    return true;
}

static sint32 hd_car_halo_type(const HdCarModel *car, uint8 visible, sint16 flag)
{
    if (visible < 17 || visible >= 23)
        return -1;
    const HdCarHaloSource *source = car->haloTypes + visible - 17;
    sint32 type = source->type, selected = type & 0xffbf;
    if (type < 0)
        return -1;
    if (source->overlay >= 0)
    {
        sint32 value = flag < 0 ? (sint8)((uint16)car->overlays[source->overlay] >> 8) : (uint8)car->overlays[source->overlay];
        if ((type & 0x40) != 0 && (value & 0x40) == 0)
            value = flag < 0 ? (sint8)((uint16)car->overlays[24] >> 8) : (uint8)car->overlays[24];
        if ((value & 0x81) == 0)
            return -1;
        if ((value & 3) != 1 && (type & 0x7f00) != 0)
            selected >>= 8;
    }
    return selected & 255;
}

static VEC3F hd_car_halo_transform(const VEC3F *source, const MATRIXF *model, const MATRIXF *camera, const HdViewPose *view, bool vector)
{
    float world[3] = {0}, local[3] = {0};
    for (sint32 row = 0; row < 3; ++row)
    {
        for (sint32 axis = 0; axis < 3; ++axis)
            world[row] += model->m[row][axis] * source->v[axis];
        if (!vector)
            world[row] = world[row] * 0.25f + model->t[row] - camera->t[row];
    }
    for (sint32 row = 0; row < 3; ++row)
        for (sint32 axis = 0; axis < 3; ++axis)
            local[row] += camera->m[axis][row] * world[axis];
    float scale = vector ? 1.0f : 4.0f;
    VEC3F result = {local[0] * scale * (view->mirror ? -1 : 1), -local[1] * scale * (56320.0f / 65536), local[2] * scale};
    return result;
}

static void hd_car_halo_emit(VID_VERTEX2D *vertices, uint32 *used, const HdLensPoint *screen, const CVECTOR *colors, sint32 count, float depth, float width, float height)
{
    HdSkyPoint points[4];
    for (sint32 i = 0; i < count; ++i)
    {
        points[i].x = screen[i].x * 2 * depth / width;
        points[i].y = -screen[i].y * 2 * depth / height;
        points[i].z = depth * (65536.0f / 65534) - 131070.0f / 65534;
        points[i].w = depth;
    }
    if (count == 3)
    {
        points[3] = points[2];
        CVECTOR fourColors[4] = {colors[0], colors[1], colors[2], colors[2]};
        VID_VERTEX2D temporary[6];
        uint32 total = 0;
        hd_sky_quad(temporary, &total, points, fourColors, 0);
        memcpy(vertices + *used, temporary, 3 * sizeof(*vertices));
        *used += 3;
    }
    else
        hd_sky_quad(vertices, used, points, colors, 0);
}

static void hd_halo_shape(VID_VERTEX2D *vertices, uint32 *used, HdLensPoint screen, CVECTOR halo, CVECTOR beam, float sx, float sy, float spin, float angleOuter, float projection, sint32 shape, float depth, float width, float height)
{
    CVECTOR black = {0, 0, 0, 0};
    float sine = sinf(spin), cosine = cosf(spin);
    float outerSine = sinf(angleOuter), outerCosine = cosf(angleOuter);
    HdLensPoint points[12];
    sint32 pointCount = shape == 0 ? 12 : shape == 1 ? 8 : shape == 2 ? 4 : 6;
    for (sint32 i = 0; i < pointCount; ++i)
    {
        const SVECTOR *v = shape == 0 ? (i < 4 ? haloTables.spikes + i : haloTables.octagon + i - 4) : shape == 1 ? haloTables.octagon + i : shape == 2 ? haloTables.quad + i : haloTables.reflection + i;
        float x = (v->vx * cosine - v->vy * sine) * sx;
        float y = (v->vx * sine + v->vy * cosine) * sy;
        points[i].x = screen.x + (x * outerCosine - y * outerSine) * projection;
        points[i].y = screen.y + (x * outerSine + y * outerCosine) * projection;
    }
    if (shape == 1)
        for (sint32 i = 7; i >= 0; --i)
        {
            float x = points[i].x - screen.x, y = points[i].y - screen.y;
            float length = sqrtf(x * x + y * y);
            if (length <= 0.0001f)
                continue;
            float dx = -y * 0.5f / length, dy = x * 0.5f / length;
            HdLensPoint line[4] = {{screen.x + dx, screen.y + dy}, {screen.x - dx, screen.y - dy}, {points[i].x + dx, points[i].y + dy}, {points[i].x - dx, points[i].y - dy}};
            CVECTOR colors[4] = {beam, beam, black, black};
            hd_car_halo_emit(vertices, used, line, colors, 4, depth, width, height);
        }
    sint32 sides = shape == 0 ? 8 : shape == 2 ? 4 : 6;
    for (sint32 i = 0; i < sides; ++i)
    {
        sint32 edge = shape == 2 ? 3 - i : i;
        HdLensPoint from, to;
        if (shape == 1)
        {
            const SVECTOR *a = haloTables.hexagon + edge, *b = haloTables.hexagon + ((edge + 1) % 6);
            float x = (a->vx * cosine - a->vy * sine) * sx, y = (a->vx * sine + a->vy * cosine) * sy;
            from.x = screen.x + (x * outerCosine - y * outerSine) * projection;
            from.y = screen.y + (x * outerSine + y * outerCosine) * projection;
            x = (b->vx * cosine - b->vy * sine) * sx;
            y = (b->vx * sine + b->vy * cosine) * sy;
            to.x = screen.x + (x * outerCosine - y * outerSine) * projection;
            to.y = screen.y + (x * outerSine + y * outerCosine) * projection;
        }
        else
        {
            sint32 start = shape == 0 ? 4 : 0;
            from = points[start + edge];
            to = points[start + (edge + 1) % sides];
        }
        HdLensPoint triangle[3] = {to, screen, from};
        CVECTOR colors[3] = {black, halo, black};
        hd_car_halo_emit(vertices, used, triangle, colors, 3, depth, width, height);
        if (shape == 0)
        {
            HdLensPoint quad[4] = {points[haloTables.spikeFrom[edge]], screen, points[edge + 4], points[haloTables.spikeTo[edge]]};
            CVECTOR colors[4] = {black, beam, black, black};
            hd_car_halo_emit(vertices, used, quad, colors, 4, depth, width, height);
        }
    }
}

static VEC3F hd_halo_transform(const VEC3F *point, const MATRIXF *model, const MATRIXF *camera, const HdViewPose *view, bool vector, const float (*menu)[4])
{
    if (menu == 0)
        return hd_car_halo_transform(point, model, camera, view, vector);
    VEC3F result = {0};
    for (sint32 row = 0; row < 3; ++row)
    {
        for (sint32 axis = 0; axis < 3; ++axis)
            result.v[row] += menu[row][axis] * point->v[axis];
        if (!vector)
            result.v[row] += menu[row][3];
    }
    return result;
}

static void hd_car_halo_geometry(VID_VERTEX2D *vertices, uint32 *used, const COORD16 *source, sint16 flag, sint32 type, bool reflection, sint16 reflectOffset, const MATRIXF *model, const MATRIXF *camera, const HdViewPose *view, float width, float height, const float (*menu)[4] = 0, float menuX = 256, float menuY = 120)
{
    sint32 styleIndex = (type & 127) + (menu != 0 ? 1 : 11);
    if (styleIndex >= 34)
        return;
    const HdHaloStyle *style = haloTables.styles + styleIndex;
    VEC3F point = {(float)source[0].x, reflection ? (float)(sint16)(-reflectOffset - source[0].y) : (float)source[0].y, (float)source[0].z};
    VEC3F center = hd_halo_transform(&point, model, camera, view, false, menu);
    if (center.z < 128 || center.x > center.z || -center.x > center.z)
        return;
    float projection = view->projection / center.z;
    HdLensPoint screen = {center.x * projection, center.y * projection};
    const COORD16 *edge = source + (flag < 0 ? 1 : 2);
    point.x = (float)edge->x;
    point.y = (float)edge->y;
    point.z = (float)edge->z;
    VEC3F projectedEdge = hd_halo_transform(&point, model, camera, view, false, menu);
    float angleOuter = 0, outerSize = 0;
    if ((style->flags & 4) != 0 && projectedEdge.z > 0)
    {
        float x = projectedEdge.x * view->projection / projectedEdge.z - screen.x;
        float y = projectedEdge.y * view->projection / projectedEdge.z - screen.y;
        angleOuter = atan2f(y, x);
        outerSize = sqrtf(x * x + y * y) * center.z;
    }
    COORD16 a = {(sint16)(source[1].x - source[0].x), (sint16)(source[1].y - source[0].y), (sint16)(source[1].z - source[0].z)};
    COORD16 b = {(sint16)(source[2].x - source[0].x), (sint16)(source[2].y - source[0].y), (sint16)(source[2].z - source[0].z)};
    const COORD16 *scaleEdge = flag < 0 ? &b : &a;
    uint32 squares = scaleEdge->x * scaleEdge->x + (uint32)(scaleEdge->y * scaleEdge->y) + (uint32)(scaleEdge->z * scaleEdge->z);
    float base = (float)((uint32)((sint32)squares >> 1) << (style->scale & 31));
    float radius = base;
    if ((style->flags & 1) != 0)
    {
        VEC3F normal = {(float)(sint16)(b.y * a.z - b.z * a.y), (float)(sint16)(b.z * a.x - b.x * a.z), (float)(sint16)(b.x * a.y - b.y * a.x)};
        normal = hd_halo_transform(&normal, model, camera, view, true, menu);
        VECTORF normalized = {0};
        normalized.vx = normal.x;
        normalized.vy = normal.y;
        normalized.vz = normal.z;
        NormalizeVectorf(&normalized, &normalized);
        float intensity = normalized.vz * 4096;
        if ((style->flags & 2) != 0)
            intensity = (intensity - 3891) * 20;
        intensity -= 600;
        if (intensity < 0)
            intensity = 0;
        radius *= intensity / 2048;
    }
    CVECTOR halo = style->halo, beam = style->beam;
    if ((type & 128) != 0)
    {
        halo.r >>= 1;
        halo.g >>= 1;
        halo.b >>= 1;
        beam.r >>= 1;
        beam.g >>= 1;
        beam.b >>= 1;
    }
    if (reflection)
    {
        halo.r >>= 1;
        halo.g >>= 1;
        halo.b >>= 1;
    }
    if (menu != 0)
    {
        screen.x += menuX - width * 0.5f;
        screen.y += menuY - height * 0.5f;
    }
    float spin = (style->flags & 8) != 0 ? (screen.x + width * 0.5f + screen.y + height * 0.5f) * (6.28318530718f / 1024) : 0;
    float sx = reflection ? -(base + outerSize / 128) / 4096 : (radius + outerSize / 32) / 4096;
    float sy = reflection ? sx : radius / 4096;
    sint32 shape = reflection ? 3 : center.z < 3200 ? 0 : center.z < 7040 ? 1 : 2;
    hd_halo_shape(vertices, used, screen, halo, beam, sx, sy, reflection ? 0 : spin, reflection ? 0 : angleOuter, projection, shape, menu != 0 ? center.z : center.z / 4, width, height);
    if (menu == 0)
    {
        if (reflection)
            ++hd_car_halo_reflections;
        else
            ++hd_car_halo_count;
    }
}

void hd_frontend_capture_halo_tables(void)
{
    Flare::CaptureHaloSource(&haloTables);
}

uint32 hd_frontend_halo_geometry(VID_VERTEX2D *vertices, const COORD16 *points, sint16 flag, sint32 type, const float matrix[3][4], bool mirror, float x, float y, float depthBias)
{
    VEC3F cameraPoints[3];
    for (sint32 i = 0; i < 3; ++i)
    {
        VEC3F point = {(float)points[i].x, (float)points[i].y, (float)points[i].z};
        cameraPoints[i] = hd_halo_transform(&point, 0, 0, 0, false, matrix);
        if (cameraPoints[i].z <= 0)
            return 0;
    }
    float x1 = cameraPoints[1].x / cameraPoints[1].z - cameraPoints[0].x / cameraPoints[0].z;
    float y1 = cameraPoints[1].y / cameraPoints[1].z - cameraPoints[0].y / cameraPoints[0].z;
    float x2 = cameraPoints[2].x / cameraPoints[2].z - cameraPoints[0].x / cameraPoints[0].z;
    float y2 = cameraPoints[2].y / cameraPoints[2].z - cameraPoints[0].y / cameraPoints[0].z;
    float area = x1 * y2 - x2 * y1;
    if ((mirror ? -area : area) <= 0)
        return 0;
    HdViewPose view = {};
    view.projection = 512;
    uint32 used = 0;
    hd_car_halo_geometry(vertices, &used, points, flag, type, false, 0, 0, 0, &view, 512, 240, matrix, x, y);
    for (uint32 i = 0; depthBias != 0 && i < used; ++i)
    {
        float depth = vertices[i].pos[3];
        float biasedDepth = depth + depthBias;
        if (biasedDepth < 1)
            biasedDepth = 1;
        vertices[i].pos[2] = (65536.0f / 65534 - (131070.0f / 65534) / biasedDepth) * depth;
    }
    return used;
}

static bool hd_track_halo_resources(void)
{
    bool active = trackHaloStates[0].count != 0 || trackHaloStates[1].count != 0;
    if (!active)
        return true;
    if (!hd_sprite_materials())
        return false;
    if (trackHaloMaterial == 0)
    {
        trackHaloMaterial = vid_mat_create();
        if (trackHaloMaterial == 0)
            return false;
        VID_MATERIAL_DATA *material = vid_mat_data(trackHaloMaterial);
        material->shader = weatherShader;
        material->texture = worldVram;
        material->blend = VID_BLEND_ADD;
        material->depth_test = 1;
        material->depth_write = 0;
        material->depth_bias_slope = -2;
        material->depth_bias_units = -2;
        vid_mat_commit(trackHaloMaterial);
    }
    for (sint32 player = 0; player < 2; ++player)
    {
        uint32 count = trackHaloStates[player].count;
        if (count > 0xffffffff / 72)
            return false;
        if (trackHaloCapacities[player] < count * 72)
        {
            vid_mesh_free(trackHaloMeshes[player]);
            trackHaloMeshes[player] = vid_mesh_create_2d(count * 72);
            trackHaloCapacities[player] = trackHaloMeshes[player] != 0 ? count * 72 : 0;
            if (trackHaloMeshes[player] == 0)
                return false;
        }
        if (trackHaloCommandCapacities[player] < count)
        {
            HdTrackHaloCommand *commands = (HdTrackHaloCommand *)realloc(trackHaloCommands[player], count * sizeof(*commands));
            if (commands == 0)
                return false;
            trackHaloCommands[player] = commands;
            trackHaloCommandCapacities[player] = count;
        }
    }
    return true;
}

static VEC3F hd_track_halo_transform(VEC3F position, const MATRIXF *camera, const HdViewPose *view)
{
    const float delta[3] = {position.x - camera->t[0], position.y - camera->t[1], position.z - camera->t[2]};
    float local[3] = {0, 0, 0};
    for (sint32 row = 0; row < 3; ++row)
        for (sint32 axis = 0; axis < 3; ++axis)
            local[row] += camera->m[axis][row] * delta[axis];
    VEC3F result = {local[0] * (view->mirror ? -1 : 1), -local[1] * (56320.0f / 65536), local[2]};
    return result;
}

static VEC3F hd_track_halo_interpolate(VEC3F previousPosition, const coorddef *position, float phase)
{
    VEC3F currentPosition = hd_track_halo_position(position);
    VEC3F result = {previousPosition.x + (currentPosition.x - previousPosition.x) * phase, previousPosition.y + (currentPosition.y - previousPosition.y) * phase, previousPosition.z + (currentPosition.z - previousPosition.z) * phase};
    return result;
}

static void hd_track_halo_plane(VID_VERTEX2D *vertices, uint32 first, uint32 count, const VEC3F *position, const MATRIXF *camera, const HdViewPose *view, float width, float height)
{
    float dx = position->x - camera->t[0], dz = position->z - camera->t[2];
    float length = sqrtf(dx * dx + dz * dz);
    if (length < 0.0001f || view->projection <= 0)
        return;
    float normal[3];
    for (sint32 axis = 0; axis < 3; ++axis)
        normal[axis] = (camera->m[0][axis] * dx + camera->m[2][axis] * dz) / length;
    VEC3F center = hd_track_halo_transform(*position, camera, view);
    center.x *= view->mirror ? -1 : 1;
    center.y /= -(56320.0f / 65536);
    float distance = normal[0] * center.x + normal[1] * center.y + normal[2] * center.z;
    for (uint32 i = first; i < count; ++i)
    {
        VID_VERTEX2D *vertex = vertices + i;
        float w = vertex->pos[3];
        if (w <= 0)
            continue;
        float x = vertex->pos[0] / w * width / (2 * view->projection) * (view->mirror ? -1 : 1);
        float y = vertex->pos[1] / w * height / (2 * view->projection * (56320.0f / 65536));
        float denominator = normal[0] * x + normal[1] * y + normal[2];
        if (denominator <= 0.0001f)
            continue;
        float depth = distance / denominator;
        if (depth <= 0)
            continue;
        vertex->pos[0] *= depth / w;
        vertex->pos[1] *= depth / w;
        vertex->pos[2] = depth * (65536.0f / 65534) - 131070.0f / 65534;
        vertex->pos[3] = depth;
    }
}

static sint32 hd_track_halo_geometry(VID_VERTEX2D *vertices, uint32 *used, const HdTrackHalo *halo, float phase, const MATRIXF *camera, const HdViewPose *view, float width, float height)
{
    const PreparedTrackHalo *source = &halo->source;
    VEC3F center = hd_track_halo_transform(hd_track_halo_interpolate(halo->previousStart, &source->start, phase), camera, view);
    float horizontal = 1 + (width > 320 ? width - 320 : 0) * 0.5f / view->projection;
    if (center.z < 128 || fabsf(center.x) > center.z * horizontal)
        return -1;
    sint32 bucket = (sint32)floorf(center.z / 8);
    if (bucket > view->orderTableSize - 3)
        return -1;
    float projection = view->projection / center.z;
    HdLensPoint screen = {center.x * projection, center.y * projection};
    float radius = halo->previousScale + (source->scale - halo->previousScale) * phase;
    float outer = 0, angleOuter = 0, shapeDepth = center.z;
    if ((source->flags & 5) != 0 && source->hasEnd)
    {
        VEC3F end = hd_track_halo_transform(hd_track_halo_interpolate(halo->previousEnd, &source->end, phase), camera, view);
        if ((source->flags & 4) != 0)
        {
            if (end.z <= 0)
                return -1;
            float dx = end.x * view->projection / end.z - screen.x;
            float dy = end.y * view->projection / end.z - screen.y;
            outer = sqrtf(dx * dx + dy * dy) * center.z / 16;
            angleOuter = atan2f(dy, dx);
        }
        else
        {
            VECTORF direction = {0};
            direction.vx = center.x - end.x;
            direction.vy = center.y - end.y;
            direction.vz = center.z - end.z;
            NormalizeVectorf(&direction, &direction);
            float intensity = direction.vz * 4096;
            if ((source->flags & 2) != 0)
                intensity = (intensity - 3891) * 20;
            intensity -= 600;
            if (intensity <= 0)
                return -1;
            radius *= intensity / 2048;
            if (source->type == 30)
                shapeDepth /= 4;
        }
    }
    float spin = (source->flags & 8) != 0 ? (screen.x + 160 + screen.y + height * 0.5f) * (6.28318530718f / 1024) : 0;
    sint32 shape = shapeDepth < 3200 ? 0 : shapeDepth < 7040 ? 1 : 2;
    uint32 before = *used;
    hd_halo_shape(vertices, used, screen, source->colors[0], source->colors[1], (radius + outer) / 4096, radius / 4096, spin, angleOuter, projection, shape, center.z, width, height);
    if (!source->objectHalo)
    {
        VEC3F position = hd_track_halo_interpolate(halo->previousStart, &source->start, phase);
        hd_track_halo_plane(vertices, before, *used, &position, camera, view, width, height);
    }
    if (*used != before)
        ++hd_track_halo_count;
    return bucket;
}

static int hd_track_halo_compare(const void *left, const void *right)
{
    const HdTrackHaloCommand *a = (const HdTrackHaloCommand *)left;
    const HdTrackHaloCommand *b = (const HdTrackHaloCommand *)right;
    if (a->bucket != b->bucket)
        return a->bucket > b->bucket ? -1 : 1;
    return a->order > b->order ? -1 : a->order < b->order ? 1 : 0;
}

static bool hd_track_halo_order(const PreparedTrackHalo *source, const HdViewPose *view, HdOrderKey *key)
{
    if (!source->objectHalo)
        return true;
    for (uint32 index = 0; index < worldObjectCount; ++index)
    {
        const HdObjectEntry *entry = worldObjects + index;
        if (entry->source != source->owner)
            continue;
        key->family = HD_ORDER_OBJECTS;
        key->group = entry->kind < 2 ? 32 + hd_world_group(view, entry->chunk) * 2 + entry->kind : 102 + entry->kind;
        key->object = entry->ordinal;
        key->piece = entry->pieceCount + 1;
        return true;
    }
    return false;
}

static void hd_track_halos_view(sint32 player, float phase, const MATRIXF *camera, const HdViewPose *view, VID_RECT viewport, float logicalHeight)
{
    if (hd_track_halos_enabled == 0 || trackHaloMeshes[player] == 0)
        return;
    VID_VERTEX2D *vertices = vid_mesh_data_2d(trackHaloMeshes[player]);
    HdTrackHaloCommand *commands = trackHaloCommands[player];
    uint32 used = 0, count = 0;
    for (uint32 i = 0; i < trackHaloStates[player].count; ++i)
    {
        uint32 first = used;
        sint32 bucket = hd_track_halo_geometry(vertices, &used, trackHaloStates[player].entries + i, phase, camera, view, logicalHeight * viewport.w / viewport.h, logicalHeight);
        if (bucket >= 0 && used != first)
        {
            commands[count].first = first;
            commands[count].count = used - first;
            commands[count].bucket = bucket;
            commands[count].order = i;
            ++count;
        }
    }
    if (used != 0)
    {
        vid_mesh_commit(trackHaloMeshes[player]);
        qsortPSX(commands, count, sizeof(*commands), hd_track_halo_compare);
        for (uint32 i = 0; i < count; ++i)
        {
            const HdTrackHaloCommand *draw = commands + i;
            HdOrderKey order = {draw->bucket, HD_ORDER_TRACK_EFFECTS, 0, 0, 0, draw->order, 0};
            if (!hd_track_halo_order(&trackHaloStates[player].entries[draw->order].source, view, &order))
            {
                worldSubmissionValid = false;
                break;
            }
            hd_order_draw(&order, trackHaloMaterial, trackHaloMeshes[player], 0, draw->first, draw->count);
        }
    }
    hd_track_halo_vertices += used;
}

static void hd_car_halos_view(sint32 player, float phase, const MATRIXF *camera, const HdViewPose *view, VID_RECT viewport, float logicalHeight, const HdCarOrder orders[9])
{
    if (hd_car_halos_enabled == 0 || carHaloMeshes[player] == 0)
        return;
    uint32 used = 0;
    VID_VERTEX2D *vertices = vid_mesh_data_2d(carHaloMeshes[player]);
    for (sint32 slot = 0; slot < 9; ++slot)
    {
        const HdCarModel *car = carModels + slot;
        const HdCarOrder *order = orders + slot;
        sint32 detail = order->detail;
        if (detail < 0)
            continue;
        for (sint32 index = 0; index < 57; ++index)
        {
            const HdCarPart *part = car->parts + index;
            const HdCarPartPose *pose = part->poses + detail;
            const Transformer::zObj *object = part->object;
            if (object == 0 || pose->visible < 17 || pose->visible >= 23)
                continue;
            MATRIXF model;
            hd_mix_pose(&pose->previous, &pose->current, poseRenderPhase, &model, 0);
            if (pose->mirrored)
                for (sint32 row = 0; row < 3; ++row)
                    model.m[row][0] = -model.m[row][0];
            MATRIXF orderModel = model;
            for (sint32 row = 0; row < 3; ++row)
                for (sint32 column = 0; column < 3; ++column)
                    orderModel.m[row][column] *= 0.25f;
            for (sint32 face = 0; face < object->numFacet; ++face)
            {
                const Transformer::zFacet *facet = object->facet + face;
                uint8 ids[3] = {facet->vertexId0, facet->vertexId1, facet->vertexId2};
                if (ids[0] >= object->numVertex || ids[1] >= object->numVertex || ids[2] >= object->numVertex)
                    continue;
                COORD16 points[3] = {object->vertex[ids[0]], object->vertex[ids[1]], object->vertex[ids[2]]};
                VEC3F local[3];
                bool behind = false;
                for (sint32 i = 0; i < 3; ++i)
                {
                    VEC3F point = {(float)points[i].x, (float)points[i].y, (float)points[i].z};
                    local[i] = hd_car_halo_transform(&point, &model, camera, view, false);
                    behind = behind || local[i].z <= 0;
                }
                if (behind)
                    continue;
                float x1 = local[1].x / local[1].z - local[0].x / local[0].z, y1 = local[1].y / local[1].z - local[0].y / local[0].z;
                float x2 = local[2].x / local[2].z - local[0].x / local[0].z, y2 = local[2].y / local[2].z - local[0].y / local[0].z;
                float winding = (x1 * y2 - y1 * x2) * ((view->mirror != 0) != pose->mirrored ? -1 : 1);
                if (winding <= 0)
                    continue;
                sint32 type = hd_car_halo_type(car, pose->visible, facet->flag);
                if (type < 0)
                    continue;
                sint32 bucket = hd_car_facet_bucket(object, facet, &orderModel, camera, order->bias + pose->orderOffset);
                if (bucket < 0 || bucket >= order->size)
                    continue;
                uint32 first = used;
                hd_car_halo_geometry(vertices, &used, points, facet->flag, type, false, car->environment.reflectOffset, &model, camera, view, logicalHeight * viewport.w / viewport.h, logicalHeight);
                uint32 mainCount = used - first;
                HdOrderKey key = {order->bucket, HD_ORDER_CARS, (uint32)order->group, (uint32)bucket, (uint32)(slot * 57 + index), (uint32)(object->numFacet - 1 - face), 1};
                if (mainCount != 0)
                    hd_order_draw(&key, carHaloMaterial, carHaloMeshes[player], 0, first, mainCount);
                sint32 reflect = car->haloReflection;
                if (hd_car_halo_reflections_enabled != 0 && (reflect > 0 || (reflect == -1 && type == 5) || (reflect == -2 && type != 5)))
                {
                    first = used;
                    hd_car_halo_geometry(vertices, &used, points, facet->flag, type, true, car->environment.reflectOffset, &model, camera, view, logicalHeight * viewport.w / viewport.h, logicalHeight);
                    key.pass = 0;
                    if (used != first)
                        hd_order_draw(&key, carHaloMaterial, carHaloMeshes[player], 0, first, used - first);
                }
            }
        }
    }
    if (used != 0)
    {
        vid_mesh_commit(carHaloMeshes[player]);
    }
    hd_car_halo_vertices += used;
}

static float hd_weather_phase(const HdWeatherState *state, float phase)
{
    if (!(phase >= 0))
        phase = 0;
    if (phase > 1)
        phase = 1;
    uint32 age = (uint32)state->source.gameTick - (uint32)state->source.processTick;
    uint32 span = state->processSpan != 0 ? state->processSpan : 2;
    return age >= span ? 1.0f : (age + phase) / span;
}

static VEC3F hd_weather_tail(const HdWeatherParticle *particle, const VEC3F *position)
{
    VEC3F tail;
    for (sint32 axis = 0; axis < 3; ++axis)
        tail.v[axis] = position->v[axis] - (particle->current.v[axis] - particle->previous.v[axis]);
    return tail;
}

static float hd_weather_splat_age(sint32 age, float phase)
{
    sint32 previousAge = age > 0 ? age - 1 : 0;
    return previousAge + (age - previousAge) * phase;
}

static HdSkyPoint hd_weather_project(const VEC3F *position, const MATRIXF *camera, const HdViewPose *view, float logicalWidth, float logicalHeight)
{
    float relative[3] = {position->x - camera->t[0], position->y - camera->t[1], position->z - camera->t[2]};
    float local[3] = {0};
    for (sint32 row = 0; row < 3; ++row)
        for (sint32 axis = 0; axis < 3; ++axis)
            local[row] += camera->m[axis][row] * relative[axis];
    HdSkyPoint point;
    point.x = local[0] * 2 * view->projection / logicalWidth * (view->mirror ? -1 : 1);
    point.y = local[1] * 2 * view->projection / logicalHeight;
    point.z = 0;
    point.w = local[2];
    return point;
}

struct HdWeatherCommand
{
    uint32 first;
    sint32 blend;
};

static void hd_weather_view(sint32 player, float phase, const MATRIXF *camera, const HdViewPose *view, VID_RECT viewport, float logicalHeight)
{
    const HdWeatherState *state = weatherStates + player;
    if (!state->valid || weatherMeshes[player] == 0 || hd_weather_enabled == 0)
        return;
    if (!(phase >= 0))
        phase = 0;
    if (phase > 1)
        phase = 1;
    float particlePhase = weatherReset[player] || weatherSnapToCurrent[player] ? 1.0f : hd_weather_phase(state, phase);
    float logicalWidth = logicalHeight * viewport.w / viewport.h;
    VID_VERTEX2D *vertices = vid_mesh_data_2d(weatherMeshes[player]);
    HdWeatherCommand commands[HD_WEATHER_PARTICLES + HD_WEATHER_SPLATS];
    uint32 commandCount = 0, used = 0;
    for (sint32 slot = state->source.count - 1; slot >= 0; --slot)
    {
        const HdWeatherParticle *particle = state->particles + slot;
        VEC3F position;
        position.x = particle->previous.x + (particle->current.x - particle->previous.x) * particlePhase;
        position.y = particle->previous.y + (particle->current.y - particle->previous.y) * particlePhase;
        position.z = particle->previous.z + (particle->current.z - particle->previous.z) * particlePhase;
        HdSkyPoint point = hd_weather_project(&position, camera, view, logicalWidth, logicalHeight);
        if (point.w <= 1)
            continue;
        HdSkyPoint points[4] = {point, point, point, point};
        CVECTOR colors[4];
        memset(colors, 0, sizeof(colors));
        const Draw::tPixMap *pixmap = 0;
        sint32 blend = -1;
        if (state->source.type == Weather::kSnow)
        {
            points[1].x += 20 * view->projection / logicalWidth;
            points[2].y -= 20 * view->projection / logicalHeight;
            points[3].x = points[1].x;
            points[3].y = points[2].y;
            for (sint32 corner = 0; corner < 4; ++corner)
                colors[corner].r = colors[corner].g = colors[corner].b = 32;
            pixmap = state->source.pixmaps + state->source.snowMap;
            blend = (pixmap->tpage >> 5) & 3;
            ++hd_weather_snow_quads;
        }
        else
        {
            float endX = point.x / point.w, endY = point.y / point.w;
            float oldX = endX, oldY = endY;
            if (particle->trail)
            {
                VEC3F tail = hd_weather_tail(particle, &position);
                HdSkyPoint previous = hd_weather_project(&tail, camera, view, logicalWidth, logicalHeight);
                if (previous.w > 1)
                {
                    oldX = previous.x / previous.w;
                    oldY = previous.y / previous.w;
                }
            }
            float startX = (oldX + endX * 3) * 0.25f;
            float startY = (oldY + endY) * 0.5f;
            float dx = (endX - startX) * logicalWidth;
            float dy = (endY - startY) * logicalHeight;
            float length = sqrtf(dx * dx + dy * dy);
            if (length < 0.0001f)
                continue;
            float x = -dy / (length * logicalWidth), y = dx / (length * logicalHeight);
            points[0].x = startX - x;
            points[0].y = startY - y;
            points[1].x = startX + x;
            points[1].y = startY + y;
            points[2].x = endX - x;
            points[2].y = endY - y;
            points[3].x = endX + x;
            points[3].y = endY + y;
            for (sint32 corner = 0; corner < 4; ++corner)
                points[corner].w = 1;
            colors[2].r = colors[3].r = 32;
            colors[2].g = colors[3].g = 32;
            colors[2].b = colors[3].b = 64;
            ++hd_weather_rain_lines;
        }
        commands[commandCount].first = used;
        commands[commandCount++].blend = blend;
        hd_sky_quad(vertices, &used, points, colors, pixmap);
    }
    for (sint32 slot = state->source.splatCount - 1; slot >= 0; --slot)
    {
        if ((state->source.splatMask & ((uint32)1 << slot)) == 0)
            continue;
        const HdWeatherSplat *splat = state->source.splats + slot;
        sint32 age = state->source.gameTick - splat->startTick;
        if (age < 0 || age > 32)
            continue;
        sint32 previousAge = age > 0 ? age - 1 : 0;
        float renderAge = hd_weather_splat_age(age, phase);
        float expand = renderAge * 0.125f;
        uint8 color = (uint8)(128 - previousAge * 4 - (age - previousAge) * 4 * phase);
        float size = (splat->position.vx & 1) != 0 ? 12.0f : 18.0f;
        float x = splat->position.vx * logicalWidth / 320;
        float left = x - expand, right = x + size + expand;
        float top = (float)splat->position.vy, bottom = top + size + expand * 3;
        HdSkyPoint points[4] = {{2 * left / logicalWidth - 1, 1 - 2 * top / logicalHeight, 0, 1}, {2 * right / logicalWidth - 1, 1 - 2 * top / logicalHeight, 0, 1}, {2 * left / logicalWidth - 1, 1 - 2 * bottom / logicalHeight, 0, 1}, {2 * right / logicalWidth - 1, 1 - 2 * bottom / logicalHeight, 0, 1}};
        CVECTOR value = {color, color, color, 0};
        CVECTOR colors[4] = {value, value, value, value};
        const Draw::tPixMap *pixmap = state->source.pixmaps + 2;
        commands[commandCount].first = used;
        commands[commandCount++].blend = (pixmap->tpage >> 5) & 3;
        hd_sky_quad(vertices, &used, points, colors, pixmap);
        ++hd_weather_splat_quads;
    }
    if (used == 0)
        return;
    vid_mesh_commit(weatherMeshes[player]);
    for (uint32 command = 0; command < commandCount; ++command)
    {
        const HdWeatherCommand *draw = commands + command;
        if (draw->blend < 0)
        {
            hd_effect_pass(weatherMaterials[2]);
            hd_submit(weatherMeshes[player], 0, draw->first, 6);
        }
        else
            for (sint32 pass = 0; pass < 2; ++pass)
            {
                hd_effect_pass(weatherMaterials[pass == 0 ? 0 : draw->blend + 1]);
                hd_submit(weatherMeshes[player], 0, draw->first, 6);
            }
    }
}

static bool hd_skid_resources(void)
{
    if (skidStates[0].current.count == 0 && skidStates[1].current.count == 0)
        return true;
    if (weatherShader == 0)
        weatherShader = vid_shader_create(effectVertex, weatherFragment);
    if (weatherShader == 0)
        return false;
    for (sint32 pass = 0; pass < 5; ++pass)
        if (skidMaterials[pass] == 0)
        {
            skidMaterials[pass] = vid_mat_create();
            if (skidMaterials[pass] == 0)
                return false;
            VID_MATERIAL_DATA *material = vid_mat_data(skidMaterials[pass]);
            material->texture = worldVram;
            material->shader = weatherShader;
            material->params[0] = pass == 0 ? 1.0f : 2.0f;
            material->params[1] = pass == 1 ? 0.5f : pass == 4 ? 0.25f : 1.0f;
            material->blend = pass == 0 ? VID_BLEND_NONE : pass == 1 ? VID_BLEND_ALPHA : pass == 3 ? VID_BLEND_SUBTRACT : VID_BLEND_ADD;
            material->depth_test = 1;
            material->depth_bias_slope = -4;
            material->depth_bias_units = -4;
            material->depth_write = 0;
            material->filter = VID_FILTER_NEAREST;
            vid_mat_commit(skidMaterials[pass]);
        }
    for (sint32 player = 0; player < 2; ++player)
        if (skidMeshes[player] == 0)
        {
            skidMeshes[player] = vid_mesh_create_2d(HD_SKID_QUADS * 6);
            if (skidMeshes[player] == 0)
                return false;
        }
    return true;
}

static bool hd_skid_quad(const HdSkidState *state, uint32 index, float phase, const MATRIXF *camera, const HdViewPose *view, float logicalWidth, float logicalHeight, HdSkyPoint *points, CVECTOR *colors, Draw::tPixMap *pixmap, sint32 *bucket)
{
    const HdSkidQuad *quad = state->current.quads + index, *before = quad;
    for (uint32 i = 0; i < state->previous.count; ++i)
    {
        const HdSkidQuad *candidate = state->previous.quads + i;
        if (candidate->owner == quad->owner && candidate->type == quad->type && memcmp(&candidate->center, &quad->center, sizeof(quad->center)) == 0)
        {
            before = candidate;
            break;
        }
    }
    if (!(phase >= 0))
        phase = 0;
    if (phase > 1)
        phase = 1;
    float depth = 0;
    for (sint32 corner = 0; corner < 4; ++corner)
    {
        VEC3F position;
        for (sint32 axis = 0; axis < 3; ++axis)
            position.v[axis] = before->points[corner].v[axis] + (quad->points[corner].v[axis] - before->points[corner].v[axis]) * phase;
        VEC3F local = hd_souffle_camera(&position, camera);
        if (local.z <= 0)
            return false;
        points[corner].x = local.x * 2 * view->projection / logicalWidth * (view->mirror ? -1 : 1);
        points[corner].y = local.y * 2 * view->projection / logicalHeight;
        points[corner].z = local.z * (65536.0f / 65534) - 131070.0f / 65534;
        points[corner].w = local.z;
        colors[corner] = quad->colors[corner];
        depth += local.z;
    }
    sint32 raw = (sint32)floorf(depth / 32);
    *bucket = raw + 50;
    if (*bucket <= 0 || *bucket > view->orderTableSize - 3)
        return false;
    *pixmap = state->current.pixmaps[quad->type & 1];
    if (pixmap->clut == 65535)
        pixmap->clut = state->current.palettes[quad->type & 1][0];
    return true;
}

static void hd_skids_view(sint32 player, float phase, const MATRIXF *camera, const HdViewPose *view, VID_RECT viewport, float logicalHeight)
{
    const HdSkidState *state = skidStates + player;
    if (!state->valid || skidMeshes[player] == 0 || hd_skids_enabled == 0)
        return;

    struct Command
    {
        uint32 first;
        sint32 bucket;
        sint32 blend;
    } commands[HD_SKID_QUADS];

    uint32 count = 0, used = 0;
    VID_VERTEX2D *vertices = vid_mesh_data_2d(skidMeshes[player]);
    for (uint32 index = 0; index < state->current.count; ++index)
    {
        HdSkyPoint points[4];
        CVECTOR colors[4];
        Draw::tPixMap pixmap;
        sint32 bucket;
        if (!hd_skid_quad(state, index, phase, camera, view, logicalHeight * viewport.w / viewport.h, logicalHeight, points, colors, &pixmap, &bucket))
            continue;
        Command draw = {used, bucket, (pixmap.tpage >> 5) & 3};
        hd_sky_quad(vertices, &used, points, colors, &pixmap);
        uint32 position = count++;
        while (position > 0 && commands[position - 1].bucket <= bucket)
        {
            commands[position] = commands[position - 1];
            --position;
        }
        commands[position] = draw;
    }
    if (used == 0)
        return;
    vid_mesh_commit(skidMeshes[player]);
    for (uint32 index = 0; index < count; ++index)
        for (sint32 pass = 0; pass < 2; ++pass)
        {
            HdOrderKey order = {commands[index].bucket, HD_ORDER_SKIDS, 0, 0, 0, commands[index].first / 6, (uint32)pass};
            hd_order_draw(&order, skidMaterials[pass == 0 ? 0 : commands[index].blend + 1], skidMeshes[player], 0, commands[index].first, 6);
        }
    hd_skid_quads += count;
}

static bool hd_line_face(const Chunk *chunk, const Trk::Line *line, VID_VERTEX16 *vertices, uint32 *indices, uint32 face, HdLineFace *output)
{
    if (line->type >= 10 || chunk->vertexBuf == 0 || BWorldSm::slices == 0)
        return false;
    const CCOORD16 *source = (const CCOORD16 *)(chunk->vertexBuf + 1);
    const CCOORD16 *points[2];
    const Trk::NewSlice *slices[2];
    for (sint32 end = 0; end < 2; ++end)
    {
        sint32 slice = chunk->firstSimSliceInd + line[end].slice;
        if (line[end].firstPoint >= chunk->renderVertexCount || slice < 0 || slice >= gNumSlices)
            return false;
        points[end] = source + line[end].firstPoint;
        slices[end] = BWorldSm::slices + slice;
        if (points[end]->light < 0 || points[end]->light >= Chunk::numLight)
            return false;
    }
    output->center.x = (points[0]->x + points[1]->x) * 0.5f;
    output->center.y = (points[0]->y + points[1]->y) * 0.5f;
    output->center.z = (points[0]->z + points[1]->z) * 0.5f;
    output->first = face * 6;
    output->type = line->type;
    for (sint32 corner = 0; corner < 4; ++corner)
    {
        sint32 end = (corner & 1) == 0 ? 1 : 0;
        const CCOORD16 *point = points[end];
        const CVECTOR *color = Chunk::lightTable + point->light;
        VID_VERTEX16 *vertex = vertices + face * 4 + corner;
        memset(vertex, 0, sizeof(*vertex));
        vertex->pos[0] = point->x;
        vertex->pos[1] = point->y;
        vertex->pos[2] = point->z;
        vertex->pos[3] = 1;
        for (sint32 axis = 0; axis < 3; ++axis)
            vertex->n[axis] = (sint8)slices[end]->right[axis] >> 3;
        vertex->color[0] = color->r;
        vertex->color[1] = color->g;
        vertex->color[2] = color->b;
        vertex->color[3] = 255;
        vertex->tex[0] = line->type;
        vertex->tex[1] = (uint16)corner;
        vertex->tex[2] = (uint16)point->light;
        vertex->tex[3] = corner < 2 ? 1 : 0;
        memcpy(vertex->extra, &output->center, sizeof(output->center));
    }
    const uint32 topology[6] = {0, 1, 2, 2, 1, 3};
    for (sint32 index = 0; index < 6; ++index)
        indices[face * 6 + index] = face * 4 + topology[index];
    return true;
}

static bool hd_road_build(void)
{
    if (roadChunks != 0)
        return true;
    roadChunkCount = worldChunkCount;
    roadChunks = (HdLineChunk *)calloc(roadChunkCount, sizeof(*roadChunks));
    if (roadChunks == 0)
        return false;
    hd_road_line_total = 0;
    uint32 maximum = 0;
    for (sint32 index = 0; index < roadChunkCount; ++index)
    {
        const Chunk *chunk = Track::chunkList + index;
        if (chunk->lineBuf == 0 || chunk->lineBuf->m_num_elements < 2)
            continue;
        uint32 faces = chunk->lineBuf->m_num_elements - 1;
        if (faces > 0xffffffff / (4 * sizeof(VID_VERTEX16)))
            return false;
        HdLineChunk *line = roadChunks + index;
        line->geometry.line = true;
        line->geometry.quadTopology = true;
        line->geometry.center = worldChunks[index].center;
        line->geometry.mesh = vid_mesh_create_16(faces * 4);
        line->geometry.indices = vid_index_create(faces * 6);
        line->faces = (HdLineFace *)calloc(faces, sizeof(*line->faces));
        if (line->geometry.mesh == 0 || line->geometry.indices == 0 || line->faces == 0)
            return false;
        VID_VERTEX16 *vertices = vid_mesh_data_16(line->geometry.mesh);
        uint32 *indices = vid_index_data(line->geometry.indices);
        const Trk::Line *source = (const Trk::Line *)(chunk->lineBuf + 1);
        for (uint32 face = 0; face < faces; ++face)
        {
            if (source[face].type == 255)
                continue;
            if (!hd_line_face(chunk, source + face, vertices, indices, line->count, line->faces + line->count))
                return false;
            ++line->count;
        }
        line->geometry.count = line->count * 6;
        if (hd_road_line_total > 0xffffffff - line->count - 16)
            return false;
        hd_road_line_total += line->count;
        if (line->count > maximum)
            maximum = line->count;
        vid_mesh_commit(line->geometry.mesh);
        vid_mesh_static(line->geometry.mesh);
        vid_index_commit(line->geometry.indices, line->geometry.count);
        vid_index_static(line->geometry.indices);
    }
    if (maximum > (0xffffffff - 16) / 32)
        return false;
    roadDrawCapacity = maximum * 32 + 16;
    if (roadDrawCapacity > 0xffffffff / sizeof(HdLineDraw))
        return false;
    for (sint32 player = 0; player < 2; ++player)
    {
        roadDraws[player] = (HdLineDraw *)calloc(roadDrawCapacity, sizeof(*roadDraws[player]));
        if (roadDraws[player] == 0)
            return false;
    }
    return true;
}

static bool hd_spike_build(void)
{
    if (spikeGeometry.mesh != 0)
        return true;
    spikeGeometry.mesh = vid_mesh_create_16(16 * 4);
    spikeGeometry.indices = vid_index_create(16 * 6);
    if (spikeGeometry.mesh == 0 || spikeGeometry.indices == 0)
        return false;
    spikeGeometry.noNight = true;
    VID_VERTEX16 *vertices = vid_mesh_data_16(spikeGeometry.mesh);
    uint32 *indices = vid_index_data(spikeGeometry.indices);
    for (sint32 face = 0; face < 16; ++face)
    {
        sint32 strip = face & 7;
        const sint16 points[2][4][3] = {{{(sint16)(strip + 1), 1, 0}, {(sint16)(strip + 1), 0, -1}, {(sint16)strip, 1, 0}, {(sint16)strip, 0, -1}}, {{(sint16)strip, 1, 0}, {(sint16)strip, 0, 1}, {(sint16)(strip + 1), 1, 0}, {(sint16)(strip + 1), 0, 1}}};
        VEC3F center = {0};
        for (sint32 corner = 0; corner < 4; ++corner)
            for (sint32 axis = 0; axis < 3; ++axis)
                center.v[axis] += points[face >> 3][corner][axis] * 0.25f;
        for (sint32 corner = 0; corner < 4; ++corner)
        {
            VID_VERTEX16 *vertex = vertices + face * 4 + corner;
            memset(vertex, 0, sizeof(*vertex));
            memcpy(vertex->pos, points[face >> 3][corner], 3 * sizeof(sint16));
            vertex->pos[3] = 1;
            vertex->color[0] = vertex->color[1] = vertex->color[2] = 128;
            vertex->color[3] = 255;
            vertex->tex[0] = 10;
            vertex->tex[1] = (uint16)corner;
            memcpy(vertex->extra, &center, sizeof(center));
        }
        const uint32 topology[6] = {0, 1, 2, 2, 1, 3};
        for (sint32 index = 0; index < 6; ++index)
            indices[face * 6 + index] = face * 4 + topology[index];
    }
    spikeGeometry.count = 16 * 6;
    vid_mesh_commit(spikeGeometry.mesh);
    vid_mesh_static(spikeGeometry.mesh);
    vid_index_commit(spikeGeometry.indices, spikeGeometry.count);
    vid_index_static(spikeGeometry.indices);
    return true;
}

static bool hd_road_resources(void)
{
    if (!hd_road_build() || !hd_spike_build())
    {
        hd_road_release();
        return false;
    }
    if (roadMetadata == 0)
        roadMetadata = vid_tex_create_fmt(256, 1, VID_RGBA16UI);
    if (roadMetadata == 0)
        return false;
    uint16 *data = (uint16 *)vid_tex_data(roadMetadata);
    memset(data, 0, 256 * 4 * sizeof(*data));
    for (sint32 type = 0; type < 11; ++type)
    {
        const Draw::tPixMap *pmx = type < 10 ? roadCurrent.lines + type : &roadCurrent.spike;
        uint16 *entry = data + type * 24;
        entry[0] = pmx->u0 | (pmx->v0 << 8);
        entry[1] = pmx->u1 | (pmx->v1 << 8);
        entry[2] = pmx->u2 | (pmx->v2 << 8);
        entry[3] = pmx->u3 | (pmx->v3 << 8);
        entry[4] = pmx->clut;
        entry[5] = pmx->tpage;
        entry[6] = pmx->pad2;
        entry[7] = type < 10 ? 2 : (uint8)pmx->flag;
        HdTextureRegion region = {0};
        if ((hd_texture_filtering_enabled & HD_FILTER_WORLD) != 0)
            hd_texture_region(pmx, &region);
        memcpy(entry + 8, &region, sizeof(region));
    }
    vid_tex_commit(roadMetadata);
    for (sint32 orientation = 0; orientation < 2; ++orientation)
        for (sint32 pass = 0; pass < 6; ++pass)
            if (roadMaterials[orientation][pass] == 0)
            {
                roadMaterials[orientation][pass] = vid_mat_create();
                if (roadMaterials[orientation][pass] == 0)
                    return false;
                VID_MATERIAL_DATA *material = vid_mat_data(roadMaterials[orientation][pass]);
                material->shader = worldShader;
                material->texture = worldVram;
                material->aux[0] = roadMetadata;
                material->aux[1] = worldPalettes;
                material->aux[2] = nightTables;
                material->aux[3] = nightShape;
                material->aux[4] = nightColors;
                material->aux[5] = hd_lighting_depth();
                material->aux[6] = hd_lighting_data();
                material->params[0] = pass == 5 ? 0.0f : pass == 0 ? 1.0f : 2.0f;
                material->params[1] = pass == 1 ? 0.5f : pass == 4 ? 0.25f : 1.0f;
                material->blend = pass == 0 || pass == 5 ? VID_BLEND_NONE : pass == 1 ? VID_BLEND_ALPHA : pass == 3 ? VID_BLEND_SUBTRACT : VID_BLEND_ADD;
                material->depth_test = 1;
                material->depth_bias_slope = -4;
                material->depth_bias_units = -4;
                material->depth_write = pass == 5 ? 1 : 0;
                material->filter = VID_FILTER_NEAREST;
                material->cull = orientation + 1;
                vid_mat_commit(roadMaterials[orientation][pass]);
            }
    for (sint32 orientation = 0; orientation < 2; ++orientation)
        if (roadCompositeMaterials[orientation] == 0)
        {
            roadCompositeMaterials[orientation] = vid_mat_create();
            if (roadCompositeMaterials[orientation] == 0)
                return false;
            *vid_mat_data(roadCompositeMaterials[orientation]) = *vid_mat_read(roadMaterials[orientation][1]);
            vid_mat_data(roadCompositeMaterials[orientation])->params[7] = 1;
            vid_mat_commit(roadCompositeMaterials[orientation]);
        }
    return true;
}

static bool hd_road_chunk_visible(const HdViewPose *view, sint32 chunk, bool line)
{
    return !(line && view->world.skipRoadLines) && hd_chunk_visible(view, chunk, false, 0x800);
}

static void hd_spike_matrix(float phase, MATRIXF *model)
{
    if (!(phase >= 0))
        phase = 0;
    if (phase > 1)
        phase = 1;
    float width = roadPrevious.width + ((float)roadCurrent.width - roadPrevious.width) * phase;
    float offset = (roadPrevious.x + ((float)roadCurrent.x - roadPrevious.x) * phase) / 1024;
    VEC3F center;
    hd_position(&roadCurrent.center, &center);
    memset(model, 0, sizeof(*model));
    for (sint32 axis = 0; axis < 3; ++axis)
    {
        float right = roadCurrent.right[axis] / 128.0f;
        model->m[axis][0] = right * width / 8192;
        model->m[axis][2] = roadCurrent.forward[axis] * 0.5f;
        model->t[axis] = center.v[axis] + right * offset;
    }
    model->m[1][1] = 25;
}

static int hd_road_draw_compare(const void *a, const void *b)
{
    const HdLineDraw *left = (const HdLineDraw *)a, *right = (const HdLineDraw *)b;
    if (left->bucket != right->bucket)
        return left->bucket > right->bucket ? -1 : 1;
    return left->order > right->order ? -1 : left->order < right->order ? 1 : 0;
}

static void hd_road_view(sint32 player, float phase, const MATRIXF *camera, const HdViewPose *view, VID_RECT viewport, float logicalHeight)
{
    HdLineDraw *commands = roadDraws[player];
    if (commands == 0)
        return;
    uint32 count = 0;
    if (hd_road_lines_enabled != 0 && view->world.chunk >= 0 && view->world.chunk < worldChunkCount && Track::gInViewList != 0 && Track::gInViewCount != 0 && Track::gInViewCount[view->world.chunk] <= 36)
        for (sint32 index = 0; index < Track::gInViewCount[view->world.chunk]; ++index)
        {
            sint32 chunk = (uint16)((const sint16 *)Track::gInViewList)[view->world.chunk * 32 + index] & 1023;
            if (!hd_road_chunk_visible(view, chunk, true))
                continue;
            const HdLineChunk *line = roadChunks + chunk;
            for (uint32 face = 0; face < line->count; ++face)
            {
                const HdLineFace *source = line->faces + face;
                float depth = 0;
                for (sint32 axis = 0; axis < 3; ++axis)
                    depth += camera->m[axis][2] * (line->geometry.center.v[axis] + source->center.v[axis] - camera->t[axis]);
                sint32 bucket = (sint32)floorf(depth / 8) + 75;
                if (!roadCurrent.lineValid[source->type] || bucket <= 0 || bucket > view->orderTableSize - 3)
                    continue;
                HdLineDraw draw = {chunk, bucket, face, count};
                commands[count++] = draw;
                ++hd_road_line_quads;
            }
        }
    MATRIXF spike;
    if (hd_spike_enabled != 0 && roadCurrent.spikeActive && hd_road_chunk_visible(view, roadCurrent.chunk, false))
    {
        hd_spike_matrix(phase, &spike);
        for (uint32 face = 0; face < 16; ++face)
        {
            VEC3F center = {(face & 7) + 0.5f, 0.5f, face < 8 ? -0.5f : 0.5f};
            float depth = 0;
            for (sint32 axis = 0; axis < 3; ++axis)
            {
                float coordinate = spike.t[axis];
                for (sint32 column = 0; column < 3; ++column)
                    coordinate += spike.m[axis][column] * center.v[column];
                depth += camera->m[axis][2] * (coordinate - camera->t[axis]);
            }
            sint32 bucket = (sint32)floorf(depth / 8) + 35;
            if (bucket <= 0 || bucket > view->orderTableSize - 3)
                continue;
            HdLineDraw draw = {-1, bucket, face, count};
            commands[count++] = draw;
            ++hd_spike_quads;
        }
    }
    qsortPSX(commands, count, sizeof(*commands), hd_road_draw_compare);
    for (uint32 index = 0; index < count; ++index)
    {
        const HdLineDraw *draw = commands + index;
        MATRIXF model;
        HdWorldChunk mesh;
        sint32 type;
        sint32 rgb = -1;
        if (draw->chunk >= 0)
        {
            mesh = roadChunks[draw->chunk].geometry;
            IdentityMatrixf(&model);
            memcpy(model.t, &mesh.center, sizeof(mesh.center));
            type = roadChunks[draw->chunk].faces[draw->face].type;
        }
        else
        {
            mesh = spikeGeometry;
            model = spike;
            type = 10;
            rgb = roadCurrent.color.r | (roadCurrent.color.g << 8) | (roadCurrent.color.b << 16);
        }
        mesh.count = 6;
        const Draw::tPixMap *pmx = type < 10 ? roadCurrent.lines + type : &roadCurrent.spike;
        sint32 blend = (pmx->tpage >> 5) & 3;
        bool semi = type < 10 || (pmx->flag & 2) != 0;
        if (semi && blend == 0)
        {
            HdOrderKey order = {draw->bucket, draw->chunk >= 0 ? HD_ORDER_LINES : HD_ORDER_SPIKE, 0, 0, 0, draw->order, 0};
            hd_world_submit(&mesh, &model, camera, view, viewport, logicalHeight, -1, rgb, 0, -1, 1, 0, draw->face * 6, 0, 1, &order, roadCompositeMaterials[view->mirror != 0 ? 1 : 0]);
            ++hd_road_line_submits;
            continue;
        }
        for (sint32 pass = 0; pass < (semi ? 2 : 1); ++pass)
        {
            HdOrderKey order = {draw->bucket, draw->chunk >= 0 ? HD_ORDER_LINES : HD_ORDER_SPIKE, 0, 0, 0, draw->order, (uint32)pass};
            hd_world_submit(&mesh, &model, camera, view, viewport, logicalHeight, -1, rgb, 0, -1, 1, 0, draw->face * 6, 0, 1, &order, roadMaterials[view->mirror != 0 ? 1 : 0][!semi ? 5 : pass == 0 ? 0 : blend + 1]);
            ++hd_road_line_submits;
        }
    }
}

static sint32 hd_feedback_stamps(sint32 mode, float *offsets)
{
    sint32 count = 0;
    if ((mode & 1) == 0)
        return 0;
    if ((mode & 0x10) != 0)
        offsets[count++] = (mode & 4) != 0 ? 2.0f : 0.0f;
    if ((mode & 8) != 0)
        offsets[count++] = (mode & 4) != 0 ? 1.0f : 0.0f;
    return count;
}

static bool hd_feedback_supported(const HdFeedbackSource *source)
{
    if ((source->mode & 1) == 0 || (source->mode & 0x18) == 0)
        return true;
    return (source->mode & 2) != 0 && source->firstDepth == 0 && ((source->mode & 0x10) == 0 || source->secondDepth == 0);
}

static void hd_feedback_quad(VID_VERTEX *vertices, float offset, VID_RECT viewport, sint32 frameWidth, sint32 frameHeight, float logicalHeight)
{
    const uint8 order[6] = {0, 1, 2, 2, 1, 3};
    float logicalWidth = logicalHeight * viewport.w / viewport.h;
    float shift = offset * 2 / logicalWidth;
    float top = 1 - (float)viewport.y / frameHeight;
    float bottom = 1 - (float)(viewport.y + viewport.h) / frameHeight;
    const float x[4] = {-1 + shift, 1 + shift, -1 + shift, 1 + shift};
    const float y[4] = {1, 1, -1, -1};
    for (sint32 i = 0; i < 6; ++i)
    {
        VID_VERTEX *vertex = vertices + i;
        sint32 corner = order[i];
        memset(vertex, 0, sizeof(*vertex));
        vertex->pos[0] = x[corner];
        vertex->pos[1] = y[corner];
        vertex->pos[3] = 1;
        vertex->uv[0] = (float)(viewport.x + ((corner & 1) != 0 ? viewport.w : 0)) / frameWidth;
        vertex->uv[1] = corner < 2 ? top : bottom;
        vertex->color[0] = vertex->color[1] = vertex->color[2] = 1;
        vertex->color[3] = 0.5f;
    }
}

static bool hd_feedback_prepare(sint32 width, sint32 height, bool ui = false)
{
    float offsets[2];
    sint32 stamps = hd_feedback_stamps(feedbackSource.mode, offsets);
    hd_feedback_unsupported = !hd_feedback_supported(&feedbackSource);
    if (hd_feedback_unsupported != 0)
        return false;
    if (stamps == 0)
    {
        feedbackState.reset = true;
        return true;
    }
    if (feedbackState.width != width || feedbackState.height != height || feedbackState.views != current.viewCount || feedbackState.ui != ui)
    {
        feedbackState.reset = true;
        feedbackState.width = width;
        feedbackState.height = height;
        feedbackState.views = current.viewCount;
        feedbackState.ui = ui;
    }
    if (feedbackState.reset)
    {
        feedbackState.outputReady = false;
        feedbackState.historyReady = false;
        feedbackState.reset = false;
    }
    if (feedbackState.output == 0)
        feedbackState.output = vid_target_create_scene_ldr();
    if (feedbackState.history == 0)
        feedbackState.history = vid_target_create_scene_ldr();
    if (feedbackShader == 0)
        feedbackShader = vid_shader_create(0, "uniform sampler2D uAux0;void main(){vec3 rgb=texture(uTex,vUV).rgb;float stp=texture(uAux0,vUV).r;if(stp<0.5&&all(equal(rgb,vec3(0.0))))discard;outColor=vec4(rgb,stp>=0.5?0.5:1.0);}");
    if (feedbackState.output == 0 || feedbackState.history == 0 || feedbackShader == 0)
        return false;
    if (feedbackState.material == 0)
    {
        feedbackState.material = vid_mat_create();
        if (feedbackState.material == 0)
            return false;
        VID_MATERIAL_DATA *material = vid_mat_data(feedbackState.material);
        material->shader = feedbackShader;
        material->texture = vid_target_texture(feedbackState.history);
        material->aux[0] = vid_target_surface_texture(feedbackState.history);
        material->blend = VID_BLEND_ALPHA;
        material->filter = VID_FILTER_NEAREST;
        material->depth_test = 0;
        material->depth_write = 0;
        vid_mat_commit(feedbackState.material);
    }
    if (feedbackMaskShader == 0)
        feedbackMaskShader = vid_shader_create(0, "layout(location=1)out vec4 outSurface;void main(){outColor=vec4(0.0);outSurface=vec4(1.0,0.0,0.0,1.0);}");
    if (feedbackMaskShader == 0)
        return false;
    if (feedbackState.maskMaterial == 0)
    {
        feedbackState.maskMaterial = vid_mat_create();
        if (feedbackState.maskMaterial == 0)
            return false;
        VID_MATERIAL_DATA *material = vid_mat_data(feedbackState.maskMaterial);
        material->shader = feedbackMaskShader;
        material->blend = VID_BLEND_ALPHA;
        material->surface_write = 1;
        material->depth_test = material->depth_write = 0;
        vid_mat_commit(feedbackState.maskMaterial);
    }
    if (feedbackState.maskMesh == 0)
        feedbackState.maskMesh = vid_mesh_create(6);
    if (feedbackState.maskMesh == 0)
        return false;
    VID_RECT full = {0, 0, width, height};
    hd_feedback_quad(vid_mesh_data(feedbackState.maskMesh), 0, full, width, height, 240);
    vid_mesh_commit(feedbackState.maskMesh);
    if (feedbackState.outputReady && feedbackState.completedGeneration != feedbackSource.generation && (!feedbackState.historyReady || feedbackState.capturedGeneration != feedbackSource.generation))
    {
        if (!vid_target_copy(feedbackState.history, feedbackState.output))
            return false;
        feedbackState.historyReady = true;
        feedbackState.capturedGeneration = feedbackSource.generation;
        for (sint32 player = 0; player < current.viewCount; ++player)
            feedbackState.historyCameraGeneration[player] = feedbackState.completedCameraGeneration[player];
        ++hd_feedback_copies;
    }
    for (sint32 player = 0; player < current.viewCount; ++player)
    {
        if (feedbackState.meshes[player] == 0)
            feedbackState.meshes[player] = vid_mesh_create(12);
        if (feedbackState.meshes[player] == 0)
            return false;
        VID_RECT viewport = {0, height * player / current.viewCount, width, height * (player + 1) / current.viewCount - height * player / current.viewCount};
        VID_VERTEX *vertices = vid_mesh_data(feedbackState.meshes[player]);
        for (sint32 pass = 0; pass < stamps; ++pass)
            hd_feedback_quad(vertices + pass * 6, offsets[pass], viewport, width, height, current.viewCount == 2 ? 120.0f : 240.0f);
        vid_mesh_commit(feedbackState.meshes[player]);
    }
    return true;
}

static bool hd_feedback_active(void)
{
    return (feedbackSource.mode & 1) != 0 && (feedbackSource.mode & 0x18) != 0;
}

static void hd_feedback_complete(bool success)
{
    if (hd_feedback_active())
    {
        feedbackState.outputReady = success;
        if (success)
        {
            feedbackState.completedGeneration = feedbackSource.generation;
            for (sint32 player = 0; player < current.viewCount; ++player)
                feedbackState.completedCameraGeneration[player] = feedbackSource.cameraGeneration[player];
        }
        else
            feedbackState.historyReady = false;
    }
}

static void hd_feedback_view(sint32 player)
{
    if (hd_feedback_enabled == 0 || !feedbackState.historyReady || !hd_feedback_active() || feedbackState.historyCameraGeneration[player] != feedbackSource.cameraGeneration[player])
        return;
    float offsets[2];
    sint32 stamps = hd_feedback_stamps(feedbackSource.mode, offsets);
    vid_pass(feedbackState.material);
    hd_submit(feedbackState.meshes[player], 0, 0, stamps * 6);
    hd_feedback_draws += stamps;
}

void hd_track_halo_witness(sint32 player, sint32 type, sint32 depth)
{
    MATRIXF camera;
    HdViewPose view;
    if (player < 0 || player > 1 || type < 0 || type >= 34 || depth < 128 || !hd_view_pose(player, 1, &camera, &view))
        return;
    HdTrackHaloState *state = trackHaloStates + player;
    if (state->capacity == 0)
    {
        state->entries = (HdTrackHalo *)malloc(sizeof(*state->entries));
        if (state->entries == 0)
            return;
        state->capacity = 1;
    }
    state->count = 1;
    HdTrackHalo *halo = state->entries;
    memset(halo, 0, sizeof(*halo));
    const HdHaloStyle *style = haloTables.styles + type;
    halo->source.owner = state;
    halo->source.type = type;
    halo->source.flags = style->flags;
    halo->source.hasEnd = (style->flags & 5) != 0;
    halo->source.scale = style->scale;
    halo->source.colors[0] = style->halo;
    halo->source.colors[1] = style->beam;
    sint32 start[3], end[3];
    for (sint32 axis = 0; axis < 3; ++axis)
    {
        start[axis] = (sint32)((camera.t[axis] + camera.m[axis][2] * depth) * 1024);
        end[axis] = (sint32)((camera.t[axis] + camera.m[axis][2] * (depth - 128) + camera.m[axis][0] * ((style->flags & 4) != 0 ? 32 : 0)) * 1024);
    }
    halo->source.start.x = start[0];
    halo->source.start.y = start[1];
    halo->source.start.z = start[2];
    halo->source.end.x = end[0];
    halo->source.end.y = end[1];
    halo->source.end.z = end[2];
    halo->previousStart = hd_track_halo_position(&halo->source.start);
    halo->previousEnd = hd_track_halo_position(&halo->source.end);
    halo->previousScale = (float)halo->source.scale;
}

struct HdLightingScene
{
    MATRIXF camera;
    HdViewPose view;
    float phase;
};

static bool hd_lighting_depth_resources(void)
{
    if (carDepthShader == 0)
        carDepthShader = vid_shader_create(carVertex, "flat in uvec4 vCarTex;\n" VID_PSX_SAMPLING_GLSL HD_VRAM_WORD_GLSL "void main(){if((vCarTex.w&1u)==0u)discard;uvec2 pixel=uvec2(ivec2(floor(vUV)));uint word=vidPsxDecode(vCarTex.y,vCarTex.x,ivec2(pixel));if(vidPsxColor(word,vCarTex.z!=0u?1u:0u,false).a==0.0)discard;outColor=vec4(0.0);}");
    if (carDepthShader == 0)
        return false;
    if (carDepthMaterial == 0)
    {
        carDepthMaterial = vid_mat_create();
        if (carDepthMaterial == 0)
            return false;
        VID_MATERIAL_DATA *material = vid_mat_data(carDepthMaterial);
        *material = *vid_mat_read(carMaterial[0]);
        material->shader = carDepthShader;
        material->aux[5] = material->aux[6] = 0;
        material->cull = 0;
        material->blend = VID_BLEND_NONE;
        material->depth_test = material->depth_write = 1;
        material->depth_bias_slope = 1;
        material->depth_bias_units = 1;
        vid_mat_commit(carDepthMaterial);
    }
    return true;
}

static bool hd_lighting_caster_visible(const HdWorldChunk *mesh, const MATRIXF *model, const HdLightingProjection *projection)
{
    float scale = 0;
    for (sint32 row = 0; row < 3; ++row)
        for (sint32 axis = 0; axis < 3; ++axis)
            scale += model->m[row][axis] * model->m[row][axis];
    float radius = (mesh->radius > 0 ? mesh->radius : 1024) * sqrtf(scale);
    for (sint32 plane = 0; plane < 3; ++plane)
        for (sint32 sign = -1; sign <= 1; sign += 2)
        {
            float distance = projection->rows[3][3] + sign * projection->rows[plane][3];
            float normalLength = 0;
            for (sint32 axis = 0; axis < 3; ++axis)
            {
                float normal = projection->rows[3][axis] + sign * projection->rows[plane][axis];
                distance += normal * model->t[axis];
                normalLength += normal * normal;
            }
            if (distance < -radius * sqrtf(normalLength))
                return false;
        }
    return true;
}

static void hd_lighting_caster(const HdWorldChunk *geometry, const MATRIXF *model, const HdLightingScene *scene, sint32 slot, uint32 first = 0)
{
    if (geometry->count == 0 || !hd_lighting_caster_visible(geometry, model, lightingCapture))
        return;
    HdWorldChunk mesh = *geometry;
    mesh.noNight = true;
    VID_MATERIAL *material = carDepthMaterial;
    vid_pass(material);
    VID_RECT viewport = {0, 0, HD_LIGHTING_MAP_SIZE, HD_LIGHTING_MAP_SIZE};
    hd_world_submit(&mesh, model, &scene->camera, &scene->view, viewport, 240, -1, -1, slot, 0, 1, 0, first, 0, 1, 0, material);
    ++hd_lighting_caster_draws;
}

static bool hd_lighting_scene(void *context, const HdLightingProjection *projection)
{
    HdLightingScene *scene = (HdLightingScene *)context;
    sint32 player = projection->layer < HD_LIGHTING_HEAD_LAYER ? projection->layer / HD_LIGHTING_SUN_CASCADES : (projection->layer - HD_LIGHTING_HEAD_LAYER) / HD_LIGHTING_LAMPS;
    if (!hd_render_view_pose(player, scene->phase, &scene->camera, &scene->view))
        return false;
    lightingCapture = projection;
    for (sint32 slot = 0; slot < 9 && worldSubmissionValid; ++slot)
    {
        const HdCarModel *car = carModels + slot;
        if (!car->active)
            continue;
        for (sint32 partIndex = 0; partIndex < 57; ++partIndex)
        {
            const HdCarPart *part = car->parts + partIndex;
            const HdCarPartPose *pose = part->poses;
            if (pose->visible == 0 || pose->visible >= 17 || part->geometry.count == 0 || (car->type < 28 && (partIndex == 45 || partIndex == 46)))
                continue;
            MATRIXF model;
            hd_car_part_model(carModels[slot].parts + partIndex, 0, scene->phase, &model);
            const HdCarSurface *surfaces = hd_car_surfaces(carModels + slot, partIndex, 0);
            if (surfaces == 0)
            {
                worldSubmissionValid = false;
                break;
            }
            HdCarPart *cached = carModels[slot].parts + partIndex;
            if (cached->shadowIndices == 0)
                cached->shadowIndices = vid_index_create(part->geometry.count);
            if (cached->shadowIndices == 0)
            {
                worldSubmissionValid = false;
                break;
            }
            if (cached->shadowGeneration != renderGeneration)
            {
                uint32 *indices = vid_index_data(cached->shadowIndices);
                uint32 used = 0;
                bool changed = false;
                for (uint32 face = 0; face < part->geometry.count / 3; ++face)
                    if (surfaces[face].valid && surfaces[face].castsShadow)
                        for (uint32 corner = 0; corner < 3; ++corner)
                        {
                            uint32 value = face * 3 + corner;
                            if (indices[used] != value)
                                changed = true;
                            indices[used++] = value;
                        }
                if (used != 0 && (changed || used != cached->shadowCount))
                    vid_index_commit(cached->shadowIndices, used);
                cached->shadowCount = used;
                cached->shadowGeneration = renderGeneration;
            }
            if (cached->shadowCount != 0)
            {
                HdWorldChunk mesh = part->geometry;
                mesh.indices = cached->shadowIndices;
                mesh.count = cached->shadowCount;
                hd_lighting_caster(&mesh, &model, scene, slot);
            }
        }
    }
    lightingCapture = 0;
    return worldSubmissionValid;
}

static void hd_lighting_car_lamps(sint32 slot, float phase)
{
    const HdCarModel *car = carModels + slot;
    if (!car->active || car->headlights == 0)
        return;
    MATRIXF body;
    if (!hd_render_car_pose(slot, phase, &body))
        return;
    sint32 count = 0;
    VEC3F positions[2];
    for (sint32 partIndex = 0; partIndex < 57 && count < 2; ++partIndex)
    {
        const HdCarPart *part = car->parts + partIndex;
        const HdCarPartPose *pose = part->poses;
        if (part->object == 0 || pose->visible != 22)
            continue;
        MATRIXF model;
        hd_mix_pose(&pose->previous, &pose->current, poseRenderPhase, &model, 0);
        for (sint32 facetIndex = 0; facetIndex < part->object->numFacet && count < 2; ++facetIndex)
        {
            const Transformer::zFacet *facet = part->object->facet + facetIndex;
            if (facet->vertexId0 >= part->object->numVertex || hd_car_halo_type(car, pose->visible, facet->flag) < 0)
                continue;
            const COORD16 *point = part->object->vertex + facet->vertexId0;
            float local[3] = {point->x * (pose->mirrored ? -0.25f : 0.25f), point->y * 0.25f, point->z * 0.25f};
            HdLightingLamp lamp = {};
            for (sint32 row = 0; row < 3; ++row)
            {
                lamp.position.v[row] = model.t[row];
                for (sint32 column = 0; column < 3; ++column)
                    lamp.position.v[row] += model.m[row][column] * local[column];
            }
            if (count != 0)
            {
                float distance2 = 0;
                for (sint32 axis = 0; axis < 3; ++axis)
                {
                    float delta = lamp.position.v[axis] - positions[0].v[axis];
                    distance2 += delta * delta;
                }
                if (distance2 < 1)
                    continue;
            }
            float front = 0;
            for (sint32 axis = 0; axis < 3; ++axis)
                front += (lamp.position.v[axis] - body.t[axis]) * body.m[axis][2];
            for (sint32 axis = 0; axis < 3; ++axis)
                lamp.direction.v[axis] = body.m[axis][2] * (front >= 0 ? 1 : -1) - body.m[axis][1] * 0.025f;
            const CVECTOR *color = &car->headlightColor;
            lamp.color[0] = color->r / 255.0f * hd_headlight_intensity;
            lamp.color[1] = color->g / 255.0f * hd_headlight_intensity;
            lamp.color[2] = color->b / 255.0f * hd_headlight_intensity;
            lamp.range = (car->headlights & 4) != 0 ? 8192.0f : 4096.0f;
            lamp.owner = slot;
            lamp.active = true;
            positions[count] = lamp.position;
            hd_lighting_lamp(slot * 2 + count++, &lamp);
        }
    }
}

static bool hd_lighting_prepare(float phase)
{
    hd_lighting_frame(current.viewCount);
    if (!hd_lighting_depth_resources())
        return false;
    worldSubmissionValid = true;
    hd_car_upload();
    HdLightingScene scene;
    scene.phase = phase;
    for (sint32 player = 0; player < current.viewCount; ++player)
    {
        if (!hd_render_view_pose(player, phase, &scene.camera, &scene.view))
            return false;
        VEC3F shadowDirection = {(float)skyCurrent.sun.vx, (float)skyCurrent.sun.vy, (float)skyCurrent.sun.vz};
        shadowDirection = hd_lighting_shadow_direction(&shadowDirection, GameSetup::gData.track);
        float sunWeight = 1;
        for (sint32 slot = 0; slot < 9; ++slot)
            if (current.cars[slot].active && current.cars[slot].object == scene.view.anchor)
            {
                sunWeight = hd_car_sun_fade(slot, phase);
                break;
            }
        float distance = scene.view.world.chunkFarSquared > 0 ? sqrtf(scene.view.world.chunkFarSquared) * 2.5f : 16384;
        hd_lighting_sun(player, &scene.camera, &shadowDirection, hd_gpu_shadows_enabled != 0 && GameSetup::gData.Time == 0, distance, sunWeight);
    }
    if (hd_headlights_enabled != 0 && GameSetup::gData.Time != 0)
        for (sint32 slot = 0; slot < 9; ++slot)
            hd_lighting_car_lamps(slot, phase);
    if (!hd_render_view_pose(0, phase, &scene.camera, &scene.view))
        return false;
    return hd_lighting_capture(hd_lighting_scene, &scene);
}

static bool hd_world_view(sint32 player, float phase, VID_RECT viewport)
{
    lightingView = player;
    effectFogActive = false;
    memset(effectFogUniforms, 0, sizeof(effectFogUniforms));
    MATRIXF camera;
    HdViewPose view;
    if (!hd_render_view_pose(player, phase, &camera, &view))
        return false;
    worldSemiCount = 0;
    hd_night_view(player, phase);
    orderedCount = 0;
    HdCarOrder carOrders[9];
    hd_car_order(phase, &camera, &view, carOrders);
    float logicalHeight = current.viewCount == 2 ? 120.0f : 240.0f;
    vid_viewport(viewport);
    hd_sky_view(player, phase, &camera, &view, viewport, logicalHeight);
    hd_fog_uniforms(effectFogUniforms, &view, viewport, logicalHeight);
    effectFogActive = true;
    vid_pass(worldMaterial);
    if (!hd_world_batch_view(player, &camera, &view, viewport, logicalHeight) || !hd_static_world_view(player, phase, &camera, &view, viewport, logicalHeight) || !hd_billboard_view(player, phase, &camera, &view, viewport, logicalHeight))
        return false;
    for (sint32 chunk = 0; chunk < worldChunkCount; ++chunk)
    {
        const HdWorldChunk *world = worldChunks + chunk;
        if (!hd_chunk_visible(&view, chunk, false))
            continue;
        MATRIXF model;
        IdentityMatrixf(&model);
        model.t[0] = world->center.x;
        model.t[1] = world->center.y;
        model.t[2] = world->center.z;
        if (!hd_world_geometry(world, &model, &camera, &view, viewport, logicalHeight, -1, 0, hd_world_group(&view, chunk), 0, 0, true, false))
            return false;
    }
    for (uint32 i = 0; i < worldObjectCount; ++i)
    {
        const HdObjectEntry *entry = worldObjects + i;
        uint32 group = entry->kind < 2 ? 32 + hd_world_group(&view, entry->chunk) * 2 + entry->kind : 102 + entry->kind;
        if (entry->pieceCount != 0 && (entry->kind == 4 || hd_chunk_visible(&view, entry->chunk, true)))
            for (uint32 pieceIndex = 0; pieceIndex < entry->pieceCount; ++pieceIndex)
            {
                const HdObjectPiece *piece = entry->pieces + pieceIndex;
                MATRIXF model;
                hd_object_piece_matrix(piece, objectReset ? 1 : phase, &model, entry->current.orderOffset == -1 ? 4.0f : 1.0f);
                if (!hd_world_geometry(worldDefinitions + piece->definition, &model, &camera, &view, viewport, logicalHeight, piece->light, entry->current.orderOffset, group, entry->ordinal, pieceIndex))
                    return false;
                ++hd_destruction_draws;
            }
        if (!hd_object_visible(entry, &view))
            continue;
        MATRIXF model;
        hd_object_matrix(entry, phase, &camera, &model);
        if (!hd_world_geometry(worldDefinitions + entry->current.definition, &model, &camera, &view, viewport, logicalHeight, entry->current.light, entry->current.orderOffset, group, entry->ordinal, entry->pieceCount, !entry->current.billboard, !(hd_static_world_match(i, &model) || hd_billboard_match(i, player, &model))))
            return false;
        ++hd_world_object_draws;
    }
    hd_road_view(player, phase, &camera, &view, viewport, logicalHeight);
    hd_skids_view(player, phase, &camera, &view, viewport, logicalHeight);
    hd_cars_view(phase, &camera, &view, viewport, logicalHeight, carOrders);
    hd_world_semi_view(&camera, &view, viewport, logicalHeight);
    hd_track_halos_view(player, phase, &camera, &view, viewport, logicalHeight);
    hd_souffle_view(player, phase, &camera, &view, viewport, logicalHeight);
    hd_car_halos_view(player, phase, &camera, &view, viewport, logicalHeight, carOrders);
    hd_order_flush();
    hd_weather_view(player, phase, &camera, &view, viewport, logicalHeight);
    effectFogActive = false;
    if (hd_feedback_active() && !lensGpuFrame)
        hd_feedback_view(player);
    return true;
}

struct HdCubeScene
{
    HdViewPose view;
    MATRIXF sourceCamera;
    float phase;
};

static sint32 hd_player_cube_face(void *context, const VID_CUBE_FACE *face)
{
    cubeFace = face->index;
    const HdCubeScene *scene = (const HdCubeScene *)context;
    MATRIXF camera;
    IdentityMatrixf(&camera);
    for (sint32 row = 0; row < 3; ++row)
    {
        camera.t[row] = cubeOrigin.v[row];
        for (sint32 axis = 0; axis < 3; ++axis)
            camera.m[axis][row] = face->rotation[row][axis];
    }
    HdViewPose view = scene->view;
    view.pose.position = cubeOrigin;
    view.anchor = view.target = 0;
    view.inCar = false;
    view.mirror = false;
    view.projection = face->size * 0.5f;
    VID_RECT viewport = {0, 0, (sint32)face->size, (sint32)face->size};
    VID_GEOMETRY *originalSky = skyMeshes[0];
    VID_GEOMETRY *originalSkids = skidMeshes[0];
    skyMeshes[0] = cubeSkyMeshes[face->index];
    skidMeshes[0] = cubeSkidMeshes[face->index];
    hd_sky_view(0, scene->phase, &camera, &view, viewport, (float)face->size);
    skyMeshes[0] = originalSky;
    worldSemiCount = 0;
    orderedCount = 0;
    lightingView = 0;
    hd_night_view(0, scene->phase);
    if (!hd_world_batch_view(0, &camera, &view, viewport, (float)face->size) || !hd_static_world_view(0, scene->phase, &camera, &view, viewport, (float)face->size) || !hd_billboard_view(0, scene->phase, &camera, &view, viewport, (float)face->size))
        return 0;
    for (sint32 chunk = 0; chunk < worldChunkCount && worldSubmissionValid; ++chunk)
    {
        const HdWorldChunk *world = worldChunks + chunk;
        if (!hd_chunk_visible(&view, chunk, false))
            continue;
        MATRIXF model;
        IdentityMatrixf(&model);
        memcpy(model.t, &world->center, sizeof(model.t));
        if (!hd_world_geometry(world, &model, &camera, &view, viewport, (float)face->size, -1, 0, chunk, 0, 0, true, false))
            worldSubmissionValid = false;
    }
    for (uint32 index = 0; index < worldObjectCount && worldSubmissionValid; ++index)
    {
        const HdObjectEntry *entry = worldObjects + index;
        uint32 group = entry->kind < 2 ? 32 + hd_world_group(&view, entry->chunk) * 2 + entry->kind : 102 + entry->kind;
        for (uint32 pieceIndex = 0; pieceIndex < entry->pieceCount; ++pieceIndex)
        {
            const HdObjectPiece *piece = entry->pieces + pieceIndex;
            MATRIXF model;
            hd_object_piece_matrix(piece, objectReset ? 1 : scene->phase, &model, entry->current.orderOffset == -1 ? 4.0f : 1.0f);
            if (!hd_world_geometry(worldDefinitions + piece->definition, &model, &camera, &view, viewport, (float)face->size, piece->light, entry->current.orderOffset, group, entry->ordinal, pieceIndex))
                worldSubmissionValid = false;
        }
        if (!hd_object_visible(entry, &view))
            continue;
        MATRIXF model;
        hd_object_matrix(entry, scene->phase, &scene->sourceCamera, &model);
        if (!hd_world_geometry(worldDefinitions + entry->current.definition, &model, &camera, &view, viewport, (float)face->size, entry->current.light, entry->current.orderOffset, group, entry->ordinal, entry->pieceCount, !entry->current.billboard, !(hd_static_world_match(index, &model) || hd_billboard_match(index, 0, &model))))
            worldSubmissionValid = false;
    }
    hd_road_view(0, scene->phase, &camera, &view, viewport, (float)face->size);
    hd_skids_view(0, scene->phase, &camera, &view, viewport, (float)face->size);
    skidMeshes[0] = originalSkids;
    HdCarOrder orders[9] = {};
    for (sint32 slot = 0; slot < 9; ++slot)
        orders[slot].detail = !carModels[slot].active ? -1 : carModels[slot].type >= 28 || carModels[slot].mediumOnly ? 1 : 0;
    hd_cars_view(scene->phase, &camera, &view, viewport, (float)face->size, orders);
    hd_world_semi_view(&camera, &view, viewport, (float)face->size);
    hd_order_flush();
    ++hd_cube_faces;
    return worldSubmissionValid ? 1 : 0;
}

static bool hd_player_cube_prepare(float phase)
{
    if (hd_dynamic_reflections_enabled == 0 || hd_car_reflections_enabled == 0)
        return true;
    sint32 owner = -1;
    for (sint32 slot = 0; slot < 9; ++slot)
        if (carModels[slot].active && carModels[slot].source == Cars::gHumanRaceCarList[0])
            owner = slot;
    if (owner < 0)
        return true;
    bool paused = simVar.pauseSim != 0 || simVar.quickPauseSim != 0;
    if (cubeReady && cubeOwner == carModels[owner].source && (paused || (cubeTick == simGlobal.gameTicks && cubePhase == phase)))
        return true;
    if (playerCube == 0)
    {
        playerCube = vid_target_create_cube_depth(256);
        if (playerCube != 0)
            vid_tex_set_mip_levels(vid_target_texture(playerCube), 3);
    }
    if (playerCube == 0)
        return false;
    if (carCubeShader == 0)
        carCubeShader = vid_shader_create(carVertex, carCompositeFragment);
    if (carCubeShader == 0)
        return false;
    for (sint32 orientation = 0; orientation < 2; ++orientation)
        if (carCubeMaterials[orientation] == 0)
        {
            carCubeMaterials[orientation] = vid_mat_create();
            if (carCubeMaterials[orientation] == 0)
                return false;
            VID_MATERIAL_DATA *material = vid_mat_data(carCubeMaterials[orientation]);
            *material = *vid_mat_read(carMaterial[orientation]);
            material->texture = worldVram;
            material->aux[1] = vid_target_texture(playerCube);
            material->aux[2] = vid_target_depth_texture(playerCube);
            material->shader = carCubeShader;
            material->aux_filter[1] = VID_FILTER_TRILINEAR;
            material->blend = VID_BLEND_NONE;
            material->depth_write = 1;
            material->depth_bias_slope = 0;
            material->depth_bias_units = 0;
            vid_mat_commit(carCubeMaterials[orientation]);
        }
    for (sint32 orientation = 0; orientation < 2; ++orientation)
        for (sint32 blend = 0; blend < 4; ++blend)
            if (carCubeSemiMaterials[blend][orientation] == 0)
            {
                carCubeSemiMaterials[blend][orientation] = vid_mat_create();
                if (carCubeSemiMaterials[blend][orientation] == 0)
                    return false;
                VID_MATERIAL_DATA *material = vid_mat_data(carCubeSemiMaterials[blend][orientation]);
                *material = *vid_mat_read(carSemiMaterials[blend][orientation]);
                material->aux[1] = vid_target_texture(playerCube);
                material->aux[2] = vid_target_depth_texture(playerCube);
                material->aux_filter[1] = VID_FILTER_TRILINEAR;
                material->shader = carCubeShader;
                vid_mat_commit(carCubeSemiMaterials[blend][orientation]);
            }
    if (cubeSkyCapacity < skyCapacities[0])
    {
        cubeSkyCapacity = skyCapacities[0];
        for (sint32 face = 0; face < 6; ++face)
        {
            vid_mesh_free(cubeSkyMeshes[face]);
            cubeSkyMeshes[face] = vid_mesh_create_2d(cubeSkyCapacity);
        }
    }
    for (sint32 face = 0; face < 6; ++face)
    {
        if (cubeSkidMeshes[face] == 0)
            cubeSkidMeshes[face] = vid_mesh_create_2d(HD_SKID_QUADS * 6);
        if (cubeSkyMeshes[face] == 0 || cubeSkidMeshes[face] == 0)
            return false;
    }
    MATRIXF root;
    if (!hd_render_car_pose(owner, phase, &root))
        return false;
    memcpy(&cubeOrigin, root.t, sizeof(cubeOrigin));
    const HdCarPart *body = carModels[owner].parts;
    if (body->object != 0 && body->object->numVertex > 0 && body->poses[0].visible != 0)
    {
        float low[3] = {32767, 32767, 32767};
        float high[3] = {-32768, -32768, -32768};
        for (sint32 vertex = 0; vertex < body->object->numVertex; ++vertex)
        {
            const COORD16 *point = body->object->vertex + vertex;
            const float coordinates[3] = {(float)point->x, (float)point->y, (float)point->z};
            for (sint32 axis = 0; axis < 3; ++axis)
            {
                if (coordinates[axis] < low[axis])
                    low[axis] = coordinates[axis];
                if (coordinates[axis] > high[axis])
                    high[axis] = coordinates[axis];
            }
        }
        hd_mix_pose(&body->poses[0].previous, &body->poses[0].current, poseRenderPhase, &root, 0);
        for (sint32 row = 0; row < 3; ++row)
        {
            cubeOrigin.v[row] = root.t[row];
            for (sint32 axis = 0; axis < 3; ++axis)
                cubeOrigin.v[row] += root.m[row][axis] * (low[axis] + high[axis]) * (axis == 0 && body->poses[0].mirrored ? -0.125f : 0.125f);
        }
    }
    cubeOwner = carModels[owner].source;
    HdCubeScene scene;
    if (!hd_render_view_pose(0, phase, &scene.sourceCamera, &scene.view))
        return false;
    scene.phase = phase;
    float clear[3] = {skyCurrent.clearColor.r / 255.0f, skyCurrent.clearColor.g / 255.0f, skyCurrent.clearColor.b / 255.0f};
    cubeRendering = true;
    worldSubmissionValid = true;
    hd_cube_faces = 0;
    bool success = vid_cube_capture(playerCube, clear, hd_player_cube_face, &scene) != 0;
    cubeRendering = false;
    orderedCount = 0;
    worldSemiCount = 0;
    effectFogActive = false;
    cubeReady = success;
    if (success)
    {
        cubeTick = simGlobal.gameTicks;
        cubePhase = phase;
        ++hd_cube_captures;
    }
    return success;
}

static bool hd_feedback_surface_read(VID_SAMPLE **samples, sint32 width, sint32 height)
{
    if (feedbackSurfaceShader == 0)
        feedbackSurfaceShader = vid_shader_create(0, "void main(){outColor=vec4(vec3(texture(uTex,vUV).r),1.0);}");
    if (feedbackSurfaceShader == 0)
        return false;
    if (feedbackState.surfaceMaterial == 0)
    {
        feedbackState.surfaceMaterial = vid_mat_create();
        if (feedbackState.surfaceMaterial == 0)
            return false;
        VID_MATERIAL_DATA *material = vid_mat_data(feedbackState.surfaceMaterial);
        material->shader = feedbackSurfaceShader;
        material->texture = vid_target_surface_texture(feedbackState.output);
        material->filter = VID_FILTER_NEAREST;
        material->blend = VID_BLEND_NONE;
        material->depth_test = material->depth_write = 0;
        vid_mat_commit(feedbackState.surfaceMaterial);
    }
    if (!vid_begin(0, 0, 0))
        return false;
    VID_RECT viewport = {0, 0, width, height};
    vid_viewport(viewport);
    vid_pass(feedbackState.surfaceMaterial);
    if (!vid_submit(feedbackState.maskMesh, 0, 0, 6))
    {
        vid_cancel();
        return false;
    }
    for (sint32 y = 0; y < 60; ++y)
        for (sint32 x = 0; x < 80; ++x)
            vid_sample(samples[y * 80 + x], (x + 0.5f) / 80, (y + 0.5f) / 60);
    if (!vid_end())
        return false;
    for (sint32 i = 0; i < 80 * 60; ++i)
        hd_world_preview_masks[i] = (vid_sample_color(samples[i]) & 255) >= 128;
    return true;
}

static bool hd_world_frame(float phase, bool diagnostic, bool ui, bool capture = false)
{
    carRenderPhase = phase;
    hd_prepare_pose_phase();
    if (!historyValid || !worldObjectSnapshotValid || current.viewCount < 1 || current.viewCount > 2)
        return false;
    for (sint32 player = 0; player < current.viewCount; ++player)
        if (!trackHaloStates[player].valid)
            return false;
    sint32 width, height;
    vid_resize(&width, &height);
    if (width <= 0 || height <= 0 || !hd_world_resources() || !hd_world_textures() || !hd_world_build() || !hd_world_batch_build() || !hd_sky_resources() || !hd_car_resources() || !hd_shadow_build(phase) || !hd_weather_resources() || !hd_souffle_resources() || !hd_celestial_resources() || !hd_lens_resources() || !hd_car_halo_resources() || !hd_track_halo_resources() || !hd_skid_resources() || !hd_road_resources())
        return false;
    lensGpuFrame = hd_lens_eligible() && hd_lens_enabled != 0 && hd_celestial_enabled != 0;
    vid_stats_read(&renderStatsBegin);
    VID_SHADER *measuredShaders[7] = {worldShader, worldBatchShader, carShader, carCubeShader, carDepthShader, weatherShader, skyShader};
    for (sint32 shader = 0; shader < 7; ++shader)
        shaderStatsBegin[shader] = vid_shader_draw_count(measuredShaders[shader]);
    ++renderGeneration;
    if (!hd_static_world_build(phase))
        return false;
    for (sint32 player = 0; player < current.viewCount; ++player)
        if (!hd_billboard_prepare(player, phase))
            return false;
    if (!hd_cars_prepare())
        return false;
    if (!hd_lighting_prepare(phase))
        return false;
    if (!hd_player_cube_prepare(phase))
        return false;
    if (!hd_feedback_prepare(width, height, ui))
        return false;
    if (ui && !hd_ui_prepare(width, height, phase))
        return false;
    if (!diagnostic && sceneFrame.image == 0)
        sceneFrame.image = vid_target_create();
    if (!diagnostic && sceneFrame.image == 0)
        return false;
    sint32 sampleCount = diagnostic ? 80 * 60 : 0;
    VID_SAMPLE *samples[80 * 60];
    for (sint32 i = 0; i < sampleCount; ++i)
        samples[i] = vid_sample_create();
    if (!vid_begin(skyCurrent.clearColor.r / 255.0f, skyCurrent.clearColor.g / 255.0f, skyCurrent.clearColor.b / 255.0f))
    {
        for (sint32 i = 0; i < sampleCount; ++i)
            vid_sample_free(samples[i]);
        return false;
    }
    VID_TARGET *target = hd_feedback_active() ? feedbackState.output : !diagnostic ? sceneFrame.image : 0;
    if (lensGpuFrame ? !vid_target_select_offscreen(hd_lens_gpu_target()) : target != 0 && !(capture ? vid_target_select_offscreen(target) : vid_target_select(target)))
    {
        vid_cancel();
        for (sint32 i = 0; i < sampleCount; ++i)
            vid_sample_free(samples[i]);
        return false;
    }
    hd_feedback_draws = 0;
    memset(hd_car_variant_draws, 0, sizeof(hd_car_variant_draws));
    hd_car_interior_parts = 0;
    hd_car_part_draws = 0;
    hd_car_triangle_count = 0;
    hd_car_reflection_draws = 0;
    hd_cube_car_draws = 0;
    hd_car_semi_draws = 0;
    hd_car_shadow_draws = 0;
    VID_RECT viewport = {0, 0, width, height};
    vid_viewport(viewport);
    vid_pass(worldMaterial);
    hd_world_object_draws = 0;
    hd_world_semi_draws = 0;
    hd_ordered_draws = 0;
    hd_destruction_draws = 0;
    hd_weather_snow_quads = hd_weather_rain_lines = hd_weather_splat_quads = 0;
    hd_souffle_quads = 0;
    hd_souffle_filtered_quads = 0;
    hd_sky_quads = hd_ring_quads = hd_star_quads = 0;
    hd_sun_triangles = hd_moon_quads = hd_lightning_quads = 0;
    hd_lens_quads = 0;
    hd_lens_queries = 0;
    hd_car_halo_count = hd_car_halo_reflections = hd_car_halo_vertices = 0;
    hd_track_halo_count = hd_track_halo_vertices = 0;
    hd_skid_quads = 0;
    hd_night_draws = hd_night_car_lights = 0;
    hd_road_line_quads = hd_road_line_submits = hd_spike_quads = 0;
    bool frameReady = true;
    worldSubmissionValid = true;
    for (sint32 player = 0; player < current.viewCount; ++player)
    {
        VID_RECT viewRect = {0, height * player / current.viewCount, width, height * (player + 1) / current.viewCount - height * player / current.viewCount};
        frameReady = hd_world_view(player, phase, viewRect) && frameReady;
    }
    if (lensGpuFrame)
    {
        float clear[3] = {skyCurrent.clearColor.r / 255.0f, skyCurrent.clearColor.g / 255.0f, skyCurrent.clearColor.b / 255.0f};
        float logicalWidth = 240.0f * width / height;
        float x = 0.5f + lensCenterX / logicalWidth, y = 0.5f - lensCenterY / 240;
        bool paused = simVar.pauseSim != 0 || simVar.quickPauseSim != 0;
        if (!frameReady || !worldSubmissionValid)
        {
            vid_cancel();
            for (sint32 i = 0; i < sampleCount; ++i)
                vid_sample_free(samples[i]);
            hd_lens_gpu_complete(false);
            hd_feedback_complete(false);
            return false;
        }
        if (!hd_lens_gpu_resolve(target, capture, viewport, clear, x, y, lensMarkerValid, paused, simGlobal.gameTicks))
        {
            for (sint32 i = 0; i < sampleCount; ++i)
                vid_sample_free(samples[i]);
            hd_lens_gpu_complete(false);
            hd_feedback_complete(false);
            return false;
        }
        HdViewPose view;
        MATRIXF camera;
        if (hd_render_view_pose(0, phase, &camera, &view))
            hd_lens_view(&view, viewport, 240, paused ? 1 : phase);
        if (hd_feedback_active())
            hd_feedback_view(0);
    }
    vid_viewport(viewport);
    if (hd_feedback_active())
    {
        vid_pass(feedbackState.maskMaterial);
        frameReady = vid_submit(feedbackState.maskMesh, 0, 0, 6) != 0 && frameReady;
    }
    if (ui)
        frameReady = hd_ui_render(hd_feedback_active()) && frameReady;
    if (!frameReady || !worldSubmissionValid)
    {
        vid_cancel();
        for (sint32 i = 0; i < sampleCount; ++i)
            vid_sample_free(samples[i]);
        hd_lens_gpu_complete(false);
        hd_feedback_complete(false);
        return false;
    }
    vid_viewport(viewport);
    if (diagnostic)
        for (sint32 y = 0; y < 60; ++y)
            for (sint32 x = 0; x < 80; ++x)
                vid_sample(samples[y * 80 + x], (x + 0.5f) / 80, (y + 0.5f) / 60);
    bool success = vid_end() != 0;
    VID_STATS completed;
    vid_stats_read(&completed);
    hd_render_stats.draws = completed.draws - renderStatsBegin.draws;
    hd_render_stats.triangles = completed.triangles - renderStatsBegin.triangles;
    hd_render_stats.program_changes = completed.program_changes - renderStatsBegin.program_changes;
    hd_render_stats.texture_binds = completed.texture_binds - renderStatsBegin.texture_binds;
    hd_render_stats.state_changes = completed.state_changes - renderStatsBegin.state_changes;
    hd_render_stats.uniform_uploads = completed.uniform_uploads - renderStatsBegin.uniform_uploads;
    hd_render_stats.uniform_bytes = completed.uniform_bytes - renderStatsBegin.uniform_bytes;
    hd_render_stats.vertex_bytes = completed.vertex_bytes - renderStatsBegin.vertex_bytes;
    hd_render_stats.index_bytes = completed.index_bytes - renderStatsBegin.index_bytes;
    measuredShaders[3] = carCubeShader;
    measuredShaders[4] = carDepthShader;
    for (sint32 shader = 0; shader < 7; ++shader)
        hd_shader_draws[shader] = vid_shader_draw_count(measuredShaders[shader]) - shaderStatsBegin[shader];
    hd_lens_gpu_complete(success);
    hd_feedback_complete(success);
    if (!diagnostic)
    {
        sceneFrame.ready = success && (!hd_feedback_active() || vid_target_copy(sceneFrame.image, feedbackState.output) != 0);
        if (sceneFrame.ready)
        {
            sceneFrame.width = width;
            sceneFrame.height = height;
            ++hd_scene_captures;
        }
        return success;
    }
#if defined(_DEBUG)
    FILE *file = success ? xport_fopen("../status/enhance/world-preview.ppm", "wb") : 0;
#endif
    hd_world_preview_pixels = 0;
    hd_world_preview_hash = 2166136261;
    hd_world_preview_view_hashes[0] = hd_world_preview_view_hashes[1] = 2166136261;
#if defined(_DEBUG)
    if (file != 0)
        fprintf(file, "P6\n80 60\n255\n");
#endif
    for (sint32 i = 0; i < sampleCount; ++i)
    {
        uint32 color = vid_sample_color(samples[i]);
        hd_world_preview_colors[i] = color;
        hd_world_preview_hash = (hd_world_preview_hash ^ color) * 16777619;
        sint32 player = (i / 80) * current.viewCount / 60;
        hd_world_preview_view_hashes[player] = (hd_world_preview_view_hashes[player] ^ color) * 16777619;
        if (color != 0x121724)
            ++hd_world_preview_pixels;
#if defined(_DEBUG)
        uint8 rgb[3] = {(uint8)(color >> 16), (uint8)(color >> 8), (uint8)color};
        if (file != 0)
            fwrite(rgb, 1, sizeof(rgb), file);
#endif
    }
#if defined(_DEBUG)
    if (file != 0)
        fclose(file);
#endif
    if (success && hd_world_preview_surface != 0 && hd_feedback_active())
        success = hd_feedback_surface_read(samples, width, height);
    for (sint32 i = 0; i < sampleCount; ++i)
        vid_sample_free(samples[i]);
    return success;
}

bool hd_world_preview(void)
{
    return hd_world_frame(hd_world_preview_phase, true, hd_world_ui != 0);
}

bool hd_world_material_check(void)
{
    hd_world_material_checks = hd_world_material_errors = 0;
    hd_order_material_checks = hd_order_material_errors = 0;
    if (orderedCount != 0 || !hd_world_resources())
        return false;
    bool submissionValid = worldSubmissionValid;
    uint32 orderedDrawCount = hd_ordered_draws;
    VID_TEXTURE *pixels = vid_tex_create_fmt(4, 1, VID_RGBA5551);
    VID_TEXTURE *metadata = vid_tex_create_fmt(3, 1, VID_RGBA16UI);
    VID_GEOMETRY *mesh = vid_mesh_create_16(6);
    VID_MATERIAL *materials[2] = {vid_mat_create(), vid_mat_create()};
    VID_SAMPLE *samples[4] = {vid_sample_create(), vid_sample_create(), vid_sample_create(), vid_sample_create()};
    bool success = pixels != 0 && metadata != 0 && mesh != 0 && materials[0] != 0 && materials[1] != 0 && samples[0] != 0 && samples[1] != 0 && samples[2] != 0 && samples[3] != 0;
    if (success)
    {
        const uint16 words[4] = {0x001F, 0x83E0, 0x8000, 0};
        memcpy(vid_tex_data(pixels), words, sizeof(words));
        vid_tex_commit(pixels);
        const sint16 points[4][4] = {{-4, 4, 8, 1}, {4, 4, 8, 1}, {-4, -4, 8, 1}, {4, -4, 8, 1}};
        const uint8 order[6] = {0, 1, 2, 2, 1, 3};
        VID_VERTEX16 *vertices = vid_mesh_data_16(mesh);
        for (sint32 index = 0; index < 6; ++index)
        {
            memset(vertices + index, 0, sizeof(*vertices));
            memcpy(vertices[index].pos, points[order[index]], sizeof(points[0]));
            vertices[index].tex[1] = order[index];
        }
        vid_mesh_commit(mesh);
        float uniforms[18][4] = {0};
        uniforms[0][0] = uniforms[1][1] = uniforms[2][2] = uniforms[14][3] = 1;
        uniforms[3][0] = uniforms[3][1] = 2;
        uniforms[3][2] = 65536.0f / 65534;
        uniforms[3][3] = -131070.0f / 65534;
        const float background[3] = {64, 128, 192};
        sint32 width, height;
        vid_resize(&width, &height);
        success = width > 0 && height > 0;
        for (sint32 semi = 0; semi < 2 && success; ++semi)
            for (sint32 blend = 0; blend < 4 && success; ++blend)
            {
                uint16 values[12] = {0, 3, 0, 3, 0, (uint16)(0x100 | (blend << 5)), 0, (uint16)(1 | (semi << 1))};
                memcpy(vid_tex_data(metadata), values, sizeof(values));
                vid_tex_commit(metadata);
                for (sint32 pass = 0; pass < 2; ++pass)
                {
                    VID_MATERIAL_DATA *material = vid_mat_data(materials[pass]);
                    *material = *vid_mat_read(pass == 0 ? worldMaterial : worldSemiMaterials[blend]);
                    material->texture = pixels;
                    material->aux[0] = metadata;
                    material->cull = 0;
                    vid_mat_commit(materials[pass]);
                }
                success = vid_begin(background[0] / 255, background[1] / 255, background[2] / 255) != 0;
                if (!success)
                    break;
                VID_RECT viewport = {0, 0, width, height};
                vid_viewport(viewport);
                vid_uniforms(&uniforms[0][0], 18);
                for (sint32 pass = 0; pass < 2; ++pass)
                {
                    vid_pass(materials[pass]);
                    success = vid_submit(mesh, 0, 0, 6) != 0 && success;
                }
                for (sint32 index = 0; index < 4; ++index)
                    success = vid_sample(samples[index], ((float)index + 0.5f) / 4, 0.5f) != 0 && success;
                if (!success)
                {
                    vid_cancel();
                    break;
                }
                success = vid_end() != 0;
                for (sint32 index = 0; index < 4 && success; ++index)
                    for (sint32 channel = 0; channel < 3; ++channel)
                    {
                        float expected = index == 3 ? background[channel] : index == 0 ? (channel == 0 ? 255.0f : 0.0f) : index == 1 && channel == 1 ? 255.0f : 0.0f;
                        if (semi != 0 && (index == 1 || index == 2))
                            expected = blend == 0 ? (expected + background[channel]) * 0.5f : blend == 2 ? background[channel] - expected : background[channel] + expected * (blend == 3 ? 0.25f : 1.0f);
                        if (expected < 0)
                            expected = 0;
                        if (expected > 255)
                            expected = 255;
                        sint32 actual = (vid_sample_color(samples[index]) >> ((2 - channel) * 8)) & 255;
                        if (fabsf((float)actual - expected) > 1)
                            ++hd_world_material_errors;
                        ++hd_world_material_checks;
                    }
            }
        if (success)
        {
            const uint16 values[8] = {0, 3, 0, 3, 0, 0x120, 0, 3};
            memcpy(vid_tex_data(metadata), values, sizeof(values));
            vid_tex_commit(metadata);
            for (sint32 pass = 0; pass < 2; ++pass)
            {
                VID_MATERIAL_DATA *material = vid_mat_data(materials[pass]);
                *material = *vid_mat_read(worldSemiMaterials[1]);
                material->texture = pixels;
                material->aux[0] = metadata;
                material->blend = pass == 0 ? VID_BLEND_ADD : VID_BLEND_SUBTRACT;
                material->cull = 0;
                vid_mat_commit(materials[pass]);
            }
            for (sint32 test = 0; test < 6 && success; ++test)
            {
                bool addFirst = (test & 1) == 0;
                success = vid_begin(background[0] / 255, background[1] / 255, background[2] / 255) != 0;
                if (!success)
                    break;
                VID_RECT viewport = {0, 0, width, height};
                vid_viewport(viewport);
                worldSubmissionValid = true;
                for (sint32 index = 0; index < 2; ++index)
                {
                    sint32 pass = test >= 2 ? 1 - index : index;
                    bool first = (pass == 0) == addFirst;
                    HdOrderKey key = {test < 4 && first ? 200 : 100, test >= 4 && first ? 4 : 0, 0, 0, 0, 0, 0};
                    hd_order_draw(&key, materials[pass], mesh, 0, 0, 6, &uniforms[0][0], 18);
                }
                hd_order_flush();
                success = worldSubmissionValid && vid_sample(samples[1], 0.375f, 0.5f) != 0;
                if (!success)
                {
                    vid_cancel();
                    break;
                }
                success = vid_end() != 0;
                for (sint32 channel = 0; channel < 3 && success; ++channel)
                {
                    float expected = channel == 1 ? (addFirst ? 0.0f : 255.0f) : background[channel];
                    sint32 actual = (vid_sample_color(samples[1]) >> ((2 - channel) * 8)) & 255;
                    if (fabsf((float)actual - expected) > 1)
                        ++hd_order_material_errors;
                    ++hd_order_material_checks;
                }
            }
        }
    }
    for (sint32 index = 0; index < 4; ++index)
        vid_sample_free(samples[index]);
    for (sint32 pass = 0; pass < 2; ++pass)
        vid_mat_free(materials[pass]);
    vid_mesh_free(mesh);
    vid_tex_free(metadata);
    vid_tex_free(pixels);
    worldSubmissionValid = submissionValid;
    hd_ordered_draws = orderedDrawCount;
    return success && hd_world_material_checks == 96 && hd_world_material_errors == 0 && hd_order_material_checks == 18 && hd_order_material_errors == 0;
}

bool hd_car_material_check(void)
{
    hd_car_material_checks = hd_car_material_errors = 0;
    if (!hd_world_resources() || !hd_car_resources())
        return false;
    VID_TEXTURE *pixels = vid_tex_create_fmt(4, 1, VID_RGBA5551);
    VID_TEXTURE *metadata = vid_tex_create_fmt(256, 1, VID_RGBA16UI);
    VID_GEOMETRY *mesh = vid_mesh_create_16(6);
    VID_MATERIAL *materials[2] = {vid_mat_create(), vid_mat_create()};
    VID_SAMPLE *samples[4] = {vid_sample_create(), vid_sample_create(), vid_sample_create(), vid_sample_create()};
    bool success = pixels != 0 && metadata != 0 && mesh != 0 && materials[0] != 0 && materials[1] != 0 && samples[0] != 0 && samples[1] != 0 && samples[2] != 0 && samples[3] != 0;
    if (success)
    {
        const uint16 words[4] = {0x001F, 0x83E0, 0x8000, 0};
        memcpy(vid_tex_data(pixels), words, sizeof(words));
        vid_tex_commit(pixels);
        const sint16 points[4][4] = {{-4, 4, 8, 1}, {4, 4, 8, 1}, {-4, -4, 8, 1}, {4, -4, 8, 1}};
        const uint8 order[6] = {0, 1, 2, 2, 1, 3};
        const float background[3] = {64, 128, 192};
        float uniforms[18][4] = {0};
        uniforms[0][0] = uniforms[1][1] = uniforms[2][2] = 1;
        uniforms[3][0] = uniforms[3][1] = 2;
        uniforms[3][2] = 65536.0f / 65534;
        uniforms[3][3] = -131070.0f / 65534;
        uniforms[5][0] = uniforms[5][1] = uniforms[5][2] = 128.0f / 255;
        sint32 width, height;
        vid_resize(&width, &height);
        success = width > 0 && height > 0;
        for (sint32 semi = 0; semi < 2 && success; ++semi)
            for (sint32 blend = 0; blend < 4 && success; ++blend)
            {
                uint16 *data = (uint16 *)vid_tex_data(metadata);
                memset(data, 0, 256 * 4 * sizeof(*data));
                data[1] = (uint16)(0x100 | (blend << 5));
                data[193 * 4 + 2] = 1;
                vid_tex_commit(metadata);
                VID_VERTEX16 *vertices = vid_mesh_data_16(mesh);
                for (sint32 index = 0; index < 6; ++index)
                {
                    memset(vertices + index, 0, sizeof(*vertices));
                    memcpy(vertices[index].pos, points[order[index]], sizeof(points[0]));
                    vertices[index].uv[0] = (order[index] & 1) != 0 ? 3 : 0;
                    vertices[index].tex[1] = (uint16)semi;
                    vertices[index].tex[2] = 2;
                }
                vid_mesh_commit(mesh);
                for (sint32 pass = 0; pass < 2; ++pass)
                {
                    VID_MATERIAL_DATA *material = vid_mat_data(materials[pass]);
                    *material = *vid_mat_read(pass == 0 ? carMaterial[0] : carSemiMaterials[blend][0]);
                    material->texture = pixels;
                    material->aux[0] = metadata;
                    material->cull = 0;
                    vid_mat_commit(materials[pass]);
                }
                success = vid_begin(background[0] / 255, background[1] / 255, background[2] / 255) != 0;
                if (!success)
                    break;
                VID_RECT viewport = {0, 0, width, height};
                vid_viewport(viewport);
                vid_uniforms(&uniforms[0][0], 18);
                for (sint32 pass = 0; pass < 2; ++pass)
                {
                    vid_pass(materials[pass]);
                    success = vid_submit(mesh, 0, 0, 6) != 0 && success;
                }
                for (sint32 index = 0; index < 4; ++index)
                    success = vid_sample(samples[index], ((float)index + 0.5f) / 4, 0.5f) != 0 && success;
                if (!success)
                {
                    vid_cancel();
                    break;
                }
                success = vid_end() != 0;
                for (sint32 index = 0; index < 4 && success; ++index)
                    for (sint32 channel = 0; channel < 3; ++channel)
                    {
                        float expected = index == 3 ? background[channel] : index == 0 ? (channel == 0 ? 255.0f : 0.0f) : index == 1 && channel == 1 ? 255.0f : 0.0f;
                        if (semi != 0 && (index == 1 || index == 2))
                            expected = blend == 0 ? (expected + background[channel]) * 0.5f : blend == 2 ? background[channel] - expected : background[channel] + expected * (blend == 3 ? 0.25f : 1.0f);
                        if (expected < 0)
                            expected = 0;
                        if (expected > 255)
                            expected = 255;
                        sint32 actual = (vid_sample_color(samples[index]) >> ((2 - channel) * 8)) & 255;
                        if (fabsf((float)actual - expected) > 1)
                            ++hd_car_material_errors;
                        ++hd_car_material_checks;
                    }
            }
    }
    for (sint32 index = 0; index < 4; ++index)
        vid_sample_free(samples[index]);
    for (sint32 pass = 0; pass < 2; ++pass)
        vid_mat_free(materials[pass]);
    vid_mesh_free(mesh);
    vid_tex_free(metadata);
    vid_tex_free(pixels);
    return success && hd_car_material_checks == 96 && hd_car_material_errors == 0;
}

void hd_og_presented(void)
{
    if (simVar.pauseSim == 0)
        sceneFrame.ready = false;
}

void hd_capture_pause(void)
{
    if (!sceneFrame.ready && !xport_is_headless() && hd_ui_world_ready(current.viewCount))
        hd_world_frame(1, false, true, true);
}

static bool hd_pause_prepare(sint32 width, sint32 height)
{
    if (!sceneFrame.ready || sceneFrame.width <= 0 || sceneFrame.height <= 0 || !hd_ui_pause_ready() || !hd_ui_prepare(width, height))
        return false;
    if (sceneShader == 0)
        sceneShader = vid_shader_create(0, "void main(){outColor=vec4(texture(uTex,vUV).rgb,1.0);}");
    if (sceneShader == 0)
        return false;
    if (sceneFrame.material == 0)
    {
        sceneFrame.material = vid_mat_create();
        if (sceneFrame.material == 0)
            return false;
        VID_MATERIAL_DATA *material = vid_mat_data(sceneFrame.material);
        material->shader = sceneShader;
        material->texture = vid_target_texture(sceneFrame.image);
        material->filter = VID_FILTER_LINEAR;
        material->blend = VID_BLEND_NONE;
        material->depth_test = material->depth_write = 0;
        vid_mat_commit(sceneFrame.material);
    }
    if (sceneFrame.mesh == 0)
        sceneFrame.mesh = vid_mesh_create(6);
    if (sceneFrame.mesh == 0)
        return false;
    VID_RECT viewport = {0, 0, width, height};
    VID_VERTEX *vertices = vid_mesh_data(sceneFrame.mesh);
    hd_feedback_quad(vertices, 0, viewport, width, height, 240);
    float fitWidth = (float)width;
    float fitHeight = width * (float)sceneFrame.height / sceneFrame.width;
    if (fitHeight > height)
    {
        fitHeight = (float)height;
        fitWidth = height * (float)sceneFrame.width / sceneFrame.height;
    }
    for (sint32 i = 0; i < 6; ++i)
    {
        vertices[i].pos[0] *= fitWidth / width;
        vertices[i].pos[1] *= fitHeight / height;
    }
    vid_mesh_commit(sceneFrame.mesh);
    return true;
}

static bool hd_pause_frame(bool diagnostic)
{
    sint32 width, height;
    vid_resize(&width, &height);
    if (width <= 0 || height <= 0 || !hd_pause_prepare(width, height))
        return false;
    sint32 count = diagnostic ? 80 * 60 : 0;
    VID_SAMPLE *samples[80 * 60];
    for (sint32 i = 0; i < count; ++i)
        samples[i] = vid_sample_create();
    if (!vid_begin(0, 0, 0))
    {
        for (sint32 i = 0; i < count; ++i)
            vid_sample_free(samples[i]);
        return false;
    }
    VID_RECT viewport = {0, 0, width, height};
    vid_viewport(viewport);
    vid_pass(sceneFrame.material);
    if (!vid_submit(sceneFrame.mesh, 0, 0, 6) || !hd_ui_render(false))
    {
        vid_cancel();
        for (sint32 i = 0; i < count; ++i)
            vid_sample_free(samples[i]);
        return false;
    }
    vid_viewport(viewport);
    if (diagnostic)
        for (sint32 y = 0; y < 60; ++y)
            for (sint32 x = 0; x < 80; ++x)
                vid_sample(samples[y * 80 + x], (x + 0.5f) / 80, (y + 0.5f) / 60);
    bool success = vid_end() != 0;
    if (success)
        ++hd_pause_frames;
    if (diagnostic)
    {
        hd_pause_preview_hash = 2166136261;
        hd_pause_preview_pixels = 0;
        for (sint32 i = 0; i < count; ++i)
        {
            uint32 color = vid_sample_color(samples[i]);
            hd_pause_preview_colors[i] = color;
            hd_pause_preview_hash = (hd_pause_preview_hash ^ color) * 16777619;
            if (color != 0)
                ++hd_pause_preview_pixels;
            vid_sample_free(samples[i]);
        }
    }
    return success;
}

bool hd_pause_preview(void)
{
    return hd_pause_frame(true);
}

bool hd_world_raster_ready(void)
{
    return hd_ui_world_ready(current.viewCount) && (feedbackSource.mode & 1) == 0;
}

bool hd_present(float phase)
{
    if (hd_ui_frontend_ready())
    {
        hd_frontend_sample(phase);
        return hd_ui_present();
    }
    if (simVar.pauseSim != 0 || hd_ui_pause_ready())
        return hd_pause_frame(false);
    if (!hd_ui_world_ready(current.viewCount))
        return false;
    return hd_world_frame(phase, false, true);
}
