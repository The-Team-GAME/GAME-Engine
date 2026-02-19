//-----------------------------------------------------------------------------------
// file:    PostProcessing.h
// project: Monkey Stole My GPA
//
// brief:   Modular post-processing pipeline for the graphics renderer. Provides
//          FBO-based scene capture, Kawase bloom, 3D LUT color grading, and gamma
//          correction. Designed for easy extension with future effects (vignette,
//          chromatic aberration, CRT emulation, color quantization).
//
// Copyright  2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include <string>
#include <vector>
#include "../Precompiled.h"

namespace Engine::Graphics::PostProcessing {

  //-------------------------------------------------------------------------
  // Post-processing settings – add new effect parameters here
  //-------------------------------------------------------------------------
  struct Settings {
    bool enabled = false;

    // Which layers are affected (empty = ALL layers affected)
    std::vector<std::string> affected_layers;

    // Gamma correction
    float gamma = 1.0f;

    // Kawase Bloom
    bool bloom_enabled = false;
    float bloom_threshold = 0.8f;
    float bloom_intensity = 1.0f;
    int bloom_iterations = 5;       // number of downsample/upsample passes
    glm::vec3 bloom_tint = glm::vec3(1.0f);       // multiply bloom by this color
    bool bloom_dirt_enabled = false;
    float bloom_anamorphic = 0.0f;                 // -1=vertical, 0=uniform, 1=horizontal

    // Color Grading (3D LUT)
    bool lut_enabled = false;
    float lut_intensity = 1.0f;     // 0 = original color, 1 = full LUT

    // Vignette
    bool vignette_enabled = false;
    float vignette_intensity = 0.5f;  // 0 = none, 1 = full black corners
    float vignette_radius = 0.75f;    // how far from center darkening starts
    glm::vec3 vignette_color = glm::vec3(0.0f);    // tint color (default black)
    float vignette_roundness = 1.0f;               // 1=circle, <1=elliptical
    float vignette_smoothness = 0.45f;             // falloff softness

    // Chromatic Aberration
    bool chromatic_aberration_enabled = false;
    float chromatic_aberration_intensity = 0.005f; // UV offset per channel
    int chromatic_aberration_samples = 3;          // 3 or 6
    bool chromatic_aberration_edge_only = false;

    // CRT Emulation
    bool crt_enabled = false;
    float crt_scanline_intensity = 0.3f; // strength of scanlines
    float crt_curvature = 0.02f;         // barrel distortion amount
    float crt_scanline_count = 300.0f;   // number of scanlines
    bool crt_subpixel_enabled = false;
    bool crt_glow_enabled = false;
    float crt_glow_intensity = 0.15f;
    float crt_corner_radius = 0.03f;               // 0 = sharp corners
    float crt_flicker_intensity = 0.0f;            // 0 = none

    // Color Quantization (posterization)
    bool quantize_enabled = false;
    int quantize_levels = 8;             // color levels per channel (2-256)
    bool quantize_dither_enabled = false;
    glm::ivec3 quantize_levels_rgb = glm::ivec3(8); // per-channel levels
    int quantize_palette = 0;                       // 0=custom, 1=NES, 2=GB, 3=PICO-8

    // Film Grain (NEW effect)
    bool film_grain_enabled = false;
    float film_grain_intensity = 0.1f;
    float film_grain_size = 1.0f;

    // Tone Mapping (NEW effect)
    bool tonemapping_enabled = false;
    int tonemapping_mode = 0;                      // 0=ACES, 1=Reinhard, 2=Uncharted2
    float tonemapping_exposure = 1.0f;

    // Sharpen (NEW effect)
    bool sharpen_enabled = false;
    float sharpen_intensity = 0.5f;                // 0 = none, 1 = full CAS

    // Effect variant (0 = Type 1, 1 = Type 2)
    int effect_variant = 0;

    // MSAA Anti-Aliasing
    bool msaa_enabled = false;
    int msaa_samples = 4;           // 2, 4, or 8
  };

  //-------------------------------------------------------------------------
  // Lifecycle
  //-------------------------------------------------------------------------

  // Initialize post-processing resources (FBOs, shaders, identity LUT).
  // Call after OpenGL context and renderer are initialized.
  bool init();

  // Clean up all post-processing GPU resources.
  void exit();

  //-------------------------------------------------------------------------
  // Render pipeline
  //-------------------------------------------------------------------------

  // Bind the scene FBO – all subsequent draw calls render into it.
  void begin_scene_capture();

  // Run post-processing passes and blit the result to target_fbo.
  // Pass 0 for target_fbo to render to the default framebuffer.
  void end_scene_capture_and_apply(GLuint target_fbo = 0);

  //-------------------------------------------------------------------------
  // Settings
  //-------------------------------------------------------------------------

  Settings& get_settings();
  void set_settings(const Settings& settings);

  //-------------------------------------------------------------------------
  // LUT management
  //-------------------------------------------------------------------------

  // Load a .cube LUT file for color grading.
  bool load_lut_from_cube(const std::string& filepath);

  //-------------------------------------------------------------------------
  // Bloom dirt texture
  //-------------------------------------------------------------------------

  // Load a dirt/lens texture for bloom overlay.
  void load_bloom_dirt_texture(const std::string& filepath);

  //-------------------------------------------------------------------------
  // Serialization
  //-------------------------------------------------------------------------

  // Serialize current settings to a JSON string.
  std::string serialize_settings_to_json(const Settings& settings);

  // Deserialize settings from a JSON string.
  Settings deserialize_settings_from_json(const std::string& json_str);

  //-------------------------------------------------------------------------
  // Layer queries
  //-------------------------------------------------------------------------

  // Check if a named layer should render into the post-processing FBO.
  // Returns true if the layer is in affected_layers, or if affected_layers is empty.
  bool is_layer_affected(const std::string& layer_name);

  //-------------------------------------------------------------------------
  // Resize
  //-------------------------------------------------------------------------

  // Resize all FBOs when the viewport/window changes.
  void resize(int width, int height);

  // Get current internal resolution
  int get_width();
  int get_height();

} // namespace Engine::Graphics::PostProcessing
