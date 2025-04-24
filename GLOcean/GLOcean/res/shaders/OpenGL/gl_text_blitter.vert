#version 460 core

layout(location = 0) in vec2 vPosition;
layout(location = 1) in vec2 vUV;
layout(location = 2) in vec3 vColor;

out vec3 Color;
out vec2 TexCoord;

void main() {
    Color = vColor;
    TexCoord = vUV;
    gl_Position = vec4(vPosition, 0.0, 1.0);
}
