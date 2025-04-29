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
