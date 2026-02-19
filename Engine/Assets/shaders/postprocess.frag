#version 330 core
in vec2 v_texCoord;
out vec4 FragColor;

uniform sampler2D u_scene;        // scene color texture
uniform sampler2D u_bloom;        // bloom blur texture
uniform sampler3D u_lut;          // 3D LUT texture

uniform float u_gamma;            // gamma value (default 2.2)
uniform bool u_bloom_enabled;
uniform float u_bloom_intensity;
uniform bool u_lut_enabled;
uniform float u_lut_intensity;    // 0 = original, 1 = full LUT
uniform float u_lut_size;         // LUT dimension (e.g. 25)

void main() {
  vec3 color = texture(u_scene, v_texCoord).rgb;

  // --- Bloom composite ---
  if (u_bloom_enabled) {
    vec3 bloom = texture(u_bloom, v_texCoord).rgb;
    color += bloom * u_bloom_intensity;
  }

  // --- Color grading (3D LUT) ---
  if (u_lut_enabled) {
    // Scale UVW to sample at texel centers
    float scale = (u_lut_size - 1.0) / u_lut_size;
    float offset = 0.5 / u_lut_size;
    vec3 lut_coord = clamp(color, 0.0, 1.0) * scale + offset;
    vec3 graded = texture(u_lut, lut_coord).rgb;
    color = mix(color, graded, u_lut_intensity);
  }

  // --- Gamma correction ---
  color = pow(max(color, 0.0), vec3(1.0 / u_gamma));

  FragColor = vec4(color, 1.0);
}
