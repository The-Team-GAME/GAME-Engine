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
uniform int   u_chromatic_aberration_samples; // 3 or 6
uniform bool  u_chromatic_aberration_edge_only;

// Vignette
uniform bool  u_vignette_enabled;
uniform float u_vignette_intensity;
uniform float u_vignette_radius;
uniform vec3  u_vignette_color;
uniform float u_vignette_roundness;
uniform float u_vignette_smoothness;

// Color Quantization
uniform bool  u_quantize_enabled;
uniform float u_quantize_levels;
uniform bool  u_quantize_dither_enabled;
uniform vec3  u_quantize_levels_rgb;
uniform int   u_quantize_palette;

// CRT Emulation
uniform bool  u_crt_enabled;
uniform float u_crt_scanline_intensity;
uniform float u_crt_curvature;
uniform float u_crt_scanline_count;
uniform vec2  u_resolution;
uniform bool  u_crt_subpixel_enabled;
uniform bool  u_crt_glow_enabled;
uniform float u_crt_glow_intensity;
uniform float u_crt_corner_radius;
uniform float u_crt_flicker_intensity;

// Bloom enhancements
uniform vec3  u_bloom_tint;
uniform bool  u_bloom_dirt_enabled;
uniform sampler2D u_bloom_dirt;    // texture unit 3
uniform float u_bloom_anamorphic;

// Time (for animated effects)
uniform float u_time;

// Film Grain
uniform bool  u_film_grain_enabled;
uniform float u_film_grain_intensity;
uniform float u_film_grain_size;

// Tone Mapping
uniform bool  u_tonemapping_enabled;
uniform int   u_tonemapping_mode;
uniform float u_tonemapping_exposure;

// --- CRT barrel distortion helper ---
vec2 crt_distort(vec2 uv) {
  vec2 centered = uv - 0.5;
  float r2 = dot(centered, centered);
  centered *= 1.0 + u_crt_curvature * r2;
  return centered + 0.5;
}

// --- CRT rounded corners helper ---
float crt_corner_mask(vec2 uv, float radius) {
  vec2 d = abs(uv - 0.5) * 2.0; // 0..1 from center
  // SDF for rounded rect
  vec2 q = d - (1.0 - radius);
  float sdf = length(max(q, 0.0)) - radius;
  return smoothstep(0.01, 0.0, sdf);
}

// --- Bayer 8x8 dithering matrix ---
float bayer8x8(ivec2 pos) {
  const int bayer[64] = int[](
     0, 32,  8, 40,  2, 34, 10, 42,
    48, 16, 56, 24, 50, 18, 58, 26,
    12, 44,  4, 36, 14, 46,  6, 38,
    60, 28, 52, 20, 62, 30, 54, 22,
     3, 35, 11, 43,  1, 33,  9, 41,
    51, 19, 59, 27, 49, 17, 57, 25,
    15, 47,  7, 39, 13, 45,  5, 37,
    63, 31, 55, 23, 61, 29, 53, 21
  );
  ivec2 p = pos % 8;
  return float(bayer[p.y * 8 + p.x]) / 64.0 - 0.5;
}

// --- Tone mapping helpers ---
vec3 aces_tonemap(vec3 x) {
  return clamp((x * (2.51 * x + 0.03)) / (x * (2.43 * x + 0.59) + 0.14), 0.0, 1.0);
}

vec3 reinhard_tonemap(vec3 x) {
  return x / (x + 1.0);
}

vec3 uncharted2_partial(vec3 x) {
  float A = 0.15; float B = 0.50; float C = 0.10;
  float D = 0.20; float E = 0.02; float F = 0.30;
  return ((x*(A*x+C*B)+D*E)/(x*(A*x+B)+D*F))-E/F;
}

vec3 uncharted2_tonemap(vec3 x) {
  float W = 11.2;
  vec3 curr = uncharted2_partial(x);
  vec3 white_scale = vec3(1.0) / uncharted2_partial(vec3(W));
  return curr * white_scale;
}

// --- Hash-based noise for film grain ---
float hash_noise(vec2 p) {
  return fract(sin(dot(p, vec2(12.9898, 78.233))) * 43758.5453);
}

void main() {
  vec2 uv = v_texCoord;

  // ============================================================
  // 1. CRT barrel distortion (UV warp)
  // ============================================================
  if (u_crt_enabled && u_crt_curvature > 0.0) {
    uv = crt_distort(uv);
    // Discard pixels outside the curved screen
    if (uv.x < 0.0 || uv.x > 1.0 || uv.y < 0.0 || uv.y > 1.0) {
      FragColor = vec4(0.0, 0.0, 0.0, 1.0);
      return;
    }
  }

  // ============================================================
  // 2. Chromatic Aberration (enhanced – 6-tap + edge mask)
  // ============================================================
  vec3 color;
  if (u_chromatic_aberration_enabled) {
    vec2 dir = uv - 0.5;
    float mask = u_chromatic_aberration_edge_only ? length(dir) * 2.0 : 1.0;
    float offset = u_chromatic_aberration_intensity * mask;

    if (u_chromatic_aberration_samples >= 6) {
      // 6-tap spectral sampling
      vec3 accum = vec3(0.0);
      // Spectral weights: approximate R, G, B contribution at each tap
      // Taps go from red to blue across 6 positions
      const vec3 weights[6] = vec3[](
        vec3(1.0, 0.0, 0.0),
        vec3(0.5, 0.5, 0.0),
        vec3(0.0, 1.0, 0.0),
        vec3(0.0, 1.0, 0.0),
        vec3(0.0, 0.5, 0.5),
        vec3(0.0, 0.0, 1.0)
      );
      vec3 total_weight = vec3(0.0);
      for (int i = 0; i < 6; i++) {
        float t = float(i) / 5.0 - 0.5; // -0.5 to +0.5
        vec3 tap = texture(u_scene, uv + dir * offset * t).rgb;
        accum += tap * weights[i];
        total_weight += weights[i];
      }
      color = accum / total_weight;
    } else {
      // Original 3-tap: offset R and B channels radially from center
      float r = texture(u_scene, uv + dir * offset).r;
      float g = texture(u_scene, uv).g;
      float b = texture(u_scene, uv - dir * offset).b;
      color = vec3(r, g, b);
    }
  } else {
    color = texture(u_scene, uv).rgb;
  }

  // ============================================================
  // 3. Bloom composite (enhanced – tint + dirt overlay)
  // ============================================================
  if (u_bloom_enabled) {
    vec3 bloom = texture(u_bloom, uv).rgb;
    bloom *= u_bloom_tint;
    bloom *= u_bloom_intensity;
    color += bloom;

    // Dirt overlay: multiply bloom by dirt texture
    if (u_bloom_dirt_enabled) {
      vec3 dirt = texture(u_bloom_dirt, uv).rgb;
      color += bloom * dirt;
    }
  }

  // ============================================================
  // 4. Tone Mapping (ACES / Reinhard / Uncharted2)
  // ============================================================
  if (u_tonemapping_enabled) {
    color *= u_tonemapping_exposure;
    if (u_tonemapping_mode == 0) {
      color = aces_tonemap(color);
    } else if (u_tonemapping_mode == 1) {
      color = reinhard_tonemap(color);
    } else {
      color = uncharted2_tonemap(color);
    }
  }

  // ============================================================
  // 5. Color grading (3D LUT)
  // ============================================================
  if (u_lut_enabled) {
    // Scale UVW to sample at texel centers
    float scale = (u_lut_size - 1.0) / u_lut_size;
    float offset = 0.5 / u_lut_size;
    vec3 lut_coord = clamp(color, 0.0, 1.0) * scale + offset;
    vec3 graded = texture(u_lut, lut_coord).rgb;
    color = mix(color, graded, u_lut_intensity);
  }

  // ============================================================
  // 6. Vignette (enhanced – color + elliptical + smoothness)
  // ============================================================
  if (u_vignette_enabled) {
    vec2 centered = uv - 0.5;
    // Apply roundness: scale x to create elliptical shape
    centered.x /= u_vignette_roundness;
    float dist = length(centered);
    float vignette = smoothstep(u_vignette_radius, u_vignette_radius - u_vignette_smoothness, dist);
    // Mix toward vignette color instead of just darkening
    color = mix(u_vignette_color, color, mix(1.0, vignette, u_vignette_intensity));
  }

  // ============================================================
  // 7. Color Quantization (enhanced – dithering + per-channel + palettes)
  // ============================================================
  if (u_quantize_enabled) {
    vec3 levels;

    // Palette presets override per-channel levels
    if (u_quantize_palette == 1) {
      // NES: ~54 colors, approximately 4 levels per channel
      levels = vec3(4.0, 4.0, 3.0);
    } else if (u_quantize_palette == 2) {
      // Game Boy: 4 shades of green
      levels = vec3(4.0);
      // Convert to greenish monochrome first
      float luma = dot(color, vec3(0.2126, 0.7152, 0.0722));
      color = vec3(luma * 0.7, luma, luma * 0.3);
    } else if (u_quantize_palette == 3) {
      // PICO-8: 16 colors, approximately 3-4 levels per channel
      levels = vec3(4.0, 4.0, 3.0);
    } else {
      // Custom: use per-channel levels or global
      levels = u_quantize_levels_rgb;
    }

    // Bayer dithering before quantization
    if (u_quantize_dither_enabled) {
      ivec2 pos = ivec2(gl_FragCoord.xy);
      float threshold = bayer8x8(pos);
      color += threshold / levels;
    }

    color = floor(color * levels) / levels;
  }

  // ============================================================
  // 8. Film Grain (animated noise based on uv + time)
  // ============================================================
  if (u_film_grain_enabled) {
    float grain_noise = hash_noise(uv * u_film_grain_size * u_resolution + vec2(u_time * 1000.0));
    float luminance = dot(color, vec3(0.2126, 0.7152, 0.0722));
    // Apply more grain to darker areas (1 - luminance)
    color += (grain_noise - 0.5) * u_film_grain_intensity * (1.0 - luminance);
  }

  // ============================================================
  // 9. CRT Scanlines + sub-pixels + glow + corners + flicker
  // ============================================================
  if (u_crt_enabled) {
    // Scanlines
    float scanline = sin(uv.y * u_crt_scanline_count * 3.14159) * 0.5 + 0.5;
    color *= 1.0 - u_crt_scanline_intensity * (1.0 - scanline);

    // RGB sub-pixel mask
    if (u_crt_subpixel_enabled) {
      int px = int(gl_FragCoord.x) % 3;
      vec3 mask = vec3(0.7);
      if (px == 0)      mask.r = 1.0;
      else if (px == 1) mask.g = 1.0;
      else              mask.b = 1.0;
      color *= mask;
    }

    // Phosphor glow (simple box blur approximation)
    if (u_crt_glow_enabled) {
      vec2 texel = 1.0 / u_resolution;
      vec3 glow = vec3(0.0);
      glow += texture(u_scene, uv + vec2(-texel.x, 0.0)).rgb;
      glow += texture(u_scene, uv + vec2( texel.x, 0.0)).rgb;
      glow += texture(u_scene, uv + vec2(0.0, -texel.y)).rgb;
      glow += texture(u_scene, uv + vec2(0.0,  texel.y)).rgb;
      glow *= 0.25;
      color = mix(color, glow, u_crt_glow_intensity);
    }

    // Rounded corners
    if (u_crt_corner_radius > 0.0) {
      float corner = crt_corner_mask(uv, u_crt_corner_radius);
      color *= corner;
    }

    // Brightness flicker
    if (u_crt_flicker_intensity > 0.0) {
      float flicker = 1.0 - u_crt_flicker_intensity * 0.5 * (sin(u_time * 60.0) * 0.5 + 0.5);
      color *= flicker;
    }
  }

  // ============================================================
  // 10. Gamma correction
  // ============================================================
  color = pow(max(color, 0.0), vec3(1.0 / u_gamma));

  FragColor = vec4(color, 1.0);
}
