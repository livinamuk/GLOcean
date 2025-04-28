#version 450

layout(location = 0) in vec3 vPosition;
layout(location = 1) in vec3 n;

uniform mat4 u_projectionView;
uniform mat4 u_model;
out vec3 position;
out vec3 normal;

void main () {
    vec4 worldPos = u_model * vec4(vPosition, 1.0);
    //worldPos = vec4(vPosition, 1.0);
    gl_Position = u_projectionView * worldPos;
    position = worldPos.xyz;
    normal = n;
}
