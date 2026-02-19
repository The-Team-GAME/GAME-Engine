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
uniform int   u_chromatic_aberration_samples; // Type 1: 3 or 6 | Type 2: 3-12
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

// Effect variant: 0 = Type 1, 1 = Type 2
uniform int u_effect_variant;

// ============================================================
// Helper functions
// ============================================================

// --- CRT barrel distortion ---
vec2 crt_distort(vec2 uv) {
  vec2 centered = uv - 0.5;
  float r2 = dot(centered, centered);
  centered *= 1.0 + u_crt_curvature * r2;
  return centered + 0.5;
}

// --- CRT rounded corners (Type 1 only) ---
float crt_corner_mask(vec2 uv, float radius) {
  vec2 d = abs(uv - 0.5) * 2.0;
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

// --- Noise helpers ---
float hash_noise(vec2 p) {
  return fract(sin(dot(p, vec2(12.9898, 78.233))) * 43758.5453);
}

// Type 2 rand (same formula, used differently)
float rand(vec2 co) {
  return fract(sin(dot(co, vec2(12.9898, 78.233))) * 43758.5453);
}

// --- PICO-8 palette (Type 2 quantization) ---
const vec3 pico8_palette[16] = vec3[](
  vec3(0.000, 0.000, 0.000),   //  0: black
  vec3(0.114, 0.169, 0.325),   //  1: dark blue
  vec3(0.494, 0.145, 0.325),   //  2: dark purple
  vec3(0.000, 0.529, 0.318),   //  3: dark green
  vec3(0.671, 0.322, 0.212),   //  4: brown
  vec3(0.373, 0.341, 0.310),   //  5: dark grey
  vec3(0.761, 0.765, 0.780),   //  6: light grey
  vec3(1.000, 0.945, 0.910),   //  7: white
  vec3(1.000, 0.000, 0.302),   //  8: red
  vec3(1.000, 0.639, 0.000),   //  9: orange
  vec3(1.000, 0.925, 0.153),   // 10: yellow
  vec3(0.000, 0.894, 0.212),   // 11: green
  vec3(0.161, 0.678, 1.000),   // 12: blue
  vec3(0.514, 0.463, 0.612),   // 13: indigo
  vec3(1.000, 0.467, 0.659),   // 14: pink
  vec3(1.000, 0.800, 0.667)    // 15: peach
);

vec3 match_palette(vec3 color) {
  float best_dist = 999.0;
  vec3 best_color = vec3(0.0);
  for (int i = 0; i < 16; i++) {
    float d = distance(color, pico8_palette[i]);
    if (d < best_dist) {
      best_dist = d;
      best_color = pico8_palette[i];
    }
  }
  return best_color;
}

// ============================================================
// Main
// ============================================================
void main() {
  vec2 uv = v_texCoord;

  // ============================================================
  // 1. CRT barrel distortion (UV warp) — same for both types
  // ============================================================
  if (u_crt_enabled && u_crt_curvature > 0.0) {
    uv = crt_distort(uv);
    if (uv.x < 0.0 || uv.x > 1.0 || uv.y < 0.0 || uv.y > 1.0) {
      FragColor = vec4(0.0, 0.0, 0.0, 1.0);
      return;
    }
  }

  // ============================================================
  // 2. Chromatic Aberration
  //    Type 1: Fixed 3/6-tap with hardcoded spectral weight array
  //    Type 2: Arbitrary 3-12 samples, smoothstep edge mask,
  //            continuous spectral weight formula
  // ============================================================
  vec3 color;
  if (u_chromatic_aberration_enabled) {
    vec2 dir = uv - 0.5;

    if (u_effect_variant == 0) {
      // ---- Type 1 ----
      float mask = u_chromatic_aberration_edge_only ? length(dir) * 2.0 : 1.0;
      float ca_off = u_chromatic_aberration_intensity * mask;

      if (u_chromatic_aberration_samples >= 6) {
        vec3 accum = vec3(0.0);
        const vec3 weights[6] = vec3[](
          vec3(1.0, 0.0, 0.0),
          vec3(0.5, 0.5, 0.0),
          vec3(0.0, 1.0, 0.0),
          vec3(0.0, 1.0, 0.0),
          vec3(0.0, 0.5, 0.5),
          vec3(0.0, 0.0, 1.0)
        );
        vec3 total_w = vec3(0.0);
        for (int i = 0; i < 6; i++) {
          float t = float(i) / 5.0 - 0.5;
          vec3 tap = texture(u_scene, uv + dir * ca_off * t).rgb;
          accum += tap * weights[i];
          total_w += weights[i];
        }
        color = accum / total_w;
      } else {
        float r = texture(u_scene, uv + dir * ca_off).r;
        float g = texture(u_scene, uv).g;
        float b = texture(u_scene, uv - dir * ca_off).b;
        color = vec3(r, g, b);
      }
    } else {
      // ---- Type 2 ----
      float len = length(dir);
      float mask = u_chromatic_aberration_edge_only
                     ? smoothstep(0.0, 0.8, len * 2.0) : 1.0;
      float ca_off = u_chromatic_aberration_intensity * mask;

      int samples = clamp(u_chromatic_aberration_samples, 3, 12);
      vec3 accum = vec3(0.0);
      vec3 total_w = vec3(0.0);
      for (int i = 0; i < samples; i++) {
        float t = float(i) / float(samples - 1); // 0..1
        vec2 sample_uv = uv + dir * ca_off * (t - 0.5);
        vec3 tap = texture(u_scene, sample_uv).rgb;
        // Continuous spectral weights: R peaks at t=0, G at 0.5, B at 1.0
        vec3 w = vec3(
          max(0.0, 1.0 - abs(t * 2.0 - 0.0) * 2.0),
          max(0.0, 1.0 - abs(t * 2.0 - 1.0) * 2.0),
          max(0.0, 1.0 - abs(t * 2.0 - 2.0) * 2.0)
        );
        accum += tap * w;
        total_w += w;
      }
      color = accum / max(total_w, vec3(0.001));
    }
  } else {
    color = texture(u_scene, uv).rgb;
  }

  // ============================================================
  // 3. Bloom composite — same for both types
  // ============================================================
  if (u_bloom_enabled) {
    vec3 bloom = texture(u_bloom, uv).rgb;
    bloom *= u_bloom_tint;
    bloom *= u_bloom_intensity;
    color += bloom;

    if (u_bloom_dirt_enabled) {
      vec3 dirt = texture(u_bloom_dirt, uv).rgb;
      color += bloom * dirt;
    }
  }

  // ============================================================
  // 4. Tone Mapping (ACES / Reinhard / Uncharted2)
  //    Type 1: Standard Uncharted2
  //    Type 2: Uncharted2 pre-multiplies input by 2.0
  // ============================================================
  if (u_tonemapping_enabled) {
    color *= u_tonemapping_exposure;
    if (u_tonemapping_mode == 0) {
      color = aces_tonemap(color);
    } else if (u_tonemapping_mode == 1) {
      color = reinhard_tonemap(color);
    } else {
      if (u_effect_variant == 0) {
        color = uncharted2_tonemap(color);        // Type 1
      } else {
        color = uncharted2_tonemap(color * 2.0);  // Type 2
      }
    }
  }

  // ============================================================
  // 5. Color grading (3D LUT) — same for both types
  // ============================================================
  if (u_lut_enabled) {
    float scale = (u_lut_size - 1.0) / u_lut_size;
    float lut_off = 0.5 / u_lut_size;
    vec3 lut_coord = clamp(color, 0.0, 1.0) * scale + lut_off;
    vec3 graded = texture(u_lut, lut_coord).rgb;
    color = mix(color, graded, u_lut_intensity);
  }

  // ============================================================
  // 6. Vignette
  //    Type 1: Divides centered.x by roundness
  //    Type 2: Divides centered by vec2(roundness, 1.0),
  //            guards smoothness with max(0.001)
  // ============================================================
  if (u_vignette_enabled) {
    vec2 centered = uv - 0.5;

    if (u_effect_variant == 0) {
      // ---- Type 1 ----
      centered.x /= u_vignette_roundness;
      float dist = length(centered);
      float vignette = smoothstep(u_vignette_radius,
                                  u_vignette_radius - u_vignette_smoothness, dist);
      color = mix(u_vignette_color, color, mix(1.0, vignette, u_vignette_intensity));
    } else {
      // ---- Type 2 ----
      centered /= vec2(u_vignette_roundness, 1.0);
      float dist = length(centered);
      float sm = max(u_vignette_smoothness, 0.001);
      float vignette = smoothstep(u_vignette_radius,
                                  u_vignette_radius - sm, dist);
      color = mix(u_vignette_color, color, mix(1.0, vignette, u_vignette_intensity));
    }
  }

  // ============================================================
  // 7. Color Quantization
  //    Type 1: Hardcoded palette-level approximations,
  //            dither applied per-channel (/levels vec3)
  //    Type 2: Actual PICO-8 palette matching (palette 3),
  //            dither applied as scalar (/u_quantize_levels)
  // ============================================================
  if (u_quantize_enabled) {
    if (u_effect_variant == 0) {
      // ---- Type 1 ----
      vec3 levels;
      if (u_quantize_palette == 1) {
        levels = vec3(4.0, 4.0, 3.0);
      } else if (u_quantize_palette == 2) {
        levels = vec3(4.0);
        float luma = dot(color, vec3(0.2126, 0.7152, 0.0722));
        color = vec3(luma * 0.7, luma, luma * 0.3);
      } else if (u_quantize_palette == 3) {
        levels = vec3(4.0, 4.0, 3.0);
      } else {
        levels = u_quantize_levels_rgb;
      }
      if (u_quantize_dither_enabled) {
        ivec2 pos = ivec2(gl_FragCoord.xy);
        float threshold = bayer8x8(pos);
        color += threshold / levels;
      }
      color = floor(color * levels) / levels;
    } else {
      // ---- Type 2 ----
      if (u_quantize_palette == 3) {
        // Actual PICO-8: closest-color palette matching
        if (u_quantize_dither_enabled) {
          ivec2 pos = ivec2(gl_FragCoord.xy);
          float threshold = bayer8x8(pos);
          color += threshold / u_quantize_levels;
        }
        color = match_palette(color);
      } else {
        // Other palettes: same level logic, scalar dither
        vec3 levels;
        if (u_quantize_palette == 1) {
          levels = vec3(4.0, 4.0, 3.0);
        } else if (u_quantize_palette == 2) {
          levels = vec3(4.0);
          float luma = dot(color, vec3(0.2126, 0.7152, 0.0722));
          color = vec3(luma * 0.7, luma, luma * 0.3);
        } else {
          levels = u_quantize_levels_rgb;
        }
        if (u_quantize_dither_enabled) {
          ivec2 pos = ivec2(gl_FragCoord.xy);
          float threshold = bayer8x8(pos);
          color += threshold / u_quantize_levels;
        }
        color = floor(color * levels) / levels;
      }
    }
  }

  // ============================================================
  // 8. Film Grain
  //    Type 1: UV * size * resolution noise, strong dark-area weighting
  //    Type 2: Simpler UV * size + time noise, softer luminance weighting
  // ============================================================
  if (u_film_grain_enabled) {
    float luminance = dot(color, vec3(0.2126, 0.7152, 0.0722));

    if (u_effect_variant == 0) {
      // ---- Type 1 ----
      float grain = hash_noise(uv * u_film_grain_size * u_resolution
                               + vec2(u_time * 1000.0));
      color += (grain - 0.5) * u_film_grain_intensity * (1.0 - luminance);
    } else {
      // ---- Type 2 ----
      float grain = rand(uv * u_film_grain_size + u_time);
      color += (grain - 0.5) * u_film_grain_intensity * (1.0 - luminance * 0.5);
    }
  }

  // ============================================================
  // 9. CRT Scanlines + sub-pixels + glow + corners + flicker
  //    Type 1: pi scanlines, 0.7-base subpixels, box-blur glow,
  //            SDF rounded corners, sin(60t) flicker
  //    Type 2: 2*pi scanlines, 0.5-base subpixels, brightness glow,
  //            no rounded corners, sin(20t)+rand flicker
  // ============================================================
  if (u_crt_enabled) {
    if (u_effect_variant == 0) {
      // ---- Type 1 ----
      float scanline = sin(uv.y * u_crt_scanline_count * 3.14159) * 0.5 + 0.5;
      color *= 1.0 - u_crt_scanline_intensity * (1.0 - scanline);

      if (u_crt_subpixel_enabled) {
        int px = int(gl_FragCoord.x) % 3;
        vec3 smask = vec3(0.7);
        if (px == 0)      smask.r = 1.0;
        else if (px == 1) smask.g = 1.0;
        else              smask.b = 1.0;
        color *= smask;
      }

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

      if (u_crt_corner_radius > 0.0) {
        float corner = crt_corner_mask(uv, u_crt_corner_radius);
        color *= corner;
      }

      if (u_crt_flicker_intensity > 0.0) {
        float flicker = 1.0 - u_crt_flicker_intensity * 0.5
                        * (sin(u_time * 60.0) * 0.5 + 0.5);
        color *= flicker;
      }
    } else {
      // ---- Type 2 ----
      float scanline = sin(uv.y * u_crt_scanline_count * 2.0 * 3.14159) * 0.5 + 0.5;
      color *= 1.0 - u_crt_scanline_intensity * (1.0 - scanline);

      if (u_crt_subpixel_enabled) {
        int px = int(gl_FragCoord.x * 2.0) % 3;
        vec3 smask = vec3(0.5);
        if (px == 0)      smask.r = 1.0;
        else if (px == 1) smask.g = 1.0;
        else              smask.b = 1.0;
        color *= smask;
      }

      if (u_crt_glow_enabled) {
        color *= (1.0 + u_crt_glow_intensity);
      }

      // Type 2: no rounded corners

      if (u_crt_flicker_intensity > 0.0) {
        float flicker = 1.0 - u_crt_flicker_intensity * 0.5
                        * (sin(u_time * 20.0) * 0.5 + 0.5
                           + rand(vec2(u_time)) * 0.1);
        color *= flicker;
      }
    }
  }

  // ============================================================
  // 10. Gamma correction — same for both types
  // ============================================================
  color = pow(max(color, 0.0), vec3(1.0 / u_gamma));

  FragColor = vec4(color, 1.0);
}
