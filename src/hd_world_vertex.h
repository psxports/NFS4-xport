#ifndef HD_WORLD_VERTEX_H
#define HD_WORLD_VERTEX_H

#define HD_WORLD_VERTEX_GLSL(attributes) \
    "layout(location=0) in vec4 aPos;\n" \
    "layout(location=2) in vec4 aColor;\n" \
    "layout(location=3) in vec4 aN; layout(location=4) in vec4 aExtra;\nuniform vec4 uDraw[26];\n" attributes HD_LIGHTING_VERTEX_GLSL "uniform highp usampler2D uAux0; uniform highp usampler2D uAux1;\n" \
    "out vec2 vUV; out vec4 vColor; out float vDepth; out vec2 vFog;out vec3 vAtmospherePosition;flat out float vFaceDirection;\n" \
    "flat out uvec4 vPmx; flat out uvec2 vWindow;flat out ivec4 vRegion;\n" \
    "uvec4 meta(uint index){return texelFetch(uAux0,ivec2(int(index&255u),int(index>>8)),0);}\n" \
    "uniform highp usampler2D uAux2; uniform highp usampler2D uAux3; uniform sampler2D uAux4;\n" \
    "uint nightIndex(uint light,vec3 p){vec4 v=vec4(p,1.0);\n" \
    "if(uDraw[13].w>0.5)light=texelFetch(uAux2,ivec2(int(light),int(uDraw[14].x)),0).w;if(uDraw[6].y>0.5){\n" \
    "float x=dot(uDraw[7],v),z=dot(uDraw[9],v),width=uDraw[13].y*32.0,nearZ=uDraw[13].x;\n" \
    "if(x>-width&&x<width&&z>nearZ&&z<nearZ+uDraw[13].z*64.0){\n" \
    "ivec2 cell=ivec2(floor(vec2((x+width)/uDraw[13].y,(z-nearZ)/uDraw[13].z)));\n" \
    "uint strength=texelFetch(uAux3,cell,0).x;light=texelFetch(uAux2,ivec2(int(light),int(strength)),0).x;}}\n" \
    "if(uDraw[6].z>0.5){float x=dot(uDraw[10],v),z=dot(uDraw[12],v);\n" \
    "if(z>=-2047.0&&z<2048.0&&x>=-1023.0&&x<1024.0){ivec2 cell=ivec2(floor((x+1024.0)/32.0),floor(abs(z)/32.0));\n" \
    "uint strength=texelFetch(uAux3,cell,0).x;uvec4 table=texelFetch(uAux2,ivec2(int(light),int(strength>>1)),0);light=z<0.0?table.z:table.y;}}\n" \
    "return light;}\n" HD_WORLD_FACING_GLSL \
    "\n#ifndef HD_WORLD_POSITION\n#define HD_WORLD_POSITION aPos.xyz\n#endif\n" \
    "#ifndef HD_WORLD_NORMAL\n#define HD_WORLD_NORMAL aExtra.xyz\n#endif\n" \
    "#ifndef HD_WORLD_LIGHT_POINT\n#define HD_WORLD_LIGHT_POINT aN.xyz\n#endif\n" \
    "#ifndef HD_WORLD_COLOR\n#define HD_WORLD_COLOR (aColor/255.0)\n#endif\nvoid main(){\n" \
    "vec3 position=HD_WORLD_POSITION;if(uDraw[14].z>0.5)position+=aN.xyz*(aTex.w!=0u?1.0:-1.0);vec4 p=vec4(position,1.0);\n" \
    "vec3 c=vec3(dot(uDraw[0],p),dot(uDraw[1],p),dot(uDraw[2],p));\n" \
    "vFaceDirection=(aTex.w&32768u)!=0u?hdWorldFacing(HD_WORLD_NORMAL,position,uDraw[17]):0.0;\n" \
    "uint base=aTex.x*6u; uvec4 info=meta(base+1u);\n" \
    "uint flags=info.w>>8;\n" \
    "\n" \
    "uvec4 uv=meta(base); uint packedUV=uv[int(aTex.y)];\n" \
    "vUV=vec2(float(packedUV&255u),float(packedUV>>8))+vec2(0.5);\n" \
    "if(info.x==65535u)info.x=texelFetch(uAux1,ivec2(0,int(info.z)),0).x;\n" \
    "vPmx=info;vRegion=ivec4(meta(base+2u));\n" \
    "uvec2 lo=uvec2(uv.x&255u,uv.x>>8);uvec2 hi=uvec2(uv.w&255u,uv.w>>8);\n" \
    "uvec2 size=uvec2(abs(ivec2(hi)-ivec2(lo)))+uvec2(1u);\n" \
    "vWindow=(flags&128u)!=0u?(~(size-uvec2(1u)))&uvec2(248u):uvec2(0u);\n" \
    "vColor=uDraw[5].w>0.5?vec4(uDraw[5].rgb,1.0):HD_WORLD_COLOR;\n" \
    "if(uDraw[14].y>0.5){bool flatLight=uDraw[6].x>=0.0||(aTex.w&65536u)!=0u;uint light=uDraw[6].x>=0.0?uint(uDraw[6].x):aTex.z;\n" \
    "light=nightIndex(light,uDraw[14].z>0.5?aPos.xyz-aN.xyz:flatLight?HD_WORLD_LIGHT_POINT:position);vColor=vec4(texelFetch(uAux4,ivec2(int(light),0),0).rgb,1.0);}\n" \
    "vDepth=c.z;vFog=vec2(0.0);vAtmospherePosition=c*uDraw[14].w;\n" \
    "float depthZ=max(c.z-hdWorldDepth(),0.0001);float clipZ=hdWorldDepth()>0.0?(depthZ*uDraw[3].z+uDraw[3].w)*c.z/depthZ:c.z*uDraw[3].z+uDraw[3].w;" \
    "gl_Position=vec4(c.x*uDraw[3].x,c.y*uDraw[3].y,clipZ,c.z);hdLightingVertex(position);if(uDraw[14].z>0.5&&(info.w&4u)!=0u)gl_Position=vec4(2.0,2.0,2.0,1.0);}\n"

#endif
