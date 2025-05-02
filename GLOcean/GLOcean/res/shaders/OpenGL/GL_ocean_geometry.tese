#version 450
layout(quads, equal_spacing, ccw) in;
//layout(quads, fractional_odd_spacing, ccw) in;
//layout(quads, fractional_even_spacing, ccw) in;

layout(location = 0) in highp vec3 tcPosition[];
layout(location = 0) out highp vec3 WorldPos;
layout(location = 1) out mediump vec3 Normal;
layout(location = 2) out highp vec3 DebugColor;

uniform mat4 u_model;
uniform mat4 u_projectionView;
uniform vec2 u_fftGridSize;
uniform int u_mode = 0;

layout(binding = 0) uniform sampler2D DisplacementTexture_band0;
layout(binding = 1) uniform sampler2D NormalTexture_band0;
layout(binding = 2) uniform sampler2D DisplacementTexture_band1;
layout(binding = 3) uniform sampler2D NormalTexture_band1;

void main2() {
    highp vec2 tessCoord = gl_TessCoord.xy;

    vec3 p0 = tcPosition[0];
    vec3 p1 = tcPosition[1];
    vec3 p2 = tcPosition[2];
    vec3 p3 = tcPosition[3];

    float u = gl_TessCoord.x;
    float v = gl_TessCoord.y;

    vec3 pos = mix(mix(p0, p1, u), mix(p3, p2, u), v);
    vec3 fftSpacePosition =  pos;
    
    vec2 resolution_band0 = u_fftGridSize;
    
    highp vec2 uv = fract(fftSpacePosition.xz / resolution_band0);
      
    // Displacement
    float deltaX_0 = texture(DisplacementTexture_band0, uv).x;
    float height_0 = texture(DisplacementTexture_band0, uv).y;
    float deltaZ_0 = texture(DisplacementTexture_band0, uv).z;    
    
    float deltaX = deltaX_0;
    float height = height_0;
    float deltaZ = deltaZ_0;

    // Normals
    vec3 normal_0 = texture(NormalTexture_band0, uv).rgb;
    Normal = normalize(normal_0);

    vec3 localPosition  = vec3(fftSpacePosition) + vec3(deltaX, height, deltaZ);
    
    DebugColor = vec3(uv, 0);

    WorldPos = vec4(u_model * vec4(localPosition.xyz, 1.0)).xyz;
    
    gl_Position = u_projectionView * vec4(WorldPos.xyz, 1.0);
}


void main() {
    highp vec2 tessCoord = gl_TessCoord.xy;

    vec3 p0 = tcPosition[0];
    vec3 p1 = tcPosition[1];
    vec3 p2 = tcPosition[2];
    vec3 p3 = tcPosition[3];

    float u = gl_TessCoord.x;
    float v = gl_TessCoord.y;

    vec3 pos = mix(mix(p0, p1, u), mix(p3, p2, u), v);
    
    WorldPos = vec4(u_model * vec4(pos.xyz, 1.0)).xyz;

    //const glm::uvec2 GetTesslationMeshSize() {
    //    return Ocean::GetBaseFFTResolution() / glm::uvec2(g_meshSubdivisionFactor) + glm::uvec2(1);
    //}
    
    float fftResoltion_band0 = 512.0;
    float fftResoltion_band1 = 512.0;
    float patchSize_band0 = 8.0;
    float patchSize_band1 = 13.123;

    highp vec2 uv_band0 = fract(WorldPos.xz / patchSize_band0);
    highp vec2 uv_band1 = fract(WorldPos.xz / patchSize_band1);
    
    float displacementScale_band0 = patchSize_band0 / fftResoltion_band0;
    float deltaX_band0 = textureLod(DisplacementTexture_band0, uv_band0, 0).x * displacementScale_band0;
    float height_band0 = textureLod(DisplacementTexture_band0, uv_band0, 0).y * displacementScale_band0;
    float deltaZ_band0 = textureLod(DisplacementTexture_band0, uv_band0, 0).z * displacementScale_band0;
                                
    float displacementScale_band1 = patchSize_band1 / fftResoltion_band1;
    float deltaX_band1 = texture(DisplacementTexture_band1, uv_band1).x * displacementScale_band1;
    float height_band1 = texture(DisplacementTexture_band1, uv_band1).y * displacementScale_band1;
    float deltaZ_band1 = texture(DisplacementTexture_band1, uv_band1).z * displacementScale_band1;
    
    float deltaX = deltaX_band0 + deltaX_band1;
    float height = height_band0 + height_band1;
    float deltaZ = deltaZ_band0 + deltaZ_band1;

    // Normals
    vec3 normal_0 = texture(NormalTexture_band0, uv_band0).rgb;
    vec3 normal_1 = texture(NormalTexture_band1, uv_band1).rgb;
    
    //Normal = normalize(normal_0 + normal_1);
    Normal = normalize(mix (normal_0, normal_1, 0.5));

    DebugColor = mix(vec3(uv_band0, 0), vec3(uv_band1, 0), 0.5);

    if (u_mode == 1) {
        deltaX = deltaX_band0;
        height = height_band0;
        deltaZ = deltaZ_band0;
        Normal = normal_0;
        DebugColor = vec3(uv_band0, 0);
    }
    if (u_mode == 2) {
        deltaX = deltaX_band1;
        height = height_band1;
        deltaZ = deltaZ_band1;
        Normal = normal_1;
        DebugColor = vec3(uv_band1, 0);
    }
    
    WorldPos += vec3(deltaX, height, deltaZ);
    gl_Position = u_projectionView * vec4(WorldPos.xyz, 1.0);

}
