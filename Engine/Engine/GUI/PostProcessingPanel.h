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
    ImGui::SliderFloat("Gamma", &pp.gamma, 0.5f, 5.0f, "%.2f");
    if (ImGui::IsItemHovered())
      ImGui::SetTooltip("Default: 2.2\nLower = brighter, Higher = darker");

    ImGui::SameLine();
    if (ImGui::SmallButton("Reset##Gamma")) { pp.gamma = 2.2f; }
  }

  ImGui::Separator();

  // ==================== Bloom ====================
  if (ImGui::CollapsingHeader("Bloom", ImGuiTreeNodeFlags_DefaultOpen))
  {
    ImGui::Checkbox("Enable Bloom", &pp.bloom_enabled);

    if (pp.bloom_enabled)
    {
      ImGui::SliderFloat("Threshold", &pp.bloom_threshold, 0.0f, 3.0f, "%.2f");
      if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Brightness threshold for bloom extraction.\n"
          "Lower = more glow, Higher = only bright spots");

      ImGui::SliderFloat("Intensity", &pp.bloom_intensity, 0.0f, 5.0f, "%.2f");
      if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Strength of the bloom effect");

      ImGui::SliderInt("Iterations", &pp.bloom_iterations, 1, 8);
      if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Number of downsample/upsample passes.\n"
          "More = wider bloom radius");
    }
  }

  ImGui::Separator();

  // ==================== Color Grading (LUT) ====================
  if (ImGui::CollapsingHeader("Color Grading (LUT)", ImGuiTreeNodeFlags_DefaultOpen))
  {
    ImGui::Checkbox("Enable LUT", &pp.lut_enabled);

    if (pp.lut_enabled)
    {
      ImGui::SliderFloat("LUT Intensity", &pp.lut_intensity, 0.0f, 1.0f, "%.2f");
      if (ImGui::IsItemHovered())
        ImGui::SetTooltip("0 = original colors, 1 = full LUT effect");

      // LUT file loader
      static char lut_path[256] = "Assets/lut/PictureFX_J.S._Style_Ethereal_Purple.cube";
      ImGui::InputText("LUT Path", lut_path, sizeof(lut_path));

      if (ImGui::Button("Load .cube LUT"))
      {
        if (load_lut(lut_path))
        {
          ImGui::SameLine();
          ImGui::TextColored(ImVec4(0.2f, 0.8f, 0.2f, 1.0f), "Loaded!");
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
