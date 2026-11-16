#ifndef HD_LIGHTING_H
#define HD_LIGHTING_H

#include "xport.h"
#include "psx.h"
#include "vid.h"
#include "vid_shadow.h"

enum
{
    HD_LIGHTING_FIRST_TRACK = 6,
    HD_LIGHTING_TRACKS = 12,
    HD_LIGHTING_MAP_SIZE = 1024,
    HD_LIGHTING_SUN_CASCADES = 2,
    HD_LIGHTING_HEAD_LAYER = HD_LIGHTING_SUN_CASCADES * 2,
    HD_LIGHTING_LAMPS = 18,
    HD_LIGHTING_LAYERS = HD_LIGHTING_HEAD_LAYER + HD_LIGHTING_LAMPS,
    HD_LIGHTING_RECORDS = 16 + HD_LIGHTING_LAMPS * 8
};

extern uint32 hd_lighting_captures;
extern uint32 hd_lighting_caster_draws;
extern float hd_headlight_intensity;
extern float hd_shadow_strength;
extern float hd_shadow_vertical_bias[HD_LIGHTING_TRACKS];
extern float hd_sun_height_bias[HD_LIGHTING_TRACKS];

struct HdLightingProjection
{
    float rows[4][4];
    VEC3F center;
    float radius;
    sint32 owner;
    uint32 layer;
    bool active;
    const HdLightingProjection *covered;
};

struct HdLightingLamp
{
    VEC3F position;
    VEC3F direction;
    float range;
    float color[3];
    sint32 owner;
    bool active;
};

typedef bool (*HdLightingCaster)(void *context, const HdLightingProjection *projection);

VEC3F hd_lighting_shadow_direction(const VEC3F *sun, sint32 track);
void hd_lighting_raise_sun(SVECTOR *sun, sint32 track);
bool hd_lighting_resources(void);
void hd_lighting_free(void);
void hd_lighting_frame(sint32 views = 1);
void hd_lighting_sun(sint32 view, const MATRIXF *camera, const VEC3F *sun, bool enabled, float distance = 16384, float sunWeight = 1);
void hd_lighting_lamp(sint32 lamp, const HdLightingLamp *source);
bool hd_lighting_capture(HdLightingCaster draw, void *context);
bool hd_lighting_ready(void);
VID_TEXTURE *hd_lighting_depth(void);
VID_TEXTURE *hd_lighting_data(void);
void hd_lighting_uniforms(float uniforms[26][4], const MATRIXF *model, sint32 view, bool enabled);
void hd_lighting_orthographic(HdLightingProjection *projection, const VEC3F *center, const VEC3F *sun, float radius, bool snap = true, const VEC3F *right = 0);
bool hd_lighting_spot(HdLightingProjection *projection, const HdLightingLamp *lamp);

#define HD_LIGHTING_VERTEX_GLSL \
    "out vec3 vWorldPosition;void hdLightingVertex(vec3 p){vec4 q=vec4(p,1.0);" \
    "vWorldPosition=vec3(dot(uDraw[18],q),dot(uDraw[19],q),dot(uDraw[20],q));" \
    "if(uDraw[21].w>0.5){q=vec4(vWorldPosition,1.0);gl_Position=vec4(dot(uDraw[22],q),dot(uDraw[23],q),dot(uDraw[24],q),dot(uDraw[25],q));}}\n"

#define HD_LIGHTING_CASCADE_GLSL "float hdLightCascadeBlend(vec4 clip){vec3 p=abs(clip.xyz)/max(abs(clip.w),0.000001);return smoothstep(0.8,0.96,max(max(p.x,p.y),p.z));}\n"

#define HD_LIGHTING_FRAGMENT_GLSL \
    "in vec3 vWorldPosition;uniform highp sampler2DArrayShadow uAux5;uniform highp sampler2D uAux6;\n" VID_SHADOW_SAMPLING_GLSL HD_LIGHTING_CASCADE_GLSL "vec4 hdLightRecord(int index){return texelFetch(uAux6,ivec2(index,int(uDraw[21].y)),0);}" \
    "vec4 hdLightProject(int index){vec4 p=vec4(vWorldPosition,1.0);return vec4(dot(hdLightRecord(index),p),dot(hdLightRecord(index+1),p),dot(hdLightRecord(index+2),p),dot(hdLightRecord(index+3),p));}" \
    "vec3 hdLightCoord(vec4 clip){return clip.xyz/max(abs(clip.w),0.000001)*0.5+0.5;}" \
    "float hdLightSample(vec4 clip,float layer,vec3 p,vec2 gradient){" \
    "if(clip.w<=0.0||p.z<0.0||p.z>1.0||any(lessThan(p.xy,vec2(0.002)))||any(greaterThan(p.xy,vec2(0.998))))return 1.0;" \
    "p.z-=min(0.00002,2.0/max(clip.w*clip.w,1.0));return vidShadowTent3Array(uAux5,p,layer,vec2(1.0/1024.0),clamp(gradient,vec2(-0.1),vec2(0.1)));}" \
    "float hdLightVisibility(vec4 clip,float layer){vec3 p=hdLightCoord(clip);return hdLightSample(clip,layer,p,vidShadowGradient(dFdx(p),dFdy(p)));}" \
    "float hdSunVisibility=1.0,hdSunDirect=1.0;vec3 hdLampEnergy=vec3(0.0);void hdLightingPrepareNormal(vec3 normal){" \
    "float n2=dot(normal,normal);normal*=inversesqrt(max(n2,0.00000001));" \
    "if(uDraw[21].x<0.5)return;" \
    "vec4 settings=hdLightRecord(12);if(settings.x>0.0&&uDraw[21].z<0.5){vec4 nearClip=hdLightProject(0),farClip=hdLightProject(4);" \
    "vec3 nearP=hdLightCoord(nearClip),farP=hdLightCoord(farClip);" \
    "vec2 nearG=vidShadowGradient(dFdx(nearP),dFdy(nearP)),farG=vidShadowGradient(dFdx(farP),dFdy(farP));" \
    "float blend=hdLightCascadeBlend(nearClip),nearVisibility=1.0,farVisibility=1.0;" \
    "if(blend<1.0)nearVisibility=hdLightSample(nearClip,settings.y,nearP,nearG);" \
    "if(blend>0.0)farVisibility=hdLightSample(farClip,settings.z,farP,farG);" \
    "float visibility=mix(nearVisibility,farVisibility,blend);" \
    "hdSunDirect=visibility;hdSunVisibility=1.0-settings.x*(1.0-visibility);}vec3 energy=vec3(0.0);" \
    "for(int lamp=0;lamp<18;++lamp){if(lamp>=int(settings.w))break;int index=16+lamp*8;vec4 clip=hdLightProject(index);" \
    "vec4 origin=hdLightRecord(index+4),axis=hdLightRecord(index+5),color=hdLightRecord(index+6);" \
    "if(axis.w<0.5)continue;" \
    "vec3 lampP=hdLightCoord(clip);vec2 lampG=vidShadowGradient(dFdx(lampP),dFdy(lampP));vec3 delta=vWorldPosition-origin.xyz;float distance2=dot(delta,delta);" \
    "float distance=sqrt(max(distance2,0.000001));vec3 ray=delta/distance;" \
    "float cone=smoothstep(0.82,0.94,dot(ray,axis.xyz))*(1.0-smoothstep(0.75,1.0,max(abs(clip.x),abs(clip.y))/max(clip.w,0.000001)));float fade=1.0-smoothstep(origin.w*0.75,origin.w,distance);" \
    "float diffuse=abs(dot(normal,-ray));float attenuation=1.0/(1.0+distance2/max(origin.w*origin.w*0.025,1.0));" \
    "float visible=1.0;if(cone*fade*diffuse>0.0&&uDraw[21].z<0.5)visible=hdLightSample(clip,color.w,lampP,lampG);" \
    "energy+=color.rgb*cone*fade*diffuse*attenuation*visible*axis.w;}hdLampEnergy=energy;}" \
    "void hdLightingPrepare(){hdLightingPrepareNormal(cross(dFdx(vWorldPosition),dFdy(vWorldPosition)));}vec3 hdLighting(vec3 base,vec3 albedo){base*=hdSunVisibility;return base+albedo*(1.0-exp(-hdLampEnergy))*max(vec3(0.0),vec3(1.0)-base);}\n"

#endif
