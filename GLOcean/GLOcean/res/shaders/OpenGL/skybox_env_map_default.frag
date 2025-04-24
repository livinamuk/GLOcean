#version 460

#include "../common/post_processing.glsl"

uniform samplerCube environmentMap;

in vec3 position;

out vec4 color;

void main () {
    color = texture(environmentMap, position);
    color.rgb = AdjustSaturation(color.rgb, -0.5);
    color.rgb = AdjustLightness(color.rgb, -0.75);
}
