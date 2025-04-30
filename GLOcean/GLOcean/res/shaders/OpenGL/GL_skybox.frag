#version 460
#include "../common/post_processing.glsl"

uniform samplerCube environmentMap;
in vec3 position;
out vec4 fragOut;

void main () {
    fragOut = texture(environmentMap, position);
    //fragOut.rgb = AdjustSaturation(fragOut.rgb, -0.5);
    //fragOut.rgb = AdjustLightness(fragOut.rgb, -0.75);
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