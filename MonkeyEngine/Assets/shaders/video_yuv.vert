#version 330 core
layout (location = 0) in vec2 a_position;
layout (location = 1) in vec2 a_texCoord;

out vec2 v_texCoord;

uniform vec2 u_pos;   // center position in NDC (-1..1)
uniform vec2 u_size;  // half-size in NDC

void main()
{
  vec2 world = u_pos + a_position * u_size;
  gl_Position = vec4(world, 0.0, 1.0);
  v_texCoord = a_texCoord;
}
