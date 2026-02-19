#version 330 core
in vec2 v_texCoord;
out vec4 FragColor;

uniform sampler2D u_scene;        // scene color texture
uniform sampler2D u_bloom;        // bloom blur texture
uniform sampler3D u_lut;          // 3D LUT texture

uniform float u_gamma;            // gamma value (default 1.0)
uniform bool u_bloom_enabled;
uniform float u_bloom_intensity;
uniform bool u_lut_enabled;
uniform float u_lut_intensity;    // 0 = original, 1 = full LUT
uniform float u_lut_size;         // LUT dimension (e.g. 25)

// Chromatic Aberration
uniform bool  u_chromatic_aberration_enabled;
uniform float u_chromatic_aberration_intensity;

// Vignette
uniform bool  u_vignette_enabled;
uniform float u_vignette_intensity;
uniform float u_vignette_radius;

// Color Quantization
uniform bool  u_quantize_enabled;
uniform float u_quantize_levels;

// CRT Emulation
uniform bool  u_crt_enabled;
uniform float u_crt_scanline_intensity;
uniform float u_crt_curvature;
uniform float u_crt_scanline_count;
uniform vec2  u_resolution;

// --- CRT barrel distortion helper ---
vec2 crt_distort(vec2 uv) {
  vec2 centered = uv - 0.5;
  float r2 = dot(centered, centered);
  centered *= 1.0 + u_crt_curvature * r2;
  return centered + 0.5;
}

void main() {
  vec2 uv = v_texCoord;

  // --- CRT barrel distortion (applied to UVs before sampling) ---
  if (u_crt_enabled && u_crt_curvature > 0.0) {
    uv = crt_distort(uv);
    // Discard pixels outside the curved screen
    if (uv.x < 0.0 || uv.x > 1.0 || uv.y < 0.0 || uv.y > 1.0) {
      FragColor = vec4(0.0, 0.0, 0.0, 1.0);
      return;
    }
  }

  // --- Chromatic Aberration ---
  vec3 color;
  if (u_chromatic_aberration_enabled) {
    // Offset R and B channels radially from center
    vec2 dir = uv - 0.5;
    float r = texture(u_scene, uv + dir * u_chromatic_aberration_intensity).r;
    float g = texture(u_scene, uv).g;
    float b = texture(u_scene, uv - dir * u_chromatic_aberration_intensity).b;
    color = vec3(r, g, b);
  } else {
    color = texture(u_scene, uv).rgb;
  }

  // --- Bloom composite ---
  if (u_bloom_enabled) {
    vec3 bloom = texture(u_bloom, uv).rgb;
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

  // --- Vignette ---
  if (u_vignette_enabled) {
    vec2 centered = uv - 0.5;
    float dist = length(centered);
    float vignette = smoothstep(u_vignette_radius, u_vignette_radius - 0.45, dist);
    color *= mix(1.0, vignette, u_vignette_intensity);
  }

  // --- Color Quantization (posterization) ---
  if (u_quantize_enabled) {
    color = floor(color * u_quantize_levels) / u_quantize_levels;
  }

  // --- CRT Scanlines ---
  if (u_crt_enabled) {
    float scanline = sin(uv.y * u_crt_scanline_count * 3.14159) * 0.5 + 0.5;
    color *= 1.0 - u_crt_scanline_intensity * (1.0 - scanline);
  }

  // --- Gamma correction ---
  color = pow(max(color, 0.0), vec3(1.0 / u_gamma));

  FragColor = vec4(color, 1.0);
}
