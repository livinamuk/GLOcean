#version 460
#include "../common/post_processing.glsl"

uniform vec3 u_viewPos;
layout(binding = 3) uniform samplerCube cubeMap;

in vec3 position;
in vec3 normal;

out vec4 fragOut;

uniform bool u_wireframe;
uniform vec3 u_wireframeColor;

void main () {
    vec3 lightDir = normalize(vec3(-0.5, 0.25, 1));
    
    vec3 eyeVector = normalize(position - u_viewPos);
    vec3 n = normalize(normal);
    vec3 l = normalize(lightDir);
        
    float snellRatio = 3.0 / 4.0;
    
    // Compute Fresnel term according to the formula given by Tessendorf
    float eyeCos = dot(n, -eyeVector);
    float thetaI = acos(eyeCos);
    float thetaT = asin(sin(thetaI) * snellRatio);
    
    float fresnel;
    
    if (abs(thetaI) < 1e-06) {
      fresnel = (1 - snellRatio) / (1 + snellRatio);
      fresnel = fresnel * fresnel;
    }
    else {
        float fresnel1 = sin(thetaT - thetaI) / sin(thetaT + thetaI);
        float fresnel2 = tan(thetaT - thetaI) / tan(thetaT + thetaI);
    
        fresnel = 0.5 * (fresnel1 * fresnel1 + fresnel2 * fresnel2);
    }
    
    // Compute reflected and refracted vectors
    
    float gamma = snellRatio * eyeCos;
    if (gamma < 0.0) {
        gamma += sqrt(1.0 - snellRatio * snellRatio * (1.0 - eyeCos * eyeCos));
    }
    else if (gamma > 0.0) {
        gamma -= sqrt(1.0 - snellRatio * snellRatio * (1.0 - eyeCos * eyeCos));
    }
    
    vec3 reflected = normalize(eyeVector + 2.0 * n * eyeCos);
    vec3 refracted = normalize(snellRatio * eyeVector + gamma * n);
    
    // Paint the sea
    
    //vec3 lightDir = normalize(vec3(1.0, 0.5, -1.0));
    float transparencyFactor = 0.925;
    transparencyFactor = 0.1;
    float specularFactor = clamp(dot(reflected, lightDir), 0.0, 1.0);
    specularFactor = pow(specularFactor, 100.0) * 100.0;
    
    vec3 seaColour = vec3(0.286, 0.612, 0.576) * 0.05;
    vec3 sunColour = vec3(1.0, 1.0, 0.8) * 0.19;
    vec3 seaBottomColour = vec3(texture(cubeMap, refracted));
    vec3 skyColour = vec3(texture(cubeMap, reflected));
      
    // Match the color tweaks used to render the skybox
    //skyColour.rgb = AdjustSaturation(skyColour.rgb, -0.5);
    //skyColour.rgb = AdjustLightness(skyColour.rgb, -0.75);
    
    vec3 transmittedColour = mix(seaColour, seaBottomColour, transparencyFactor);
    vec3 reflectedColor = skyColour + specularFactor * sunColour;
    
    fragOut = clamp(vec4(mix(transmittedColour, reflectedColor, fresnel), 1.0), 0.0, 1.0);
    
   // fragOut.rgb = mix(fragOut.rgb, Tonemap_ACES(fragOut.rgb), 1.0);

    vec3 spec = vec3(reflected);

    
    //float ndotl = dot(n, l);
    //fragOut.rgb = vec3(eyeCos);

    //fragOut = vec4(n, 1.0);
    //fragOut = vec4(spec, 1.0);
    //fragOut = vec4(4, 0, 0, 1);

    if (u_wireframe) {
        fragOut.rgb = u_wireframeColor;
        fragOut.rgb = vec3(0,0.5,0);
    }
    
}
