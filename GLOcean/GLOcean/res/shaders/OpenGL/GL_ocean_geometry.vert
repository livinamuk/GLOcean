#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 0) out vec3 vPosition;

uniform float u_meshSubdivisionFactor;

void main () {
    vPosition = inPosition * vec3(u_meshSubdivisionFactor, 0, u_meshSubdivisionFactor);
}
