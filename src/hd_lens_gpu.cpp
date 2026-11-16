#include <string.h>
#include "xport.h"
#include "vid.h"
#include "hd_lens_gpu.h"
#include "hd_sampling.h"

static VID_TARGET *lensScene;
static VID_TARGET *lensVisibility[2];
static VID_GEOMETRY *lensScreen;
static VID_SHADER *lensCopyShader;
static VID_SHADER *lensVisibilityShader;
static VID_SHADER *lensDrawShader;
static VID_SHADER *lensParamShader;
static VID_MATERIAL *lensParamMaterials[6];
static VID_MATERIAL *lensCopyMaterial;
static VID_MATERIAL *lensVisibilityMaterials[2];
static VID_MATERIAL *lensDrawMaterials[6];
static sint32 lensVisibilityIndex;
static sint32 lensVisibilityTick;
static bool lensVisibilityValid;
static bool lensVisibilityHold;
static bool lensVisibilityInitialized;
static bool lensVisibilityPending;
static sint32 lensPreviousIndex;
static sint32 lensPreviousTick;
static bool lensPreviousValid;
static bool lensPreviousHold;
static float lensVisibilityMaximum;
static float lensVisibilityOldMaximum;
static float lensPreviousMaximum;
static float lensPreviousOldMaximum;

static const char lensVisibilityFragment[] = "uniform sampler2D uAux0;\n"
                                             "void main(){float old=uParams[0].w>0.5?texelFetch(uAux0,ivec2(0),0).r:0.0;"
                                             "ivec2 size=textureSize(uTex,0);ivec2 point=clamp(ivec2(floor(uParams[0].xy*vec2(size))),ivec2(0),size-1);"
                                             "vec3 sun=texelFetch(uTex,point,0).rgb;bool white=uParams[1].x>0.5&&all(greaterThanEqual(sun,vec3(247.5/255.0)));"
                                             "float next=clamp(old+(white?uParams[0].z:-uParams[0].z),0.0,1.0);outColor=vec4(next,old,0.0,1.0);}\n";

static const char lensDrawVertex[] = "layout(location=0) in vec4 aPos;layout(location=1) in vec2 aUV;layout(location=2) in vec4 aColor;layout(location=5) in uvec4 aTex;\n"
                                     "out vec2 vUV;out vec4 vColor;out float vDepth;out vec2 vFog;flat out uvec4 vSkyTex;out float vVisibility;"
                                     "uniform vec4 uDraw[1];uniform sampler2D uAux0;\n"
                                     "void main(){vec2 history=texelFetch(uAux0,ivec2(0),0).rg;vVisibility=mix(history.y,history.x,uDraw[0].x);"
                                     "gl_Position=aPos;if((aTex.w&16384u)!=0u)gl_Position.xy=uDraw[0].yz+(aPos.xy-uDraw[0].yz)*vVisibility;"
                                     "vUV=aUV;vColor=aColor/255.0;if((aTex.w&8192u)==0u)vColor.rgb*=vVisibility;vDepth=aPos.w;vFog=vec2(0.0);vSkyTex=aTex;}\n";

static const char lensDrawFragment[] = "flat in uvec4 vSkyTex;in float vVisibility;\n" VID_PSX_SAMPLING_GLSL HD_VRAM_WORD_GLSL HD_PSX_PREMULTIPLIED_GLSL "void main(){if(vVisibility<=0.0)discard;bool composite=uParams[1].w>0.5;if(vSkyTex.z==0u){outColor=vec4(vColor.rgb,composite?0.0:1.0);return;}"
                                       "uint word=vidPsxDecode(vSkyTex.y,vSkyTex.x,ivec2(floor(vUV)));bool semi=(word&32768u)!=0u;vec4 color=vidPsxColor(word,composite?0u:uint(uParams[0].x),false);"
                                       "if(color.a==0.0)discard;vec3 rgb=(vSkyTex.z&2u)!=0u?color.rgb*vVisibility:color.rgb*vColor.rgb*(255.0/128.0);"
                                       "outColor=composite?hdPsxPremultiplied(rgb,(vSkyTex.y>>5)&3u,semi):vec4(clamp(rgb,0.0,1.0),uParams[0].y); }\n";

static const char lensParamVertex[] = "layout(location=0)in vec4 aPos;uniform vec4 uDraw[3];uniform sampler2D uAux0;uniform sampler2D uAux1;out vec2 vUV;out vec4 vColor;out float vDepth;out vec2 vFog;flat out uvec4 vSkyTex;out float vVisibility;"
    "vec4 record(int i){return texelFetch(uAux1,ivec2(i,int(aPos.x)),0);}"
    "void main(){int c=int(aPos.y);vec4 p=record(c),shape=record(13);uint flags=uint(record(14)[c]);float angle=(uDraw[2].x+uDraw[1].x*0.5+uDraw[1].y*0.5-uDraw[2].y)*shape.z*0.00153398078789;float s=sin(angle),k=cos(angle);"
    "vec2 offset=vec2(p.x*k-p.y*s,p.x*s+p.y*k)*shape.xy;float coefficient=p.w;if(shape.z!=0.0){offset*=uDraw[1].z;if((flags&16384u)!=0u)coefficient=uDraw[1].z;}"
    "gl_Position=vec4((uDraw[2].xy*coefficient+offset)*2.0/uDraw[1].xy,0.0,1.0);vec2 history=texelFetch(uAux0,ivec2(0),0).rg;vVisibility=mix(history.y,history.x,uDraw[0].x);"
    "if((flags&16384u)!=0u)gl_Position.xy=uDraw[0].yz+(gl_Position.xy-uDraw[0].yz)*vVisibility;uint color=uint(record(9)[c]);vColor=vec4(vec3(color&255u,(color>>8)&255u,(color>>16)&255u)/255.0,1.0);if((flags&8192u)==0u)vColor.rgb*=vVisibility;"
    "uint uv=uint(record(10)[c]);vUV=vec2(uv&255u,uv>>8)+vec2(0.5);vSkyTex=uvec4(record(11));vDepth=1.0;vFog=vec2(0.0);}";

void hd_lens_gpu_reset(bool hold)
{
    hd_lens_gpu_complete(false);
    lensVisibilityHold = hold;
    if (!hold)
    {
        lensVisibilityValid = false;
        lensVisibilityMaximum = lensVisibilityOldMaximum = 0;
    }
}

void hd_lens_gpu_complete(bool success)
{
    if (lensVisibilityPending && !success)
    {
        lensVisibilityIndex = lensPreviousIndex;
        lensVisibilityTick = lensPreviousTick;
        lensVisibilityValid = lensPreviousValid;
        lensVisibilityHold = lensPreviousHold;
        lensVisibilityMaximum = lensPreviousMaximum;
        lensVisibilityOldMaximum = lensPreviousOldMaximum;
    }
    lensVisibilityPending = false;
}

void hd_lens_gpu_release(void)
{
    for (sint32 pass = 0; pass < 6; ++pass)
    {
        vid_mat_free(lensDrawMaterials[pass]);
        lensDrawMaterials[pass] = 0;
        vid_mat_free(lensParamMaterials[pass]);
        lensParamMaterials[pass] = 0;
    }
    for (sint32 index = 0; index < 2; ++index)
    {
        vid_mat_free(lensVisibilityMaterials[index]);
        lensVisibilityMaterials[index] = 0;
        vid_target_free(lensVisibility[index]);
        lensVisibility[index] = 0;
    }
    vid_mat_free(lensCopyMaterial);
    vid_target_free(lensScene);
    vid_mesh_free(lensScreen);
    vid_shader_free(lensCopyShader);
    vid_shader_free(lensVisibilityShader);
    vid_shader_free(lensDrawShader);
    vid_shader_free(lensParamShader);
    lensParamShader = 0;
    lensCopyMaterial = 0;
    lensScene = 0;
    lensScreen = 0;
    lensCopyShader = lensVisibilityShader = lensDrawShader = 0;
    lensVisibilityInitialized = false;
    hd_lens_gpu_reset();
}

bool hd_lens_gpu_prepare(VID_TEXTURE *vram)
{
    if (lensScene == 0)
        lensScene = vid_target_create();
    for (sint32 index = 0; index < 2; ++index)
        if (lensVisibility[index] == 0)
            lensVisibility[index] = vid_target_create();
    if (lensScene == 0 || lensVisibility[0] == 0 || lensVisibility[1] == 0)
        return false;
    if (!lensVisibilityInitialized)
    {
        for (sint32 index = 0; index < 2; ++index)
        {
            if (!vid_begin(0, 0, 0))
                return false;
            if (!vid_target_select_offscreen_size(lensVisibility[index], 1, 1))
            {
                vid_cancel();
                return false;
            }
            if (!vid_end())
                return false;
        }
        lensVisibilityInitialized = true;
    }
    if (lensScreen == 0)
    {
        lensScreen = vid_mesh_create(6);
        if (lensScreen == 0)
            return false;
        VID_VERTEX *vertices = vid_mesh_data(lensScreen);
        const uint8 order[6] = {0, 1, 2, 2, 1, 3};
        for (sint32 i = 0; i < 6; ++i)
        {
            VID_VERTEX *vertex = vertices + i;
            memset(vertex, 0, sizeof(*vertex));
            vertex->pos[0] = (order[i] & 1) != 0 ? 1.0f : -1.0f;
            vertex->pos[1] = (order[i] & 2) != 0 ? 1.0f : -1.0f;
            vertex->pos[3] = 1;
            vertex->uv[0] = (order[i] & 1) != 0 ? 1.0f : 0.0f;
            vertex->uv[1] = (order[i] & 2) != 0 ? 1.0f : 0.0f;
            for (sint32 channel = 0; channel < 4; ++channel)
                vertex->color[channel] = 1;
        }
        vid_mesh_commit(lensScreen);
        vid_mesh_static(lensScreen);
    }
    if (lensCopyShader == 0)
        lensCopyShader = vid_shader_create(0, "void main(){outColor=texture(uTex,vUV);}");
    if (lensVisibilityShader == 0)
        lensVisibilityShader = vid_shader_create(0, lensVisibilityFragment);
    if (lensDrawShader == 0)
        lensDrawShader = vid_shader_create(lensDrawVertex, lensDrawFragment);
    if (lensCopyShader == 0 || lensVisibilityShader == 0 || lensDrawShader == 0)
        return false;
    if (lensCopyMaterial == 0)
    {
        lensCopyMaterial = vid_mat_create();
        if (lensCopyMaterial == 0)
            return false;
        VID_MATERIAL_DATA *material = vid_mat_data(lensCopyMaterial);
        material->shader = lensCopyShader;
        material->texture = vid_target_texture(lensScene);
        material->filter = VID_FILTER_NEAREST;
        vid_mat_commit(lensCopyMaterial);
    }
    for (sint32 index = 0; index < 2; ++index)
        if (lensVisibilityMaterials[index] == 0)
        {
            lensVisibilityMaterials[index] = vid_mat_create();
            if (lensVisibilityMaterials[index] == 0)
                return false;
            VID_MATERIAL_DATA *material = vid_mat_data(lensVisibilityMaterials[index]);
            material->shader = lensVisibilityShader;
            material->texture = vid_target_texture(lensScene);
            material->aux[0] = vid_target_texture(lensVisibility[1 - index]);
            material->filter = VID_FILTER_NEAREST;
            vid_mat_commit(lensVisibilityMaterials[index]);
        }
    for (sint32 pass = 0; pass < 6; ++pass)
        if (lensDrawMaterials[pass] == 0)
        {
            lensDrawMaterials[pass] = vid_mat_create();
            if (lensDrawMaterials[pass] == 0)
                return false;
            VID_MATERIAL_DATA *material = vid_mat_data(lensDrawMaterials[pass]);
            material->shader = lensDrawShader;
            material->texture = vram;
            material->params[0] = pass == 0 ? 1.0f : 2.0f;
            material->params[1] = pass == 1 ? 0.5f : pass == 4 ? 0.25f : 1.0f;
            material->blend = pass == 0 ? VID_BLEND_NONE : pass == 1 ? VID_BLEND_ALPHA : pass == 3 ? VID_BLEND_SUBTRACT : VID_BLEND_ADD;
            material->filter = VID_FILTER_NEAREST;
            if (pass == 5)
            {
                material->params[7] = 1;
                material->blend = VID_BLEND_PREMULTIPLIED;
            }
            vid_mat_commit(lensDrawMaterials[pass]);
        }
    return true;
}

VID_TARGET *hd_lens_gpu_target(void)
{
    return lensScene;
}

bool hd_lens_gpu_resolve(VID_TARGET *destination, bool offscreen, VID_RECT viewport, const float *clear, float x, float y, bool eligible, bool paused, sint32 tick)
{
    if (!vid_end())
        return false;
    if (!lensVisibilityValid || (!paused && tick != lensVisibilityTick))
    {
        sint32 index = 1 - lensVisibilityIndex;
        sint32 steps = lensVisibilityValid && tick > lensVisibilityTick ? tick - lensVisibilityTick : 1;
        if (steps > 26)
            steps = 26;
        VID_MATERIAL_DATA *material = vid_mat_data(lensVisibilityMaterials[index]);
        material->params[0] = x;
        material->params[1] = 1 - y;
        material->params[2] = steps * (10.0f / 255);
        material->params[3] = lensVisibilityValid ? 1.0f : 0.0f;
        material->params[4] = eligible && x >= 0 && x < 1 && y >= 0 && y < 1 ? 1.0f : 0.0f;
        vid_mat_commit(lensVisibilityMaterials[index]);
        if (!vid_begin(0, 0, 0))
            return false;
        if (!vid_target_select_offscreen_size(lensVisibility[index], 1, 1))
        {
            vid_cancel();
            return false;
        }
        VID_RECT pixel = {0, 0, 1, 1};
        vid_viewport(pixel);
        vid_pass(lensVisibilityMaterials[index]);
        if (!vid_submit(lensScreen, 0, 0, 6))
        {
            vid_cancel();
            return false;
        }
        if (!vid_end())
            return false;
        lensPreviousMaximum = lensVisibilityMaximum;
        lensPreviousOldMaximum = lensVisibilityOldMaximum;
        lensVisibilityOldMaximum = lensVisibilityValid ? lensVisibilityMaximum : 0;
        lensVisibilityMaximum = lensVisibilityOldMaximum + (material->params[4] > 0.5f ? material->params[2] : -material->params[2]);
        if (lensVisibilityMaximum < 0)
            lensVisibilityMaximum = 0;
        if (lensVisibilityMaximum > 1)
            lensVisibilityMaximum = 1;
        lensPreviousIndex = lensVisibilityIndex;
        lensPreviousTick = lensVisibilityTick;
        lensPreviousValid = lensVisibilityValid;
        lensPreviousHold = lensVisibilityHold;
        lensVisibilityPending = true;
        lensVisibilityIndex = index;
        lensVisibilityTick = tick;
        lensVisibilityValid = true;
        lensVisibilityHold = false;
    }
    for (sint32 pass = 0; pass < 6; ++pass)
    {
        VID_MATERIAL_DATA *material = vid_mat_data(lensDrawMaterials[pass]);
        material->aux[0] = vid_target_texture(lensVisibility[lensVisibilityIndex]);
        vid_mat_commit(lensDrawMaterials[pass]);
    }
    if (!vid_begin(clear[0], clear[1], clear[2]))
        return false;
    if (destination != 0 && !(offscreen ? vid_target_select_offscreen(destination) : vid_target_select(destination)))
    {
        vid_cancel();
        return false;
    }
    vid_viewport(viewport);
    vid_pass(lensCopyMaterial);
    if (!vid_submit(lensScreen, 0, 0, 6))
    {
        vid_cancel();
        return false;
    }
    return true;
}

void hd_lens_gpu_pass(sint32 pass, float phase, float centerX, float centerY)
{
    float uniforms[4] = {lensVisibilityHold ? 1.0f : phase, centerX, centerY, 0};
    vid_pass(lensDrawMaterials[pass]);
    vid_uniforms(uniforms, 1);
}

bool hd_lens_gpu_param_pass(sint32 pass, float phase, float x, float y, float width, float height, float ratio, VID_TEXTURE *parameters)
{
    if (lensParamShader == 0)
        lensParamShader = vid_shader_create(lensParamVertex, lensDrawFragment);
    if (lensParamShader == 0)
        return false;
    if (lensParamMaterials[pass] == 0)
        lensParamMaterials[pass] = vid_mat_create();
    if (lensParamMaterials[pass] == 0)
        return false;
    VID_MATERIAL_DATA data = *vid_mat_read(lensDrawMaterials[pass]);
    data.shader = lensParamShader;
    data.aux[1] = parameters;
    if (memcmp(vid_mat_read(lensParamMaterials[pass]), &data, sizeof(data)) != 0)
    {
        *vid_mat_data(lensParamMaterials[pass]) = data;
        vid_mat_commit(lensParamMaterials[pass]);
    }
    float uniforms[3][4] = {{lensVisibilityHold ? 1.0f : phase, x * 2 * ratio / width, y * 2 * ratio / height, 0}, {width, height, ratio, 0}, {x, y, 0, 0}};
    vid_pass(lensParamMaterials[pass]);
    vid_uniforms(&uniforms[0][0], 3);
    return true;
}

bool hd_lens_gpu_may_be_visible(float phase)
{
    if (!(phase >= 0))
        phase = 0;
    if (phase > 1 || lensVisibilityHold)
        phase = 1;
    return lensVisibilityValid && lensVisibilityOldMaximum + (lensVisibilityMaximum - lensVisibilityOldMaximum) * phase > 0;
}
