#version 460
#include "../common/post_processing.glsl"

layout (location = 0) out vec4 LightingOut;
layout (location = 1) out vec4 WorldPositionOut;

uniform samplerCube environmentMap;
in vec3 TexCoords;
in vec4 WorldPos;

void main () {
    LightingOut = texture(environmentMap, TexCoords);
    WorldPositionOut = vec4(WorldPos.rgb, 1.0);
    //fragOut.rgb = AdjustSaturation(fragOut.rgb, -0.5);
  //  fragOut.rgb = AdjustLightness(fragOut.rgb, -0.5);
}


/*
#version 460
#include "../common/post_processing.glsl"

uniform samplerCube environmentMap;

in vec4 WorldPos;
in vec3 RayDir;

layout (location = 0) out vec4 LightingOut;
layout (location = 1) out vec4 WorldPositionOut;

const float hugeDistance = 100;

void main () {
    LightingOut = texture(environmentMap, RayDir);
    WorldPositionOut = vec4(WorldPos.xyz, 1);
    WorldPositionOut.y += 1.1;
}
*/