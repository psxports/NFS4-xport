#ifndef HD_CAR_LIGHTING_H
#define HD_CAR_LIGHTING_H

#include "vid_brdf.h"
#include "vid_reflection.h"

#define HD_CAR_VERTEX_OUTPUT_GLSL "smooth out vec3 vLocalNormal;smooth out vec3 vEnvironmentNormal;flat out float vReflective;flat out uvec3 vCarFlags;\n"

#define HD_CAR_NORMAL_GLSL \
    "smooth in vec3 vLocalNormal;flat in float vReflective;vec3 hdCarNormal(){vec3 n=vec3(dot(uDraw[18].xyz,vLocalNormal),dot(uDraw[19].xyz,vLocalNormal),dot(uDraw[20].xyz,vLocalNormal));" \
    "if(dot(n,n)<0.00001)n=cross(dFdx(vWorldPosition),dFdy(vWorldPosition));return n*inversesqrt(max(dot(n,n),0.00000001));}\n"

#define HD_CAR_SUN_GLSL \
    HD_CAR_NORMAL_GLSL VID_BRDF_GLSL "vec3 hdCarSun(vec3 base){if(vReflective<0.5||uDraw[27].w<=0.0)return base;vec3 n=hdCarNormal(),eye=uDraw[26].xyz-vWorldPosition;eye*=inversesqrt(max(dot(eye,eye),0.000001));" \
                                     "float nv=max(dot(n,eye),0.0001);float spec=vidBrdfSpecular(vec3(0.30,0.04,1.0),vec2(0.12,0.04),n,eye,uDraw[27].xyz);" \
                                     "return base*mix(1.0,vidBrdfTransmission(1.0,nv,0.04,0.25),uDraw[27].w)+uDraw[28].rgb*(spec*hdSunDirect*uDraw[27].w);}\n"

#define HD_CAR_ENVIRONMENT_GLSL \
    "uniform samplerCube uAux1;uniform highp samplerCube uAux2;\n" VID_CUBE_REFLECTION_GLSL VID_CUBE_PARALLAX_GLSL "vec3 hdCarEnvironment(vec3 base){if(vReflective<0.5)return base;vec3 n=hdCarNormal(),eye=uDraw[26].xyz-vWorldPosition;eye*=inversesqrt(max(dot(eye,eye),0.000001));" \
    "float fresnel=vidCubeFresnel(dot(n,eye),0.04);vec3 ray=reflect(-eye,n);" \
    "ray=vidCubeParallaxRay(uAux2,ray,vWorldPosition-uDraw[29].xyz,uDraw[30].xy,uDraw[29].w,uDraw[31]);" \
    "vec3 color=vidCubeSurface(uAux1,ray,0.22,2.0,0.5,dot(n,eye),0.04);return mix(base,color,clamp(0.18+fresnel*0.65,0.0,0.85));}\n"

#endif
