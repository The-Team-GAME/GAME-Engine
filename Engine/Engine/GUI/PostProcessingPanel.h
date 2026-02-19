//-----------------------------------------------------------------------------------
// file:    PostProcessingPanel.h
// project: Monkey Stole My GPA
//
// brief:   ImGui panel for live-editing post-processing settings: enable/disable,
//          gamma, Kawase bloom (threshold, intensity, iterations), 3D LUT color
//          grading from .cube files, vignette, chromatic aberration, CRT emulation,
//          color quantization, film grain, tone mapping, sharpen, and preset
//          save/load.
//
// Copyright  2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include "Precompiled.h"
#include "../Graphics/Renderer.h"
#include "../Resources.h"
#include "ResourcePicker.h"
#include <filesystem>

inline void PostProcessingPanel()
{
  using namespace Engine::Graphics;

  ImGui::Begin("Post Processing");

  auto& pp = get_post_process_settings();

  // Master toggle
  ImGui::Checkbox("Enabled (F9)", &pp.enabled);

  if (!pp.enabled) {
    ImGui::TextDisabled("Post-processing is disabled");
    ImGui::End();
    return;
  }

  ImGui::Separator();

  // ==================== Gamma ====================
  if (ImGui::CollapsingHeader("Gamma", ImGuiTreeNodeFlags_DefaultOpen))
  {
    ImGui::SliderFloat("##GammaSlider", &pp.gamma, 0.2f, 10.0f, "Gamma: %.2f");
    if (ImGui::IsItemHovered())
      ImGui::SetTooltip("Default: 1.0 (no correction)\nHigher = brighter midtones, Lower = darker\npow(color, 1/gamma): gamma>1 lifts midtones, gamma<1 crushes them");

    ImGui::SameLine();
    if (ImGui::SmallButton("Reset##Gamma")) { pp.gamma = 1.0f; }
  }

  ImGui::Separator();

  // ==================== Bloom ====================
  if (ImGui::CollapsingHeader("Bloom", ImGuiTreeNodeFlags_DefaultOpen))
  {
    ImGui::Checkbox("##BloomEnabled", &pp.bloom_enabled);
    ImGui::SameLine();
    ImGui::Text("Enable Bloom");

    if (pp.bloom_enabled)
    {
      ImGui::SliderFloat("##BloomThreshold", &pp.bloom_threshold, 0.0f, 3.0f, "Threshold: %.2f");
      if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Brightness threshold for bloom extraction.\n"
          "Lower = more glow, Higher = only bright spots");

      ImGui::SliderFloat("##BloomIntensity", &pp.bloom_intensity, 0.0f, 5.0f, "Intensity: %.2f");
      if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Strength of the bloom effect");

      ImGui::SliderInt("##BloomIterations", &pp.bloom_iterations, 1, 8, "Iterations: %d");
      if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Number of downsample/upsample passes.\n"
          "More = wider bloom radius");

      ImGui::ColorEdit3("Bloom Tint##BloomTint", &pp.bloom_tint.x);
      if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Color multiplier applied to bloom");

      ImGui::Checkbox("##BloomDirtEnabled", &pp.bloom_dirt_enabled);
      ImGui::SameLine();
      ImGui::Text("Dirt Overlay");
      if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Enable a dirt/lens texture overlay on bloom.\nLoad a texture with the button below.");

      if (pp.bloom_dirt_enabled) {
        if (ImGui::Button("Load Dirt Texture...", ImVec2(-1, 0))) {
          ImGui::OpenPopup("BloomDirtPicker");
        }
        // Simple texture picker using resource picker pattern
        std::string selected_dirt;
        if (ResourcePicker::texture_picker_popup("BloomDirtPicker", selected_dirt)) {
          PostProcessing::load_bloom_dirt_texture(selected_dirt);
        }
      }

      ImGui::SliderFloat("##BloomAnamorphic", &pp.bloom_anamorphic, -1.0f, 1.0f, "Anamorphic: %.2f");
      if (ImGui::IsItemHovered())
        ImGui::SetTooltip("-1 = vertical stretch, 0 = uniform, 1 = horizontal stretch");
    }
  }

  ImGui::Separator();

  // ==================== Color Grading (LUT) ====================
  if (ImGui::CollapsingHeader("Color Grading (LUT)", ImGuiTreeNodeFlags_DefaultOpen))
  {
    ImGui::Checkbox("##LUTEnabled", &pp.lut_enabled);
    ImGui::SameLine();
    ImGui::Text("Enable LUT");

    if (pp.lut_enabled)
    {
      ImGui::SliderFloat("##LUTIntensity", &pp.lut_intensity, 0.0f, 1.0f, "LUT Intensity: %.2f");
      if (ImGui::IsItemHovered())
        ImGui::SetTooltip("0 = original colors, 1 = full LUT effect");

      // Current LUT display and picker
      static std::string current_lut_path = "(none)";

      ImGui::Text("Current LUT:");
      ImGui::SameLine();
      ImGui::TextColored(ImVec4(0.6f, 0.8f, 1.0f, 1.0f), "%s",
        current_lut_path == "(none)" ? "(none)" :
        std::filesystem::path(current_lut_path).filename().string().c_str());

      if (ImGui::Button("Browse LUT...", ImVec2(-1, 0)))
      {
        ImGui::OpenPopup("LUT Picker");
      }

      // LUT picker popup
      std::string selected_lut;
      if (ResourcePicker::lut_picker_popup("LUT Picker", selected_lut, current_lut_path))
      {
        if (load_lut(selected_lut))
        {
          current_lut_path = selected_lut;
        }
      }
    }
  }

  ImGui::Separator();

  // ==================== Tone Mapping (NEW) ====================
  if (ImGui::CollapsingHeader("Tone Mapping"))
  {
    ImGui::Checkbox("##TonemappingEnabled", &pp.tonemapping_enabled);
    ImGui::SameLine();
    ImGui::Text("Enable Tone Mapping");

    if (pp.tonemapping_enabled)
    {
      const char* modes[] = { "ACES", "Reinhard", "Uncharted2" };
      ImGui::Combo("##TonemappingMode", &pp.tonemapping_mode, modes, IM_ARRAYSIZE(modes));
      if (ImGui::IsItemHovered())
        ImGui::SetTooltip("ACES = filmic look, Reinhard = neutral, Uncharted2 = cinematic");

      ImGui::SliderFloat("##TonemappingExposure", &pp.tonemapping_exposure, 0.1f, 5.0f, "Exposure: %.2f");
      if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Exposure multiplier applied before tone mapping");
    }
  }

  ImGui::Separator();

  // ==================== Vignette ====================
  if (ImGui::CollapsingHeader("Vignette"))
  {
    ImGui::Checkbox("##VignetteEnabled", &pp.vignette_enabled);
    ImGui::SameLine();
    ImGui::Text("Enable Vignette");

    if (pp.vignette_enabled)
    {
      ImGui::SliderFloat("##VignetteIntensity", &pp.vignette_intensity, 0.0f, 1.0f, "Intensity: %.2f");
      if (ImGui::IsItemHovered())
        ImGui::SetTooltip("0 = no darkening, 1 = full vignette at corners");

      ImGui::SliderFloat("##VignetteRadius", &pp.vignette_radius, 0.1f, 1.5f, "Radius: %.2f");
      if (ImGui::IsItemHovered())
        ImGui::SetTooltip("How far from center the darkening starts\nSmaller = more visible vignette");

      ImGui::ColorEdit3("Vignette Color##VigColor", &pp.vignette_color.x);
      if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Color of the vignette (default black)");

      ImGui::SliderFloat("##VignetteRoundness", &pp.vignette_roundness, 0.2f, 1.0f, "Roundness: %.2f");
      if (ImGui::IsItemHovered())
        ImGui::SetTooltip("1 = circular, <1 = elliptical");

      ImGui::SliderFloat("##VignetteSmoothness", &pp.vignette_smoothness, 0.01f, 1.0f, "Smoothness: %.2f");
      if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Falloff softness of the vignette edge");
    }
  }

  ImGui::Separator();

  // ==================== Chromatic Aberration ====================
  if (ImGui::CollapsingHeader("Chromatic Aberration"))
  {
    ImGui::Checkbox("##ChromaticEnabled", &pp.chromatic_aberration_enabled);
    ImGui::SameLine();
    ImGui::Text("Enable Chromatic Aberration");

    if (pp.chromatic_aberration_enabled)
    {
      ImGui::SliderFloat("##ChromaticIntensity", &pp.chromatic_aberration_intensity, 0.0f, 0.05f, "Intensity: %.4f");
      if (ImGui::IsItemHovered())
        ImGui::SetTooltip("UV offset between R/G/B channels\nHigher = more rainbow fringing at edges");

      const char* ca_modes[] = { "Simple (3-tap)", "Spectral" };
      ImGui::Combo("##CAMode", &pp.chromatic_aberration_mode, ca_modes, IM_ARRAYSIZE(ca_modes));
      if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Simple = fast 3-tap R/G/B split\nSpectral = N-tap with continuous spectral weights");

      if (pp.chromatic_aberration_mode == 1) {
        ImGui::SliderInt("##CASamples", &pp.chromatic_aberration_samples, 3, 12, "Samples: %d");
        if (ImGui::IsItemHovered())
          ImGui::SetTooltip("Number of spectral taps (3-12)\nMore = smoother color fringing");
      }

      ImGui::Checkbox("##CAEdgeOnly", &pp.chromatic_aberration_edge_only);
      ImGui::SameLine();
      ImGui::Text("Edge Only");
      if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Only apply aberration at screen edges, keep center clean");
    }
  }

  ImGui::Separator();

  // ==================== Film Grain (NEW) ====================
  if (ImGui::CollapsingHeader("Film Grain"))
  {
    ImGui::Checkbox("##FilmGrainEnabled", &pp.film_grain_enabled);
    ImGui::SameLine();
    ImGui::Text("Enable Film Grain");

    if (pp.film_grain_enabled)
    {
      const char* grain_modes[] = { "Fine", "Coarse" };
      ImGui::Combo("##GrainMode", &pp.film_grain_mode, grain_modes, IM_ARRAYSIZE(grain_modes));
      if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Fine = resolution-scaled noise, stronger in shadows\nCoarse = simpler noise, softer luminance weighting");

      ImGui::SliderFloat("##FilmGrainIntensity", &pp.film_grain_intensity, 0.0f, 0.5f, "Intensity: %.3f");
      if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Strength of the noise overlay\nApplied more in dark areas");

      ImGui::SliderFloat("##FilmGrainSize", &pp.film_grain_size, 0.5f, 3.0f, "Size: %.2f");
      if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Scale of the noise pattern\nHigher = larger grain");
    }
  }

  ImGui::Separator();

  // ==================== Sharpen (NEW) ====================
  if (ImGui::CollapsingHeader("Sharpen"))
  {
    ImGui::Checkbox("##SharpenEnabled", &pp.sharpen_enabled);
    ImGui::SameLine();
    ImGui::Text("Enable Sharpen (CAS)");

    if (pp.sharpen_enabled)
    {
      ImGui::SliderFloat("##SharpenIntensity", &pp.sharpen_intensity, 0.0f, 1.0f, "Intensity: %.2f");
      if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Contrast-adaptive sharpening intensity\n0 = none, 1 = full sharpening\nClamped to prevent ringing");
    }
  }

  ImGui::Separator();

  // ==================== CRT Emulation ====================
  if (ImGui::CollapsingHeader("CRT Emulation"))
  {
    ImGui::Checkbox("##CRTEnabled", &pp.crt_enabled);
    ImGui::SameLine();
    ImGui::Text("Enable CRT");

    if (pp.crt_enabled)
    {
      ImGui::SliderFloat("##CRTScanlineIntensity", &pp.crt_scanline_intensity, 0.0f, 1.0f, "Scanline: %.2f");
      if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Strength of horizontal scanlines\n0 = invisible, 1 = very dark lines");

      ImGui::SliderFloat("##CRTCurvature", &pp.crt_curvature, 0.0f, 0.15f, "Curvature: %.3f");
      if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Barrel distortion to simulate a curved CRT screen\n0 = flat");

      ImGui::SliderFloat("##CRTScanlineCount", &pp.crt_scanline_count, 100.0f, 800.0f, "Lines: %.0f");
      if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Number of scanlines across the screen\nHigher = finer lines");

      ImGui::Checkbox("##CRTSubpixel", &pp.crt_subpixel_enabled);
      ImGui::SameLine();
      ImGui::Text("RGB Sub-pixel Mask");
      if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Simulate RGB sub-pixel layout of CRT phosphors");
      ImGui::SameLine();
      ImGui::Checkbox("##CRTGlow", &pp.crt_glow_enabled);
      ImGui::SameLine();
      ImGui::Text("Phosphor Glow");
      if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Add a soft glow to simulate phosphor bleeding");

      if (pp.crt_glow_enabled) {
        ImGui::SliderFloat("##CRTGlowIntensity", &pp.crt_glow_intensity, 0.0f, 0.5f, "Glow: %.2f");
      }

      ImGui::SliderFloat("##CRTCornerRadius", &pp.crt_corner_radius, 0.0f, 0.15f, "Corner Radius: %.3f");
      if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Round the corners of the CRT screen\n0 = sharp corners");

      ImGui::SliderFloat("##CRTFlicker", &pp.crt_flicker_intensity, 0.0f, 0.3f, "Flicker: %.3f");
      if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Simulate brightness flicker of old CRT monitors");
    }
  }

  ImGui::Separator();

  // ==================== Color Quantization ====================
  if (ImGui::CollapsingHeader("Color Quantization"))
  {
    ImGui::Checkbox("##QuantizeEnabled", &pp.quantize_enabled);
    ImGui::SameLine();
    ImGui::Text("Enable Posterization");

    if (pp.quantize_enabled)
    {
      ImGui::SliderInt("##QuantizeLevels", &pp.quantize_levels, 2, 64, "Levels: %d");
      if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Color levels per channel (global fallback)\n2 = extreme posterization, 64 = subtle");

      ImGui::Checkbox("##QuantizeDither", &pp.quantize_dither_enabled);
      ImGui::SameLine();
      ImGui::Text("Bayer 8x8 Dithering");
      if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Apply ordered dithering to smooth color banding");

      const char* palette_names[] = { "Custom", "NES", "Game Boy", "PICO-8" };
      ImGui::Combo("##QuantizePalette", &pp.quantize_palette, palette_names, IM_ARRAYSIZE(palette_names));
      if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Preset palette: overrides per-channel levels\nCustom = use sliders below");

      if (pp.quantize_palette == 0) {
        ImGui::Text("Per-Channel Levels:");
        ImGui::SliderInt("##QuantizeR", &pp.quantize_levels_rgb.x, 2, 64, "R: %d");
        ImGui::SliderInt("##QuantizeG", &pp.quantize_levels_rgb.y, 2, 64, "G: %d");
        ImGui::SliderInt("##QuantizeB", &pp.quantize_levels_rgb.z, 2, 64, "B: %d");
      }
    }
  }

  ImGui::Separator();

  // ==================== Anti-Aliasing (MSAA) ====================
  if (ImGui::CollapsingHeader("Anti-Aliasing (MSAA)", ImGuiTreeNodeFlags_DefaultOpen))
  {
    ImGui::Checkbox("##MSAAEnabled", &pp.msaa_enabled);
    ImGui::SameLine();
    ImGui::Text("Enable MSAA");

    if (pp.msaa_enabled)
    {
      // Query GPU max once
      GLint max_samples = 0;
      glGetIntegerv(GL_MAX_SAMPLES, &max_samples);

      // Build list filtered by GPU capability
      const int all_values[] = { 2, 4, 8, 16, 32 };
      const char* all_labels[] = { "2x", "4x", "8x", "16x", "32x" };
      const char* filtered_labels[5];
      int filtered_values[5];
      int count = 0;

      for (int i = 0; i < 5; ++i) {
        if (all_values[i] <= max_samples) {
          filtered_labels[count] = all_labels[i];
          filtered_values[count] = all_values[i];
          ++count;
        }
      }

      // Find current selection index
      int current_idx = 0;
      for (int i = 0; i < count; ++i) {
        if (filtered_values[i] == pp.msaa_samples) { current_idx = i; break; }
      }

      if (ImGui::Combo("##MSAASamples", &current_idx, filtered_labels, count)) {
        pp.msaa_samples = filtered_values[current_idx];
      }
      if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Higher = smoother edges, more GPU cost");

      ImGui::TextDisabled("GPU max: %dx", max_samples);
    }
  }

  ImGui::Separator();

  // ==================== Presets ====================
  if (ImGui::CollapsingHeader("Presets"))
  {
    // --- Quick presets ---
    if (ImGui::Button("Default", ImVec2(100, 0)))
    {
      // Reset all to defaults
      PostProcessing::Settings defaults;
      defaults.enabled = pp.enabled; // keep master toggle
      defaults.affected_layers = pp.affected_layers;
      pp = defaults;
    }

    ImGui::SameLine();

    if (ImGui::Button("Cinematic", ImVec2(100, 0)))
    {
      pp.gamma = 2.4f;
      pp.bloom_enabled = true;
      pp.bloom_threshold = 0.6f;
      pp.bloom_intensity = 0.8f;
      pp.bloom_iterations = 5;
      pp.bloom_tint = glm::vec3(1.0f, 0.95f, 0.9f);
      pp.lut_enabled = true;
      pp.lut_intensity = 0.7f;
      pp.vignette_enabled = true;
      pp.vignette_intensity = 0.4f;
      pp.vignette_radius = 0.8f;
      pp.tonemapping_enabled = true;
      pp.tonemapping_mode = 0; // ACES
      pp.tonemapping_exposure = 1.2f;
      pp.film_grain_enabled = true;
      pp.film_grain_intensity = 0.05f;
    }

    ImGui::SameLine();

    if (ImGui::Button("Dreamy", ImVec2(100, 0)))
    {
      pp.gamma = 2.0f;
      pp.bloom_enabled = true;
      pp.bloom_threshold = 0.3f;
      pp.bloom_intensity = 2.0f;
      pp.bloom_iterations = 6;
      pp.bloom_tint = glm::vec3(1.0f, 0.9f, 1.0f);
      pp.lut_enabled = false;
      pp.vignette_enabled = true;
      pp.vignette_intensity = 0.3f;
      pp.chromatic_aberration_enabled = true;
      pp.chromatic_aberration_intensity = 0.003f;
    }

    ImGui::SameLine();

    if (ImGui::Button("Retro CRT", ImVec2(100, 0)))
    {
      pp.gamma = 2.2f;
      pp.crt_enabled = true;
      pp.crt_scanline_intensity = 0.4f;
      pp.crt_curvature = 0.04f;
      pp.crt_scanline_count = 300.0f;
      pp.crt_subpixel_enabled = true;
      pp.crt_glow_enabled = true;
      pp.crt_glow_intensity = 0.15f;
      pp.crt_corner_radius = 0.05f;
      pp.crt_flicker_intensity = 0.05f;
      pp.quantize_enabled = true;
      pp.quantize_palette = 1; // NES
      pp.quantize_dither_enabled = true;
    }

    ImGui::Spacing();

    // --- Save/Load to JSON ---
    static char preset_name[128] = "my_preset";
    ImGui::InputText("##PresetName", preset_name, IM_ARRAYSIZE(preset_name));
    ImGui::SameLine();
    if (ImGui::Button("Save Preset"))
    {
      std::string filename = std::string("postprocessing/") + preset_name + ".json";
      Engine::Resources::save_postprocessing_settings(filename, pp);
    }

    // Load preset: scan directory for .json files
    if (ImGui::Button("Load Preset...", ImVec2(-1, 0)))
    {
      ImGui::OpenPopup("PresetLoader");
    }

    if (ImGui::BeginPopup("PresetLoader"))
    {
      std::string dir = Engine::Resources::resource_path + "postprocessing/";
      if (std::filesystem::exists(dir)) {
        for (const auto& entry : std::filesystem::directory_iterator(dir)) {
          if (entry.path().extension() == ".json") {
            std::string name = entry.path().stem().string();
            if (ImGui::Selectable(name.c_str())) {
              std::string filepath = "postprocessing/" + name + ".json";
              auto loaded = Engine::Resources::load_postprocessing_settings(filepath);
              loaded.enabled = pp.enabled; // keep master toggle
              loaded.affected_layers = pp.affected_layers;
              pp = loaded;
            }
          }
        }
      }
      else {
        ImGui::TextDisabled("No presets directory found");
      }
      ImGui::EndPopup();
    }
  }

  ImGui::End();
}
