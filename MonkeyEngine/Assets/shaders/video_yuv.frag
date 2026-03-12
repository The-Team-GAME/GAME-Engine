#version 330 core
in vec2 v_texCoord;
out vec4 FragColor;

uniform sampler2D u_tex_y;
uniform sampler2D u_tex_cb;
uniform sampler2D u_tex_cr;

// BT.601 YCbCr -> RGB conversion matrix (from pl_mpeg documentation)
const mat4 bt601 = mat4(
  1.16438,  0.00000,  1.59603, -0.87079,
  1.16438, -0.39176, -0.81297,  0.52959,
  1.16438,  2.01723,  0.00000, -1.08139,
  0.0,      0.0,      0.0,      1.0
);

void main()
{
  float y  = texture(u_tex_y,  v_texCoord).r;
  float cb = texture(u_tex_cb, v_texCoord).r;
  float cr = texture(u_tex_cr, v_texCoord).r;

  FragColor = vec4(y, cb, cr, 1.0) * bt601;
}
