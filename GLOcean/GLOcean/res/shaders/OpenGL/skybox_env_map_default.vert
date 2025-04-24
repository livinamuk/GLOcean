#version 400

layout(location = 0) in vec3 vp;
layout(location = 1) in vec3 n;

uniform mat4 view;
uniform mat4 projection;

out vec3 position;

void main ()
{
  vec4 pos = projection * mat4(mat3(view)) * vec4 (vp, 1.0);
  gl_Position = pos.xyww;
  position = vp;
}
