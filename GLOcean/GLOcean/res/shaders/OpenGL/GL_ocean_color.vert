#version 450

layout(location = 0) in vec3 vPosition;
layout(location = 1) in vec3 n;

uniform mat4 view;
uniform mat4 projection;
uniform mat4 model;
out vec3 position;
out vec3 normal;

void main () {
    vec4 worldPos = model * vec4(vPosition, 1.0);
    gl_Position = projection * view * worldPos;
    position = worldPos.xyz;
    normal = n;
}
