#include "../common/pbr_functions.glsl"

vec3 GetDirectLighting(vec3 lightPos, vec3 lightColor, float radius, float strength, vec3 Normal, vec3 WorldPos, vec3 baseColor, float roughness, float metallic, vec3 viewPos) {
    float fresnelReflect = 1.0;
	vec3 viewDir = normalize(viewPos - WorldPos);
	float lightRadiance = strength;
	vec3 lightDir = normalize(lightPos - WorldPos);

    lightDir = normalize(vec3(-0.0, 0.25, 1));

	float lightAttenuation = smoothstep(radius, 0, length(lightPos - WorldPos));

    lightAttenuation = 1.0;

	float irradiance = max(dot(lightDir, Normal), 0.0) ;
	irradiance *= lightAttenuation * lightRadiance;
	vec3 brdf = microfacetBRDF(lightDir, viewDir, Normal, baseColor, metallic, fresnelReflect, roughness, WorldPos);
	return brdf * irradiance * clamp(lightColor, 0, 1);
}

vec3 ApplyFog(vec3 color, float dist, vec3 fogColor, float fogStart, float fogEnd) {
    float fogFactor = clamp((dist - fogStart) / (fogEnd - fogStart), 0.0, 1.0);
    return mix(color.rgb, fogColor, fogFactor);
}

float MapRange(float inValue, float minInRange, float maxInRange, float minOutRange, float maxOutRange) {
    float x = (inValue - minInRange) / (maxInRange - minInRange);
    return minOutRange + (maxOutRange - minOutRange) * x;
}