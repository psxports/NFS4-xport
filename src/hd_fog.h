#ifndef HD_FOG_H
#define HD_FOG_H

#define HD_FOG_BODY_GLSL \
    "float hdFogAmount(){if(uDraw[16].z<0.5||uDraw[15].w<=0.0)return 0.0;" \
    "return clamp((length(vAtmospherePosition)-uDraw[16].x)*uDraw[16].y,0.0,1.0)*uDraw[15].w;}\n" \
    "vec3 hdFog(vec3 color){float f=hdFogAmount();return uDraw[16].w>0.5?color*(1.0-f):mix(color,uDraw[15].rgb,f); }\n"

#define HD_FOG_GLSL "in vec3 vAtmospherePosition;uniform vec4 uDraw[18];\n" HD_FOG_BODY_GLSL

#endif
