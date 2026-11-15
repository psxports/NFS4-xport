#ifndef HD_H
#define HD_H

#include "xport.h"
#include "psx.h"
#include "hd_sampling.h"
#include "game/common/bo.h"
#include "game/common/car.h"
#include "game/common/renderview.h"
#include "game/psx/draw.h"

struct HdRenderTransform
{
    QUATF rotation;
    float stretch[3][3];
    bool valid;
};

struct HdPose
{
    VEC3F position;
    QUATF rotation;
};

struct HdCarPose
{
    const Car::tObj *source;
    const BO::tNewtonObj *object;
    HdPose pose;
    bool active;
};

struct HdLensPiece
{
    sint32 distance;
    sint32 size;
    CVECTOR color;
    uint8 type;
};

struct HdHaloStyle
{
    CVECTOR halo;
    CVECTOR beam;
    sint32 scale;
    sint32 flags;
};

struct HdHaloTables
{
    HdHaloStyle styles[34];
    SVECTOR spikes[4];
    SVECTOR octagon[8];
    SVECTOR hexagon[6];
    SVECTOR reflection[6];
    SVECTOR quad[4];
    uint8 spikeFrom[8];
    uint8 spikeTo[8];
};

struct HdCarHaloSource
{
    sint32 type;
    sint32 overlay;
};

enum
{
    HD_SKID_QUADS = 16 * 24
};

struct HdSkidQuad
{
    const void *owner;
    VEC3F center;
    VEC3F points[4];
    CVECTOR colors[4];
    sint32 type;
};

struct HdSkidSource
{
    HdSkidQuad quads[HD_SKID_QUADS];
    Draw::tPixMap pixmaps[2];
    uint16 palettes[2][16];
    bool pixmapValid[2];
    uint32 count;
};

struct HdRoadSource
{
    Draw::tPixMap lines[10];
    bool lineValid[10];
    Draw::tPixMap spike;
    bool spikeValid;
    bool spikeActive;
    sint32 slice;
    sint32 chunk;
    sint32 x;
    sint32 width;
    coorddef center;
    sint8 right[3];
    sint8 forward[3];
    CVECTOR color;
};

struct HdNightSource
{
    const BO::tNewtonObj *target;
    const BO::tNewtonObj *cop;
    matrixtdef targetMatrix;
    matrixtdef copMatrix;
    coorddef targetPosition;
    coorddef copPosition;
    coorddef humanPosition;
    uint8 shape[4096];
    uint8 player[256][16];
    uint8 police[2][256][8];
    uint8 weather[2][256];
    CVECTOR colors[256];
    CVECTOR additive[16];
    sint32 nearZ;
    sint32 xShift;
    sint32 zShift;
    sint32 lightningType;
    bool active;
    bool headlights;
    bool lightning;
};

struct HdSkySource
{
    const SVECTOR *mesh;
    const SVECTOR *ring;
    const SVECTOR *stars;
    const uint32 *starColors;
    sint32 starCount;
    sint32 flags;
    sint32 type;
    sint32 ringOffset;
    sint32 ringHeight;
    bool skyEnabled;
    bool ringEnabled;
    CVECTOR clearColor;
    CVECTOR frontColor;
    CVECTOR colors[85];
    CVECTOR ringColors[2][17];
    Draw::tPixMap pixmaps[16];
    Draw::tPixMap ringPixmaps[16];
    uint8 cloudIndices[64];
    uint8 ringIndices[16];
    sint32 ringPercent[16];
    SVECTOR sun;
    SVECTOR moon;
    SVECTOR lightning;
    SVECTOR sunOctagon[8];
    CVECTOR sunColor;
    Draw::tPixMap celestialPixmap;
    Draw::tPixMap lightningPixmaps[16];
    DVECTOR lightningOffsets[16];
    sint32 celestialFlags;
    sint32 lightningCount;
    sint32 lightningBrightness;
    bool celestialPixmapValid;
    bool lightningActive;
    HdLensPiece lensPieces[9];
    Draw::tPixMap lensPixmaps[2];
    bool lensPixmapValid[2];
    SVECTOR lensSpikes[4];
    SVECTOR lensOctagon[8];
    uint8 lensSpikeFrom[8];
    uint8 lensSpikeTo[8];
    CVECTOR sunBeamColor;
    sint32 lensSize;
};

enum
{
    HD_WEATHER_PARTICLES = 152,
    HD_WEATHER_SPLATS = 21
};

struct HdWeatherSplat
{
    DVECTOR position;
    sint32 startTick;
};

struct HdWeatherSource
{
    const SVECTOR *owner;
    matrixtdef rotation;
    coorddef camera;
    SVECTOR particles[HD_WEATHER_PARTICLES];
    HdWeatherSplat splats[HD_WEATHER_SPLATS];
    Draw::tPixMap pixmaps[3];
    uint32 splatMask;
    sint32 count;
    sint32 splatCount;
    sint32 processTick;
    sint32 gameTick;
    sint32 type;
    sint32 snowMap;
    float projection;
    bool active;
};

enum
{
    HD_SOUFFLE_PARTICLES = 60
};

struct HdSouffleSource
{
    VEC3F points[4];
    Draw::tPixMap pixmap;
    CVECTOR color;
    float radius;
    float angle;
    sint32 kind;
    sint32 offset;
    sint32 type;
    bool active;
};

struct HdWorldViewSettings
{
    sint32 chunk;
    sint32 track;
    float chunkFarSquared;
    float polygonFarSquared;
    float lowDetailSquared;
    float lineFarSquared;
    bool skipRoadLines;
};

struct HdViewPose
{
    HdPose pose;
    HdWorldViewSettings world;
    const BO::tNewtonObj *anchor;
    const BO::tNewtonObj *target;
    sint32 mode;
    sint32 camera;
    sint32 zoom;
    bool replay;
    sint32 animation;
    sint32 lookBehind;
    bool inCar;
    sint32 mirror;
    sint32 orderTableSize;
    float projection;
    float fogStart;
    float fogScale;
    bool fogEnabled;
    float fogStrength;
    float fogColor[3];
};

struct VID_TEXTURE;
void hd_capture_transform(const float source[3][4], HdRenderTransform *result);
void hd_mix_transform(const float from[3][4], const float to[3][4], const HdRenderTransform *a, const HdRenderTransform *b, float phase, float result[3][4]);

void hd_register_cleanup(void);
VID_TEXTURE *hd_vram_texture(void);
void hd_clear_history(void);
void hd_set_logic_cursor(uint64 tick);
void hd_reset_history(void);
void hd_reset_car(const Car::tObj *car);
void hd_reset_object(const BO::tNewtonObj *object);
void hd_reset_view(sint32 player);
void hd_capture(const DRender::tView *views, sint32 count, const float *fogStarts = 0, float fogScale = 0, bool fogEnabled = false, const HdWorldViewSettings *world = 0);
void hd_release_level(void);
void hd_capture_objects(void);
void hd_capture_cars(void);
void hd_capture_sky(void);
void hd_capture_road(void);
void hd_capture_feedback(sint32 mode, sint32 firstDepth, sint32 secondDepth, bool advance);
void hd_capture_track_halos(sint32 player);
void hd_capture_night(sint32 player, const DRender::tView *view);
void hd_reset_skids(void);
void hd_capture_skids(sint32 player, const DRender::tView *view);
void hd_capture_weather(void);
void hd_reset_weather(sint32 player = -1);
void hd_reset_weather_particle(sint32 player, sint32 slot);
void hd_capture_souffle(void);
void hd_reset_souffle(sint32 slot = -1);
void hd_move_souffle(sint32 destination, sint32 source);
bool hd_world_preview(void);
bool hd_world_material_check(void);
bool hd_car_material_check(void);
bool hd_present(float phase);
bool hd_world_raster_ready(void);
void hd_capture_pause(void);
void hd_og_presented(void);
bool hd_pause_preview(void);
bool hd_car_pose(sint32 slot, float phase, MATRIXF *matrix);
bool hd_view_pose(sint32 player, float phase, MATRIXF *matrix, HdViewPose *view);

#endif
