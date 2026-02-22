#version 330 core
in vec2 v_texCoord;
out vec4 FragColor;

uniform sampler2D u_source;
uniform vec2 u_texel_size;    // 1.0 / source resolution

void main() {
  // Kawase upsample: 3x3 tent filter for smooth upsampling
  vec3 color = vec3(0.0);

  color += texture(u_source, v_texCoord + vec2(-1.0, -1.0) * u_texel_size).rgb * 1.0;
  color += texture(u_source, v_texCoord + vec2( 0.0, -1.0) * u_texel_size).rgb * 2.0;
  color += texture(u_source, v_texCoord + vec2( 1.0, -1.0) * u_texel_size).rgb * 1.0;

  color += texture(u_source, v_texCoord + vec2(-1.0,  0.0) * u_texel_size).rgb * 2.0;
  color += texture(u_source, v_texCoord                                   ).rgb * 4.0;
  color += texture(u_source, v_texCoord + vec2( 1.0,  0.0) * u_texel_size).rgb * 2.0;

  color += texture(u_source, v_texCoord + vec2(-1.0,  1.0) * u_texel_size).rgb * 1.0;
  color += texture(u_source, v_texCoord + vec2( 0.0,  1.0) * u_texel_size).rgb * 2.0;
  color += texture(u_source, v_texCoord + vec2( 1.0,  1.0) * u_texel_size).rgb * 1.0;

  color /= 16.0;

  FragColor = vec4(color, 1.0);
}
