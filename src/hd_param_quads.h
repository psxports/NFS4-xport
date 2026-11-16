#ifndef HD_PARAM_QUADS_H
#define HD_PARAM_QUADS_H

struct HdParamQuad
{
    float values[16][4];
};

struct HdParamQuads
{
    VID_GEOMETRY *mesh;
    VID_INDEX *indices;
    VID_TEXTURE *parameters;
    VID_SHADER *shader;
    VID_MATERIAL *materials[8];
    VID_MATERIAL *sources[8];
    uint32 capacity;
};

static const char paramQuadVertex[] =
    "layout(location=0)in vec4 aPos;uniform vec4 uDraw[18];uniform sampler2D uAux0;"
    "out vec2 vUV;out vec4 vColor;out float vDepth;out vec2 vFog;out vec3 vAtmospherePosition;flat out uvec4 vSkyTex;flat out ivec4 vEffectRegion;flat out ivec4 vRegion;"
    "vec4 record(int i){return texelFetch(uAux0,ivec2(i,int(aPos.x)),0);}"
    "vec3 rgb(float f){uint n=uint(f);return vec3(n&255u,(n>>8)&255u,(n>>16)&255u);}"
    "vec3 camera(vec3 p){vec4 q=vec4(p,1.0);return vec3(dot(uDraw[0],q),dot(uDraw[1],q),dot(uDraw[2],q));}"
    "void main(){int c=int(aPos.y);vec4 settings=record(12),extra=record(14);int mode=int(settings.z);float t=extra.w>=0.0?extra.w:uDraw[3].z;"
    "int pointIndex=(mode==2||mode==3)?0:c;float yScale=mode<2&&uDraw[5].y>0.5?1.0:extra.y;vec3 p=mix(record(pointIndex).xyz,record(4+pointIndex).xyz,t),local=camera(p);if(mode==5||mode==6){vec4 q=vec4(p,0.0);local=vec3(dot(uDraw[0],q),dot(uDraw[1],q),dot(uDraw[2],q));}local.y*=yScale;bool visible=settings.w>0.5;"
    "if(mode==1){vec4 shape=record(13);int kind=int(extra.x);vec3 center=camera(mix(record(0).xyz,record(4).xyz,t));center.y*=yScale;"
    "if(kind==1){float delta=shape.z-shape.x;if(delta>512.0)delta-=1024.0;if(delta< -512.0)delta+=1024.0;float angle=(shape.x+delta*t)*0.00613592315154,r=mix(shape.y,shape.w,t),s=sin(angle)*r,k=cos(angle)*r;"
    "vec2 offsets[4]=vec2[4](vec2(-s,-k),vec2(k,-s),vec2(-k,s),vec2(s,k));local=center+vec3(offsets[c].x*(uDraw[3].x<0.0?-1.0:1.0),offsets[c].y,0.0);}"
    "else if(kind==2){vec3 end=camera(mix(record(1).xyz,record(5).xyz,t));end.y*=yScale;visible=visible&&end.z>=64.0;if(abs(end.x-center.x)<32.0)end.x=center.x+(end.x>center.x?32.0:-32.0);local=(c&1)==0?center:end;local.y+=(c<2?-32.0:32.0);}"
    "else visible=visible&&camera(mix(record(1).xyz,record(5).xyz,t)).z>=32.0;}"
    "vec4 clip=vec4(local.x*uDraw[3].x,local.y*uDraw[3].y,extra.z>0.5?local.z*(65536.0/65534.0)-131070.0/65534.0:0.0,local.z);"
    "if(mode==5||mode==6){clip.z=local.z*0.9999;if(mode==6){clip.y=local.y*local.z*0.5/uDraw[4].y;clip.x+=((c&1)!=0?2.0*local.z/uDraw[4].x:0.0);clip.y-=(c>=2?2.0*local.z/uDraw[4].y:0.0);visible=visible&&local.z>0.0;}if(uDraw[5].y>0.5)clip.xy=vec2(local.x,-local.y/extra.y);}"
    "if(mode==7){vec4 source=record(13),q=vec4(source.xyz,0.0);vec3 ray=vec3(dot(uDraw[0],q),dot(uDraw[1],q),dot(uDraw[2],q));vec2 center=vec2(ray.x*uDraw[5].z/ray.z*(uDraw[3].x<0.0?-1.0:1.0),ray.y*(56320.0/65536.0)*(source.w>0.5?uDraw[5].z/ray.z:0.25));clip=vec4((center*record(c).w+record(c).xy)*2.0/uDraw[4].xy,0.0,1.0);visible=visible&&ray.z>0.0;}"
    "if(mode==2){clip.xy+=vec2((c&1)!=0?10.0*abs(uDraw[3].x):0.0,c>=2?-10.0*uDraw[3].y:0.0);visible=visible&&local.z>1.0;}"
    "if(mode==3){vec3 tail=camera(p-(record(4).xyz-record(0).xyz));vec2 end=clip.xy/local.z,old=extra.x>0.5&&tail.z>1.0?tail.xy*uDraw[3].xy/tail.z:end;vec2 start=vec2((old.x+end.x*3.0)*0.25,(old.y+end.y)*0.5);"
    "vec2 d=(end-start)*uDraw[4].xy;float n=length(d);vec2 offset=vec2(-d.y/uDraw[4].x,d.x/uDraw[4].y)/max(n,0.0001);clip=vec4((c<2?start:end)+((c&1)==0?-offset:offset),0.0,1.0);visible=visible&&local.z>1.0&&n>=0.0001;}"
    "if(mode==4){vec4 shape=record(13);float age=uDraw[4].z-shape.w,prev=max(age-1.0,0.0),a=mix(prev,age,uDraw[5].x),expand=a*0.125,size=shape.z;"
    "float x=shape.x*uDraw[4].x/320.0;vec2 pixel=vec2((c&1)==0?x-expand:x+size+expand,c<2?shape.y:shape.y+size+expand*3.0);clip=vec4(2.0*pixel.x/uDraw[4].x-1.0,1.0-2.0*pixel.y/uDraw[4].y,0.0,1.0);local=vec3(clip.xy/uDraw[17].xy,1.0);visible=visible&&age>=0.0&&age<=32.0;}"
    "gl_Position=visible?clip:vec4(2.0,2.0,2.0,1.0);uint uv=uint(record(10)[c]);vUV=vec2(uv&255u,uv>>8)+vec2(0.5);vSkyTex=uvec4(record(11));"
    "vEffectRegion=ivec4(ivec2(vSkyTex.w&255u,vSkyTex.w>>8),ivec2(settings.xy));vRegion=vEffectRegion;vColor=vec4(floor(mix(rgb(record(8)[c]),rgb(record(9)[c]),t))/255.0,1.0);"
    "if(mode==4){float age=uDraw[4].z-record(13).w;vColor.rgb=vec3(floor(128.0-mix(max(age-1.0,0.0),age,uDraw[5].x)*4.0)/255.0);}"
    "if(mode==7&&record(15).z>0.5)vColor.rgb=vec3(clamp(floor(mix(record(15).x,record(15).y,t)),0.0,255.0)/255.0);"
    "vDepth=clip.w;vFog=vec2(0.0);vAtmospherePosition=vec3(abs(uDraw[17].x)>0.000001?clip.x/uDraw[17].x:0.0,abs(uDraw[17].y)>0.000001?clip.y/uDraw[17].y:0.0,clip.w);}"
    ;

static void hd_param_free(HdParamQuads *owner)
{
    vid_mesh_free(owner->mesh);
    vid_index_free(owner->indices);
    vid_tex_free(owner->parameters);
    for (sint32 i = 0; i < 8; ++i)
        vid_mat_free(owner->materials[i]);
    vid_shader_free(owner->shader);
    memset(owner, 0, sizeof(*owner));
}

static bool hd_param_prepare(HdParamQuads *owner, uint32 capacity, const char *fragment)
{
    if (owner->mesh != 0 && owner->capacity >= capacity)
        return owner->parameters != 0 && owner->indices != 0 && owner->shader != 0;
    if (owner->mesh != 0)
        hd_param_free(owner);
    owner->capacity = capacity;
    owner->mesh = vid_mesh_create_2d(capacity * 4);
    owner->indices = vid_index_create(capacity * 6);
    owner->parameters = vid_tex_create_fmt(16, capacity, VID_RGBA32F);
    owner->shader = vid_shader_create(paramQuadVertex, fragment);
    if (owner->mesh == 0 || owner->indices == 0 || owner->parameters == 0 || owner->shader == 0)
        return false;
    VID_VERTEX2D *vertices = vid_mesh_data_2d(owner->mesh);
    uint32 *indices = vid_index_data(owner->indices);
    const uint32 order[6] = {0, 1, 2, 2, 1, 3};
    for (uint32 slot = 0; slot < capacity; ++slot)
    {
        for (uint32 corner = 0; corner < 4; ++corner)
        {
            VID_VERTEX2D *vertex = vertices + slot * 4 + corner;
            memset(vertex, 0, sizeof(*vertex));
            vertex->pos[0] = (float)slot;
            vertex->pos[1] = (float)corner;
        }
        for (uint32 i = 0; i < 6; ++i)
            indices[slot * 6 + i] = slot * 4 + order[i];
    }
    memset(vid_tex_data(owner->parameters), 0, capacity * sizeof(HdParamQuad));
    vid_mesh_commit(owner->mesh);
    vid_mesh_static(owner->mesh);
    vid_index_commit(owner->indices, capacity * 6);
    vid_index_static(owner->indices);
    vid_tex_commit(owner->parameters);
    return true;
}

static void hd_param_write(HdParamQuads *owner, uint32 slot, const HdParamQuad *value)
{
    HdParamQuad *destination = (HdParamQuad *)vid_tex_data(owner->parameters) + slot;
    if (memcmp(destination, value, sizeof(*value)) != 0)
    {
        *destination = *value;
        vid_tex_commit_rows(owner->parameters, slot, 1);
    }
}

static VID_MATERIAL *hd_param_material(HdParamQuads *owner, VID_MATERIAL *source)
{
    for (sint32 slot = 0; slot < 8; ++slot)
    {
        if (owner->sources[slot] == source)
            return owner->materials[slot];
        if (owner->sources[slot] != 0)
            continue;
        VID_MATERIAL *material = vid_mat_create();
        if (material == 0)
            return 0;
        *vid_mat_data(material) = *vid_mat_read(source);
        vid_mat_data(material)->shader = owner->shader;
        vid_mat_data(material)->aux[0] = owner->parameters;
        vid_mat_commit(material);
        owner->sources[slot] = source;
        owner->materials[slot] = material;
        return material;
    }
    return 0;
}

static float hd_param_color(CVECTOR color)
{
    return (float)(color.r | (color.g << 8) | (color.b << 16));
}

static void hd_param_texture(HdParamQuad *value, const Draw::tPixMap *pixmap, const HdTextureRegion *region = 0, uint32 flags = 1)
{
    value->values[12][3] = 1;
    value->values[14][1] = 1;
    value->values[14][3] = -1;
    if (pixmap == 0)
        return;
    value->values[10][0] = (float)(pixmap->u0 | (pixmap->v0 << 8));
    value->values[10][1] = (float)(pixmap->u1 | (pixmap->v1 << 8));
    value->values[10][2] = (float)(pixmap->u2 | (pixmap->v2 << 8));
    value->values[10][3] = (float)(pixmap->u3 | (pixmap->v3 << 8));
    value->values[11][0] = (float)pixmap->clut;
    value->values[11][1] = (float)pixmap->tpage;
    value->values[11][2] = (float)flags;
    if (region != 0)
    {
        value->values[11][3] = (float)(region->x | (region->y << 8));
        value->values[12][0] = (float)region->width;
        value->values[12][1] = (float)region->height;
    }
}

#endif
