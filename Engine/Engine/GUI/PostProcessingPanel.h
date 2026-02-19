//-----------------------------------------------------------------------------------
// file:    PostProcessingPanel.h
// project: Monkey Stole My GPA
//
// brief:   ImGui panel for live-editing post-processing settings: enable/disable,
//          gamma, Kawase bloom (threshold, intensity, iterations), 3D LUT color
//          grading from .cube files, and per-layer selection.
//
// Copyright  2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include "Precompiled.h"
#include "../Graphics/Renderer.h"
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
    ImGui::SliderFloat("##GammaSlider", &pp.gamma, 0.5f, 5.0f, "Gamma: %.2f");
    if (ImGui::IsItemHovered())
      ImGui::SetTooltip("Default: 2.2 (sRGB standard)\nHigher = brighter midtones, Lower = darker\npow(color, 1/gamma): gamma>1 lifts midtones, gamma<1 crushes them");

    ImGui::SameLine();
    if (ImGui::SmallButton("Reset##Gamma")) { pp.gamma = 2.2f; }
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

  // ==================== Presets ====================
  if (ImGui::CollapsingHeader("Presets"))
  {
    if (ImGui::Button("Default", ImVec2(100, 0)))
    {
      pp.gamma = 2.2f;
      pp.bloom_enabled = false;
      pp.bloom_threshold = 0.8f;
      pp.bloom_intensity = 1.0f;
      pp.bloom_iterations = 5;
      pp.lut_enabled = false;
      pp.lut_intensity = 1.0f;
    }

    ImGui::SameLine();

    if (ImGui::Button("Cinematic", ImVec2(100, 0)))
    {
      pp.gamma = 2.4f;
      pp.bloom_enabled = true;
      pp.bloom_threshold = 0.6f;
      pp.bloom_intensity = 0.8f;
      pp.bloom_iterations = 5;
      pp.lut_enabled = true;
      pp.lut_intensity = 0.7f;
    }

    ImGui::SameLine();

    if (ImGui::Button("Dreamy", ImVec2(100, 0)))
    {
      pp.gamma = 2.0f;
      pp.bloom_enabled = true;
      pp.bloom_threshold = 0.3f;
      pp.bloom_intensity = 2.0f;
      pp.bloom_iterations = 6;
      pp.lut_enabled = false;
    }

    ImGui::SameLine();

    if (ImGui::Button("High Contrast", ImVec2(100, 0)))
    {
      pp.gamma = 2.8f;
      pp.bloom_enabled = true;
      pp.bloom_threshold = 1.2f;
      pp.bloom_intensity = 0.5f;
      pp.bloom_iterations = 4;
      pp.lut_enabled = false;
    }
  }

  ImGui::End();
}
