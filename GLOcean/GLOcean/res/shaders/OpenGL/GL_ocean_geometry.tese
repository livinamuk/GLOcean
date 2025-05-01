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
    float patchSize_band0 = 30.0;
    float patchSize_band1 = 8.0;

    highp vec2 uv_band0 = fract(WorldPos.xz / patchSize_band0);
    highp vec2 uv_band1 = fract(WorldPos.xz / patchSize_band1);
    
    float displacementScale_band0 = patchSize_band0 / fftResoltion_band0;
    float deltaX_band0 = texture(DisplacementTexture_band0, uv_band0).x * displacementScale_band0;
    float height_band0 = texture(DisplacementTexture_band0, uv_band0).y * displacementScale_band0;
    float deltaZ_band0 = texture(DisplacementTexture_band0, uv_band0).z * displacementScale_band0;
    
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

    DebugColor = mix(vec3(uv_band0, 0), vec3(0, uv_band1), 0.5);

    int mode = 2;
    if (mode == 1) {
        deltaX = deltaX_band0;
        height = height_band0;
        deltaZ = deltaZ_band0;
        Normal = normal_0;
        DebugColor = vec3(uv_band0, 0);
    }
    if (mode == 2) {
        deltaX = deltaX_band1;
        height = height_band1;
        deltaZ = deltaZ_band1;
        Normal = normal_1;
        DebugColor = vec3(uv_band1, 0);
    }
    
  //deltaX = 0;
  //height = 0;
  //deltaZ = 0;
    
    //DebugColor = vec3(WorldPos);

    WorldPos += vec3(deltaX, height, deltaZ);
    //DebugColor = vec3(0,1,0);


    gl_Position = u_projectionView * vec4(WorldPos.xyz, 1.0);

}









void main4444() {
    highp vec2 tessCoord = gl_TessCoord.xy;

    // Assuming tcPosition comes from the Tessellation Control Shader (TCS)
    // You might need to declare `in vec3 tcPosition[];` if not already done
    vec3 p0 = tcPosition[0];
    vec3 p1 = tcPosition[1];
    vec3 p2 = tcPosition[2];
    vec3 p3 = tcPosition[3];

    float u = tessCoord.x;
    float v = tessCoord.y;

    // 1. Interpolate position in local space using bilinear interpolation
    vec3 localPosInterpolated = mix(mix(p0, p1, u), mix(p3, p2, u), v);

    // 2. Calculate the initial (undisplaced) world position
    // This is used for calculating UVs
    vec3 initialWorldPos = vec4(u_model * vec4(localPosInterpolated.xyz, 1.0)).xyz;

    // 3. Calculate UV coordinates based on the initial world position's XZ plane
    vec2 resolution_band0 = u_fftGridSize;
    // Use fract() to keep UVs within [0, 1] for tiling textures
    highp vec2 uv = fract(initialWorldPos.xz * 0.0625);

    // 4. Fetch Displacement Data using the world-space derived UVs
    float deltaX_0 = texture(DisplacementTexture_band0, uv).x;
    float height_0 = texture(DisplacementTexture_band0, uv).y;
    float deltaZ_0 = texture(DisplacementTexture_band0, uv).z;

    // --- Combine displacement from multiple bands if needed ---
    float deltaX = deltaX_0;
    float height = height_0;
    float deltaZ = deltaZ_0;
    // --- ---

    // 5. Scale the displacement based on the model matrix scale
    // Use a uniform u_modelScale if possible, otherwise hardcode
    float modelScale = 0.0625;
    // Assuming displacement (deltaX, height, deltaZ) represents LOCAL offsets
    vec3 localDisplacement = vec3(deltaX, height, deltaZ) * modelScale;

    // 6. Add the scaled LOCAL displacement to the original interpolated LOCAL position
    vec3 finalLocalPos = localPosInterpolated + localDisplacement;

    // 7. Calculate the FINAL World Position by transforming the displaced local position
    WorldPos = vec4(u_model * vec4(finalLocalPos.xyz, 1.0)).xyz;

    // --- Normal Calculation ---
    // 8. Fetch local normal using the world-space derived UVs
    // WARNING: The normal map might expect UVs derived from local space positions.
    // If visuals are wrong, this might be why.
    vec3 localNormal_0 = texture(NormalTexture_band0, uv).rgb;

    // --- Combine normals from multiple bands if needed ---
    vec3 localNormal = normalize(localNormal_0);
    // --- ---

    // Transform the LOCAL normal to World Space
    // Method 1: Inverse Transpose (Mathematically correct for non-uniform scale/shear)
    // mat3 normalMatrix = transpose(inverse(mat3(u_model)));
    // Normal = normalize(normalMatrix * localNormal);

    // Method 2: Direct Transform (Often sufficient if u_model has only rotation and UNIFORM scale)
    // Treats the normal as a direction vector (w=0)
    Normal = normalize(vec3(u_model * vec4(localNormal, 0.0)));
    // --- ---

    // Debug Output
    DebugColor = vec3(uv, 0.0); // Visualize the calculated UVs

    // 9. Calculate final clip space position
    gl_Position = u_projectionView * vec4(WorldPos, 1.0);
}