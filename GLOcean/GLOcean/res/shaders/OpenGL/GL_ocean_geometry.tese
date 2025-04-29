#version 450
layout(quads, equal_spacing, ccw) in;
//layout(quads, fractional_odd_spacing, ccw) in;
//layout(quads, fractional_even_spacing, ccw) in;

layout(location = 0) in highp vec3 tcPosition[];
layout(location = 0) out highp vec3 WorldPos;
layout(location = 1) out mediump vec3 Normal;

uniform mat4 u_model;
uniform mat4 u_projectionView;
uniform vec2 u_fftGridSize;

layout(binding = 0) uniform sampler2D heightTex;
layout(binding = 1) uniform sampler2D dispXTex;
layout(binding = 2) uniform sampler2D dispZTex;
layout(binding = 5) uniform sampler2D normalTexture;

void main() {
    highp vec2 tessCoord = gl_TessCoord.xy;

    vec3 p0 = tcPosition[0];
    vec3 p1 = tcPosition[1];
    vec3 p2 = tcPosition[2];
    vec3 p3 = tcPosition[3];

    float u = gl_TessCoord.x;
    float v = gl_TessCoord.y;

    vec3 pos = mix(mix(p0, p1, u), mix(p3, p2, u), v);
    vec3 fftSpacePosition =  pos;

    highp vec2 uv = fract(fftSpacePosition.xz / u_fftGridSize);
   
    float deltaX = texture(dispXTex, uv).r;
    float deltaZ = texture(dispZTex, uv).r;
    float height = texture(heightTex, uv).r * 0.5;

    vec3 localPosition  = vec3(fftSpacePosition) + vec3(deltaX, height, deltaZ);
    
    WorldPos = vec4(u_model * vec4(localPosition.xyz, 1.0)).xyz;
    Normal = texture(normalTexture, uv).rgb;
    
    gl_Position = u_projectionView * vec4(WorldPos.xyz, 1.0);
}
