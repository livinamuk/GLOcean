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


uniform vec3  u_fogColor = vec3(0.00, 0.10, 0.15);
uniform float u_fogDensity = 0.05;

float computeFogFactorExp2(float dist) {
    return clamp(1.0 - exp(-u_fogDensity * u_fogDensity * dist * dist), 0.0, 1.0);
}


void main() {
    vec3 N = normalize(Normal);
    vec3 V = normalize(u_viewPos - WorldPos);

    // Specular reflection
    vec3 R = reflect(-V, N);
    vec3 envColor = texture(cubeMap, R).rgb;

    // Fresnel Schlick
    float F0 = 0.02;
    float NoV = max(dot(N, V), 0.0);
    float fresnel = F0 + (1.0 - F0) * pow(1.0 - NoV, 5.0);

    // Final color: just reflection * fresnel
    vec3 color = envColor * fresnel * 1.5;

    // (later) Fog goes here

    fragOut = vec4(color, 1.0);

    
    // wireframe override
    if (u_wireframe) {
        fragOut.rgb = u_wireframeColor;
        fragOut.a   = 1.0;
    }
}

void main2() {
    vec3 n = normalize(Normal);
    vec3 viewDir = normalize(u_viewPos - WorldPos);

    // Fresnel ratio for water
    float eta = 1.00 / 1.33;

    // calculate Fresnel term (Tessendorf style)
    float cosi = clamp(dot(n, viewDir), -1.0, 1.0);
    float thetaI = acos(cosi);
    float sinT2 = eta * eta * (1.0 - cosi * cosi);
    float thetaT = asin(clamp(sqrt(sinT2), 0.0, 1.0));
    float fresnel = (abs(thetaI) < 1e-6)
        ? pow((1.0 - eta) / (1.0 + eta), 2.0)
        : 0.5 * (pow(sin(thetaT - thetaI) / sin(thetaT + thetaI), 2.0)
               + pow(tan(thetaT - thetaI) / tan(thetaT + thetaI), 2.0));

    // reflection / refraction dirs
    vec3 reflectDir = reflect(-viewDir, n);
    vec3 refractDir = refract(-viewDir, n, eta);

    // sample environment
    vec3 envColor    = texture(cubeMap, reflectDir).rgb;
    vec3 bottomColor = texture(cubeMap, refractDir).rgb;

    // compute F0 from dielectric
    vec3 F0 = mix(vec3(0.04), WATER_ALBEDO, WATER_METALLIC);

    // PBR specular
    vec3 specBRDF = microfacetSpecular(reflectDir, viewDir, n, F0, WATER_ROUGHNESS);
    //vec3 specularEnv = envColor * specBRDF;

    float NoV       = clamp(dot(n, viewDir), 0.0, 1.0);
vec3  F         = fresnelSchlick(NoV, F0);
vec3 specularEnv= envColor * F;

    // simple transmission blend
    vec3 waterTint = WATER_ALBEDO * 0.1;
    float transFactor = 0.9;
    vec3 transmitted = mix(waterTint, bottomColor, transFactor);

    transmitted = bottomColor;

    // mix by Fresnel
    vec3 color = mix(transmitted, specularEnv, fresnel);

    float dist = length(u_viewPos - WorldPos);
    float fogFact = computeFogFactorExp2(dist);
    color = mix(color, u_fogColor, fogFact);

    // tone map
    color = Tonemap_ACES(color);
    
    fragOut = vec4(color, 1.0);
   // fragOut = vec4(specBRDF, 1.0);

    // wireframe override
    if (u_wireframe) {
        fragOut.rgb = u_wireframeColor;
        fragOut.a   = 1.0;
    }


    fragOut = clamp(fragOut, 0.0, 1.0);
}
