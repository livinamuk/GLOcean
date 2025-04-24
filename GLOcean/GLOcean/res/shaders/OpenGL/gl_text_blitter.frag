#version 460 core

layout (location = 0) out vec4 FragOut;
layout (binding = 0) uniform sampler2D fontTexture;

in vec3 Color;
in vec2 TexCoord;

void main() {
    FragOut = texture(fontTexture, TexCoord);
    FragOut.rgb *= Color;
}