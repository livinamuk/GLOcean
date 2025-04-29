#include "../common/constants.glsl"

float D_GGX(float NoH, float roughness) {
    float alpha = roughness * roughness;
    float alpha2 = alpha * alpha;
    float NoH2 = NoH * NoH;
    float b = (NoH2 * (alpha2 - 1.0) + 1.0);
    return alpha2 / (PI * b * b);
}

float G1_GGX_Schlick(float NdotV, float roughness) {
  //float r = roughness; // original
  float r = 0.5 + 0.5 * roughness; // Disney remapping
  float k = (r * r) / 2.0;
  float denom = NdotV * (1.0 - k) + k;
  return NdotV / denom;
}

float G_Smith(float NoV, float NoL, float roughness) {
  float g1_l = G1_GGX_Schlick(NoL, roughness);
  float g1_v = G1_GGX_Schlick(NoV, roughness);
  return g1_l * g1_v;
}

vec3 fresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

vec3 microfacetBRDF(in vec3 L, in vec3 V, in vec3 N, in vec3 baseColor, in float metallicness, in float fresnelReflect, in float roughness, in vec3 WorldPos) {
  // Half vector
  vec3 H = normalize(V + L);

  // Dot products
  float NoV = clamp(dot(N, V), 0.0, 1.0);
  float NoL = clamp(dot(N, L), 0.0, 1.0);
  float NoH = clamp(dot(N, H), 0.0, 1.0);
  float VoH = clamp(dot(V, H), 0.0, 1.0);
  
  // Base reflectance (F0)
  vec3 f0 = vec3(0.16 * (fresnelReflect * fresnelReflect));
  f0 = mix(f0, baseColor, metallicness);

  // Fresnel term
  vec3 F = fresnelSchlick(VoH, f0);
  
  // Specular microfacet BRDF
  float D = D_GGX(NoH, roughness);
  float G = G_Smith(NoV, NoL, roughness);
  vec3 specular = (D * G * F) / max(4.0 * NoV * NoL, 0.001);

  // Energy-conserving diffuse
  vec3 notSpecular = (1.0 - F) * (1.0 - metallicness);
  vec3 diffuse = notSpecular * baseColor / PI;

  return diffuse + specular;
}

vec3 microfacetSpecular(in vec3 L, in vec3 V, in vec3 N, in vec3 F0, in float roughness) {  
    vec3 H = normalize(L + V);  

    float NoV = clamp(dot(N, V), 0.0, 1.0);  
    float NoL = clamp(dot(N, L), 0.0, 1.0);  
    float NoH = clamp(dot(N, H), 0.0, 1.0);  
    float VoH = clamp(dot(V, H), 0.0, 1.0);  

    float D = D_GGX(NoH, roughness);  
    float G = G_Smith(NoV, NoL, roughness);  
    vec3  F = fresnelSchlick(VoH, F0);  

    return (D * G * F) / max(4.0 * NoV * NoL, 0.001);  
}