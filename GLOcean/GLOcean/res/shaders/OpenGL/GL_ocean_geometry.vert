#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 0) out vec3 vPosition;

//uniform mat4 u_model;

void main () {
    //vec4 worldPos = u_model * vec4(inPosition, 1.0);
    vPosition = inPosition;
    vPosition = inPosition * vec3(8,0,8);
    //vPosition = worldPos.xyz;
}
