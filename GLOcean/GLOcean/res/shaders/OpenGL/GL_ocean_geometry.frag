#version 450
#include "../common/lighting.glsl"
#include "../common/post_processing.glsl"

layout(location = 0) in vec3 WorldPos;
layout(location = 1) in vec3 Normal;
layout(binding = 3) uniform samplerCube cubeMap;

out vec4 fragOut;

uniform vec3 u_wireframeColor;
uniform vec3 u_viewPos;
uniform bool u_wireframe;

const vec3  WATER_ALBEDO = vec3(0.09, 0.12, 0.11);
const float WATER_METALLIC = 0.0;
const float WATER_ROUGHNESS = 0.02;

uniform vec3 u_fogColor = vec3(0.00326, 0.00217, 0.00073);
uniform float u_fogDensity = 0.05;

float computeFogFactorExp2(float dist) {
    return clamp(1.0 - exp(-u_fogDensity * u_fogDensity * dist * dist), 0.0, 1.0);
}

uniform vec3 u_albedo = vec3(0.01, 0.02, 0.04);
uniform float u_roughness = 0.1;
uniform float u_metallic = 0.0; // Should be 0.0 for water
uniform float u_ao = 1.0;
const vec3 u_F0_water = vec3(0.02); 
uniform float u_fogStartDistance = 10.0;
uniform float u_fogEndDistance = 100.0;
uniform float u_fogExponent = 0.5;

void main() {

    vec3 moonColor = vec3(1.0, 0.9, 0.9);

    vec3 lightDir = normalize(vec3(-0.0, 0.25, 1));
    vec3 L = normalize(lightDir);
    vec3 N = normalize(Normal);
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

    vec3 irradiance = moonColor * 0.0;
    vec3 diffuse_IBL = irradiance * albedo * ao;

    vec3 reflection_IBL    = texture(cubeMap, R).rgb;
    vec3 specular_IBL = reflection_IBL * kS * ao * 0.25;
    vec3 Lo_indirect = (kD * diffuse_IBL) + specular_IBL;
    vec3 color_linear = Lo_direct + Lo_indirect;


     float dist = length(u_viewPos - WorldPos);
    float fogRange = u_fogEndDistance - u_fogStartDistance;

    // Calculate normalized distance within the fog range (0.0 at start, 1.0 at end)
    float normDist = (dist - u_fogStartDistance) / max(fogRange, 0.0001);
    normDist = clamp(normDist, 0.0, 1.0);
    float fogEffect = pow(normDist, u_fogExponent);
    float fogFactor = 1.0 - fogEffect;

    // Apply the fog using the non-linear factor
    color_linear = mix(u_fogColor, color_linear, fogFactor);

    color_linear = Tonemap_ACES(color_linear);
    vec3 color_gamma = gammaCorrect(color_linear);

    fragOut = vec4(color_gamma, 1.0);

    // wireframe override
    if (u_wireframe) {
        fragOut.rgb = u_wireframeColor;
        fragOut.a   = 1.0;
    }
}