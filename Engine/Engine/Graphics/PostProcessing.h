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

    // Color Grading (3D LUT)
    bool lut_enabled = false;
    float lut_intensity = 1.0f;     // 0 = original color, 1 = full LUT
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
