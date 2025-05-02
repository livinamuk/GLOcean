#version 400

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 n;

uniform mat4 view;
uniform mat4 projection;
uniform mat4 u_model;

out vec3 TexCoords;
out vec4 WorldPos;

void main () {
    TexCoords = inPosition;
    WorldPos = u_model * vec4(inPosition, 1.0);
    gl_Position = projection * view * WorldPos;
}



/*
#version 400

layout(location = 0) in vec3 inPosition;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 u_model;

out vec3 RayDir;
out vec4 WorldPos;

void main () {
// vec4 pos = projection * mat4(mat3(view)) * vec4 (inPosition, 1.0);
// gl_Position = pos.xyww;
    WorldPos = u_model * vec4(inPosition, 1.0);
  gl_Position = projection * view * WorldPos;
//
 // RayDir = inPosition;
 // WorldPos = u_model * vec4(inPosition, 1.0);
}

*/