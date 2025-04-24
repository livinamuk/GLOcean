#version 460 core
#include "../common/lighting.glsl"
#include "../common/post_processing.glsl"

layout (location = 0) out vec4 FragOut;
layout (binding = 0) uniform sampler2D baseColorTexture;
layout (binding = 1) uniform sampler2D normalTexture;
layout (binding = 2) uniform sampler2D rmaTexture;

in vec2 TexCoord;
in vec3 Normal;
in vec3 Tangent;
in vec3 BiTangent;
in vec3 WorldPos;

uniform vec3 viewPos;
uniform int settings;
uniform float viewportWidth;
uniform float viewportHeight;
uniform bool isHair;
uniform float time;

void main() {
    vec4 baseColor = texture2D(baseColorTexture, TexCoord);
    vec3 normalMap = texture2D(normalTexture, TexCoord).rgb;
    vec3 rma = texture2D(rmaTexture, TexCoord).rgb;
	baseColor.rgb = pow(baseColor.rgb, vec3(2.2));

    
    mat3 tbn = mat3(normalize(Tangent), normalize(BiTangent), normalize(Normal));
    normalMap.rgb = normalMap.rgb * 2.0 - 1.0;
    normalMap = normalize(normalMap);
    vec3 normal = normalize(tbn * (normalMap));
    
    float roughness = rma.r;
    float metallic = rma.g;

    vec3 lightPosition = (vec3(7, 0, 10) * 0.5) + vec3(0, 0.125, 1);
    vec3 lightColor = vec3(1, 0.98, 0.94);
    float lightRadius = 5;
    float lightStrength = 2;
        
    lightPosition = vec3(20, 0.525, 4);
    lightStrength = 1;
    lightRadius = 10;
		
    vec3 directLighting = GetDirectLighting(lightPosition, lightColor, lightRadius, lightStrength, normal, WorldPos.xyz, baseColor.rgb, roughness, metallic, viewPos);

    float ambientIntensity = 0.000;
    vec3 ambientColor = baseColor.rgb * lightColor;
    vec3 ambientLighting = ambientColor * ambientIntensity;

    float finalAlpha = baseColor.a;
    

    vec3 finalColor = directLighting.rgb + ambientLighting;

    // Hair transluceny    
    //if (isHair) {    
	//    vec3 viewDir = normalize(viewPos - WorldPos);
    //    vec3 lightDir = normalize(lightPosition - WorldPos.xyz);
    //    vec3 halfVector = normalize(lightDir + viewDir);
    //    float diff = max(dot(normal, lightDir), 0.0);
    //    float spec = pow(max(dot(normal, halfVector), 0.0), 32.0);
    //    float backlight = max(dot(-normal, lightDir), 0.0);
    //    vec3 lightColor = vec3(1, 0.98, 0.94);
    //    float translucencyFactor = 0.01;
    //    vec3 translucency = backlight * lightColor * translucencyFactor;
    //    finalColor.rgb += translucency * baseColor.rgb; // multiplying with baseColor is a hack but looks 100x better
    //}

    // Hair frensel
    //if (isHair) {    
	//    vec3 viewDir = normalize(viewPos - WorldPos);
    //    float frenselFactor = 0.025;
    //    float fresnel = pow(1.0 - dot(normal, viewDir), 2.0);        
    //    finalColor.rgb += vec3(fresnel * frenselFactor) * baseColor.rgb; // multiplying with baseColor is a hack but looks 100x better
    //}

    // Tone mapping
	finalColor = mix(finalColor, Tonemap_ACES(finalColor), 1.0);
	finalColor = pow(finalColor, vec3(1.0/2.2));
	finalColor = mix(finalColor, Tonemap_ACES(finalColor), 0.235);

    finalColor.rgb = finalColor.rgb * finalAlpha;
    FragOut = vec4(finalColor, finalAlpha);
}
