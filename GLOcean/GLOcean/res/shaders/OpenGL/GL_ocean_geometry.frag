#version 450
#include "../common/lighting.glsl"
#include "../common/post_processing.glsl"

layout(location = 0) in vec3 WorldPos;
layout(location = 1) in vec3 Normal;
layout(location = 2) in highp vec3 DebugColor;

layout(binding = 0) uniform sampler2D DisplacementTexture_band0;
layout(binding = 1) uniform sampler2D NormalTexture_band0;
layout(binding = 2) uniform sampler2D DisplacementTexture_band1;
layout(binding = 3) uniform sampler2D NormalTexture_band1;
layout(binding = 4) uniform samplerCube cubeMap;
layout(binding = 5) uniform sampler2D GBufferWorldPositionTexture;

layout (location = 0) out vec4 ColorOut;
layout (location = 1) out vec4 UnderwaterMaskOut;

uniform vec3 u_wireframeColor;
uniform vec3 u_viewPos;
uniform bool u_wireframe;
uniform int u_normalMultipler;

const vec3  WATER_ALBEDO = vec3(0.09, 0.12, 0.11);
const float WATER_METALLIC = 0.0;
const float WATER_ROUGHNESS = 0.02;

uniform vec3 u_fogColor = vec3(0.00326, 0.00217, 0.00073);
uniform float u_fogDensity = 0.05;

float computeFogFactorExp2(float dist) {
    return clamp(1.0 - exp(-u_fogDensity * u_fogDensity * dist * dist), 0.0, 1.0);
}

uniform vec3 u_albedo = vec3(0.01, 0.03, 0.04);
uniform float u_roughness = 0.1;
uniform float u_metallic = 0.0; // Should be 0.0 for water
uniform float u_ao = 1.0;
const vec3 u_F0_water = vec3(0.02); 
uniform float u_fogStartDistance = 20.0;
uniform float u_fogEndDistance = 350.0;
uniform float u_fogExponent = 0.25;
uniform int u_mode;

const float u_nearMipDist = 50;   // distance at which LOD = 0
const float u_farMipDist = 100;   // distance at which LOD = max
const float u_maxMipLevel = 4.0;  // max mip count


void main() {

    float fftResoltion_band0 = 512.0;
    float fftResoltion_band1 = 512.0;
    float patchSize_band0 = 8.0;
    float patchSize_band1 = 13.123;

    highp vec2 uv_band0 = fract(WorldPos.xz / patchSize_band0);
    highp vec2 uv_band1 = fract(WorldPos.xz / patchSize_band1);
    
    float displacementScale_band0 = patchSize_band0 / fftResoltion_band0;
    float displacementScale_band1 = patchSize_band1 / fftResoltion_band1;
    
    const float gridCellsPerWorldUnit_band0 = 1.0 / displacementScale_band0;
    const float gridCellsPerWorldUnit_band1 = 1.0 / displacementScale_band1;


    float viewDist = length(WorldPos - u_viewPos);
    float t = clamp((viewDist - u_nearMipDist) / (u_farMipDist - u_nearMipDist), 0.0, 1.0);
    float lod = t * u_maxMipLevel;
    //lod = clamp(lod, 1, uMaxMipLevel);


    // Estimate band 0 position
    vec2 bestGuessUV = uv_band0;
    float dispX = texture(DisplacementTexture_band0, bestGuessUV).x;
    float dispZ = texture(DisplacementTexture_band0, bestGuessUV).z;
    vec2 estimatedDisplacement = vec2(dispX, dispZ) / gridCellsPerWorldUnit_band0;
    vec2 estimatedWorldPosition = WorldPos.xz - estimatedDisplacement;
    vec2 estimatedUV = fract(estimatedWorldPosition / patchSize_band0);
    vec3 bestGuessNormal_band0 = textureLod(NormalTexture_band0, estimatedUV, lod).xyz;

    // Estimate band 0 position
    bestGuessUV = uv_band1;
    dispX = texture(DisplacementTexture_band1, bestGuessUV).x;
    dispZ = texture(DisplacementTexture_band1, bestGuessUV).z;
    estimatedDisplacement = vec2(dispX, dispZ) / gridCellsPerWorldUnit_band1;
    estimatedWorldPosition = WorldPos.xz - estimatedDisplacement;
    estimatedUV = fract(estimatedWorldPosition / patchSize_band1);
    //vec3 bestGuessNormal_band1 = texture(NormalTexture_band1, estimatedUV).xyz;
    vec3 bestGuessNormal_band1 = textureLod(NormalTexture_band1, estimatedUV, lod).xyz;    
    //vec3 bestGuessWorldPos_band1 = textureLod(DisplacementTexture_band1, estimatedUV, 0).xyz * displacementScale_band1;
    
    vec3 normal = normalize(mix (bestGuessNormal_band0, bestGuessNormal_band1, 0.5));
    normal *= u_normalMultipler;

    // Converge to up normal over distance
    float u_normalConvergeStartDist = 50;
    float u_normalConvergeMaxDist = 150;
    float u_normalConvergeMaxFactor = 0.5;
    float u_normalConvergeExponent = 1.0;
    float t2 = clamp((viewDist - u_normalConvergeStartDist) / (u_normalConvergeMaxDist - u_normalConvergeStartDist), 0.0, 1.0);
    t2 = pow(t2, u_normalConvergeExponent);
    t2 *= u_normalConvergeMaxFactor;
    //normal = normalize(mix(normal, vec3(0.0, 1.0, 0.0), t2));



   //float etimatedHeight = mix (bestGuessNormal_band0, bestGuessNormal_band1, 0.5));

    if (u_mode == 1) {
        normal = bestGuessNormal_band0;
    }
    if (u_mode == 2) {
        normal = bestGuessNormal_band1;
    }

    //normal = Normal;

    vec3 moonColor = vec3(1.0, 0.9, 0.9);

    vec3 lightDir = normalize(vec3(0.0, 0.2 , 0.5));
    vec3 L = normalize(lightDir);
    vec3 N = normalize(normal);
    vec3 V = normalize(u_viewPos - WorldPos);
    vec3 R = reflect(-V, N);
    float NoL = clamp(dot(N, L), 0.0, 1.0);

    vec3 albedo = u_albedo;
    float metallic = u_metallic;
    float roughness = u_roughness;
    float ao = u_ao;
    vec3 F0 = u_F0_water;

    vec3 F_direct = fresnelSchlick(clamp(dot(N, V), 0.0, 1.0), F0);
    vec3 spec_direct = microfacetSpecular(L, V, N, F0, roughness);

    vec3 radiance = moonColor;
    vec3 Lo_direct = spec_direct * radiance * NoL; 

    vec3 F_indirect = fresnelSchlick(clamp(dot(N, V), 0.0, 1.0), F0);
    vec3 kS = F_indirect;             // Specular reflection fraction
    vec3 kD = (vec3(1.0) - kS);       // Diffuse reflection fraction
    kD *= (1.0 - metallic);           // Scale diffuse by non-metallic factor (has no effect here as metallic=0)

    vec3 irradiance = moonColor * 0.075;
    vec3 diffuse_IBL = irradiance * albedo * ao;

    vec3 reflection_IBL    = texture(cubeMap, R).rgb;
    vec3 specular_IBL = reflection_IBL * kS * ao * 0.5;
    vec3 Lo_indirect = (kD * diffuse_IBL) + specular_IBL;
    vec3 color_linear = Lo_direct + Lo_indirect;

    
    // Subsurface scattering
    float u_oceanLevel = -0.65;   // base water height
    float u_minHeight = -0.5;    // height at which radius=minR
    float u_maxHeight = 0.9;    // height at which radius=maxR
    float minR = 0.0;           // e.g. 0.1
    float maxR = 1.0;           // e.g. 5.0

    float h = WorldPos.y - u_oceanLevel;
    float hNorm = clamp((h - u_minHeight) / (u_maxHeight - u_minHeight), 0.0, 1.0);
    float sssRadius = mix(minR, maxR, hNorm);

    //float sssRadius = 1.0;
    float sssFactor = 1.0;

    vec3 radius = vec3(sssRadius);
    vec3 subColor = Saturate(u_albedo.rgb, 2.5);
    float NdotL = max(dot(normal.xyz, L), 0.0);
    vec3 sss = 0.2 * exp(-3.0 * abs(NdotL) / (radius + 0.001)); 
    vec3 sssColor = subColor * radius * sss * sssFactor;
    color_linear += sssColor;



    float dist = length(u_viewPos - WorldPos);
    float fogRange = u_fogEndDistance - u_fogStartDistance;

    // Calculate normalized distance within the fog range (0.0 at start, 1.0 at end)
    float normDist = (dist - u_fogStartDistance) / max(fogRange, 0.0001);
    normDist = clamp(normDist, 0.0, 1.0);
    float fogEffect = pow(normDist, u_fogExponent);
    float fogFactor = 1.0 - fogEffect;

    // Apply the fog using the non-linear factor
    //color_linear = mix(u_fogColor, color_linear, fogFactor);

    color_linear = Tonemap_ACES(color_linear);
    vec3 color_gamma = gammaCorrect(color_linear);

    ColorOut = vec4(color_gamma, 1.0);


    //fragOut = vec4(DebugColor, 1.0);

    // wireframe override
    if (u_wireframe) {
        ColorOut.rgb = u_wireframeColor;
        ColorOut.a   = 1.0;
        ColorOut = vec4(DebugColor, 1.0);
    }

    
    // Gbuffer world position
    vec2 gBufferResolution = vec2(textureSize(GBufferWorldPositionTexture, 0));
    vec2 screenspace_uv = gl_FragCoord.xy / vec2(gBufferResolution);
    vec3 gBufferWorldPosition = texture(GBufferWorldPositionTexture, screenspace_uv).xyz;

    vec3 waterSurfacePosition = WorldPos;

    UnderwaterMaskOut.r = 1.0 - step(waterSurfacePosition.y, gBufferWorldPosition.y);
    //
    //if (gBufferWorldPosition.y > waterSurfacePosition.y) {    
    //    UnderwaterMaskOut.r = 0;
    //} 
    //else {    
    //    UnderwaterMaskOut.r = 1;
    //}

    if (u_normalMultipler == -1) {
        UnderwaterMaskOut.r = 0.0;
    }


    //fragOut = vec4(DebugColor, 1.0);

    
    //fragOut.rgb = bestGuessNormal_band0;
   // fragOut.rgb = normal;
   
   //vec2 estimatedDisplacement = vec2(deltaX_band0, deltaZ_band0) / gridCellsPerWorldUnit;
   //vec2 estimatedWorldPosition = worldPosition.xz - estimatedDisplacement;
   //vec2 estimatedUV = fract(estimatedWorldPosition / patchWorldSize);
   //
   //float height = texture(DisplacementTexture, estimatedUV).y;

  // fragOut.rgb = sssColor;


  //  float height = 1 + WorldPos.y;
   // fragOut = vec4(vec3(height), 1.0);

   // fragOut = vec4(vec3(ssssssradius), 1.0);

  // fragOut = vec4(bestGuessUV2, 0, 1);


}