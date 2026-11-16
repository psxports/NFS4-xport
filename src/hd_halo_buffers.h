#ifndef HD_HALO_BUFFERS_H
#define HD_HALO_BUFFERS_H

struct HdHaloBuffer
{
    VID_GEOMETRY *mesh;
    VID_TEXTURE *parameters;
    VID_TEXTURE *templates;
    VID_SHADER *shader;
    VID_MATERIAL *material;
    uint32 capacity;
};

static HdHaloBuffer *haloBufferCapture;

static const char haloBufferVertex[] =
    "layout(location=0)in vec4 aPos;uniform sampler2D uAux0;uniform sampler2D uAux1;uniform vec4 uDraw[18];"
    "out vec2 vUV;out vec4 vColor;out float vDepth;out vec2 vFog;out vec3 vAtmospherePosition;flat out uvec4 vSkyTex;flat out ivec4 vEffectRegion;"
    "vec3 rgb(float f){uint c=uint(f);return vec3(c&255u,(c>>8)&255u,(c>>16)&255u)/255.0;}"
    "void main(){int row=int(aPos.x),vertex=int(aPos.y);vec4 center=texelFetch(uAux0,ivec2(0,row),0),size=texelFetch(uAux0,ivec2(1,row),0),colors=texelFetch(uAux0,ivec2(2,row),0),settings=texelFetch(uAux0,ivec2(3,row),0);"
    "vec4 source=texelFetch(uAux1,ivec2(vertex*2,int(settings.x)),0),target=texelFetch(uAux1,ivec2(vertex*2+1,int(settings.x)),0);"
    "float s=sin(size.z),c=cos(size.z),os=sin(size.w),oc=cos(size.w);mat2 spin=mat2(c,s,-s,c),outer=mat2(oc,os,-os,oc);"
    "vec2 p=outer*((spin*source.xy)*size.xy)*center.w;bool valid=true;"
    "if(source.w!=0.0){vec2 end=outer*((spin*target.xy)*size.xy)*center.w;float n=length(end);valid=n>0.0001;p+=vec2(-end.y,end.x)*(source.w*0.5/max(n,0.0001));}"
    "p+=center.xy;float depth=center.z;gl_Position=vec4(p.x*2.0*depth/colors.z,-p.y*2.0*depth/colors.w,depth*(65536.0/65534.0)-131070.0/65534.0,depth);"
    "float bias=texelFetch(uAux0,ivec2(6,row),0).x;if(bias!=0.0)gl_Position.z=(65536.0/65534.0-(131070.0/65534.0)/max(depth+bias,1.0))*depth;"
    "if(settings.y>0.5){vec4 plane=texelFetch(uAux0,ivec2(4,row),0),projection=texelFetch(uAux0,ivec2(5,row),0);vec2 ray=gl_Position.xy/depth*projection.xy;float d=dot(plane.xyz,vec3(ray,1.0));float z=plane.w/d;if(d>0.0001&&z>0.0){gl_Position.xy*=z/depth;depth=z;gl_Position.z=z*(65536.0/65534.0)-131070.0/65534.0;gl_Position.w=z;}}"
    "if(uDraw[7].w>0.5)gl_Position.xy=gl_Position.xy*uDraw[6].xy+gl_Position.w*uDraw[6].zw;if(!valid||aPos.y>=settings.z)gl_Position=vec4(2.0,2.0,2.0,1.0);vColor=vec4(source.z==1.0?rgb(colors.x):source.z==2.0?rgb(colors.y):vec3(0.0),1.0);vUV=vec2(0.0);vSkyTex=uvec4(0u);vEffectRegion=ivec4(0);vDepth=depth;vFog=vec2(0.0);"
    "vAtmospherePosition=vec3(abs(uDraw[17].x)>0.000001?gl_Position.x/uDraw[17].x:0.0,abs(uDraw[17].y)>0.000001?gl_Position.y/uDraw[17].y:0.0,depth);}"
    ;

static void hd_halo_buffer_free(HdHaloBuffer *owner)
{
    vid_mesh_free(owner->mesh);
    vid_tex_free(owner->parameters);
    vid_tex_free(owner->templates);
    vid_mat_free(owner->material);
    vid_shader_free(owner->shader);
    memset(owner, 0, sizeof(*owner));
}

static bool hd_halo_buffer_prepare(HdHaloBuffer *owner, uint32 capacity, const float templates[4][144][4], VID_MATERIAL *base, const char *fragment)
{
    if (owner->capacity >= capacity)
        return true;
    hd_halo_buffer_free(owner);
    owner->mesh = vid_mesh_create_2d(capacity * 72);
    owner->parameters = vid_tex_create_fmt(8, capacity, VID_RGBA32F);
    owner->templates = vid_tex_create_fmt(144, 4, VID_RGBA32F);
    owner->shader = vid_shader_create(haloBufferVertex, fragment);
    owner->material = vid_mat_create();
    if (owner->mesh == 0 || owner->parameters == 0 || owner->templates == 0 || owner->shader == 0 || owner->material == 0)
        return false;
    VID_VERTEX2D *vertices = vid_mesh_data_2d(owner->mesh);
    for (uint32 slot = 0; slot < capacity; ++slot)
        for (uint32 vertex = 0; vertex < 72; ++vertex)
        {
            VID_VERTEX2D *out = vertices + slot * 72 + vertex;
            memset(out, 0, sizeof(*out));
            out->pos[0] = (float)slot;
            out->pos[1] = (float)vertex;
        }
    vid_mesh_commit(owner->mesh);
    vid_mesh_static(owner->mesh);
    memcpy(vid_tex_data(owner->templates), templates, 4 * 144 * 4 * sizeof(float));
    memset(vid_tex_data(owner->parameters), 0, capacity * 8 * 4 * sizeof(float));
    vid_tex_commit(owner->templates);
    vid_tex_commit(owner->parameters);
    *vid_mat_data(owner->material) = *vid_mat_read(base);
    vid_mat_data(owner->material)->shader = owner->shader;
    vid_mat_data(owner->material)->aux[0] = owner->parameters;
    vid_mat_data(owner->material)->aux[1] = owner->templates;
    vid_mat_commit(owner->material);
    owner->capacity = capacity;
    return true;
}

static void hd_halo_buffer_write(HdHaloBuffer *owner, uint32 slot, const float values[8][4])
{
    float *destination = (float *)vid_tex_data(owner->parameters) + slot * 32;
    if (memcmp(destination, values, 32 * sizeof(float)) != 0)
    {
        memcpy(destination, values, 32 * sizeof(float));
        vid_tex_commit_rows(owner->parameters, slot, 1);
    }
}

#endif
