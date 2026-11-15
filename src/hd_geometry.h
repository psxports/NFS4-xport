#ifndef HD_GEOMETRY_H
#define HD_GEOMETRY_H

#define HD_WORLD_FACING_GLSL "float hdWorldFacing(vec3 n,vec3 p,vec4 cameraInModel){return dot(n,p+cameraInModel.xyz)*cameraInModel.w;}\n"

#define HD_WORLD_CULL_GLSL \
    "flat in float vFaceDirection;\n" \
    "bool hdWorldBackFace(uint flags){return (flags&4096u)==0u&&vFaceDirection>0.0;}\n"

#endif
