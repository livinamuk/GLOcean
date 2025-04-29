#version 450
layout(vertices = 4) out;

layout(location = 0) in vec3 vPosition[];
layout(location = 0) out vec3 tcPosition[];

uniform vec3 u_viewPos;
uniform mat4 u_model;

const float maxTessLevel = 8.0;
const float minTessLevel = 1.0;
const float startFadeDist = 2.0;
const float endFadeDist = 30.0;
const float minPossibleLevel = 1.0;
const float maxPossibleLevel = 64.0;

float CalculateTessLevel(float dist) {
    float blendFactor = clamp((dist - startFadeDist) / (endFadeDist - startFadeDist), 0.0, 1.0);
    float tessLevel = mix(maxTessLevel, minTessLevel, blendFactor);
    return clamp(tessLevel, minPossibleLevel, maxPossibleLevel);
}

void main() {
    tcPosition[gl_InvocationID] = vPosition[gl_InvocationID];

    if (gl_InvocationID == 0) {
        vec3 worldPos0 = (u_model * vec4(vPosition[0], 1.0)).xyz;
        vec3 worldPos1 = (u_model * vec4(vPosition[1], 1.0)).xyz;
        vec3 worldPos2 = (u_model * vec4(vPosition[2], 1.0)).xyz;
        vec3 worldPos3 = (u_model * vec4(vPosition[3], 1.0)).xyz;

        float dist0 = distance(u_viewPos, worldPos0);
        float dist1 = distance(u_viewPos, worldPos1);
        float dist2 = distance(u_viewPos, worldPos2);
        float dist3 = distance(u_viewPos, worldPos3);

        float level0 = CalculateTessLevel(dist0);
        float level1 = CalculateTessLevel(dist1);
        float level2 = CalculateTessLevel(dist2);
        float level3 = CalculateTessLevel(dist3);

        gl_TessLevelOuter[0] = max(level0, level1);
        gl_TessLevelOuter[1] = max(level1, level2);
        gl_TessLevelOuter[2] = max(level2, level3);
        gl_TessLevelOuter[3] = max(level3, level0);

        gl_TessLevelInner[0] = max(gl_TessLevelOuter[1], gl_TessLevelOuter[3]);
        gl_TessLevelInner[1] = max(gl_TessLevelOuter[0], gl_TessLevelOuter[2]);
    }
}