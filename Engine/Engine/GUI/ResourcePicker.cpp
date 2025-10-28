//-----------------------------------------------------------------------------------
// file:    ResourcePicker.cpp
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   This file implements ImGui resource picker widgets for selecting textures,
//          fonts, and other resources with visual previews in a grid layout.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#include <imgui.h>
#include "ResourcePicker.h"
#include "../Resources.h"
#include <algorithm>
#include <filesystem>

namespace ResourcePicker {

  std::vector<Engine::Graphics::Texture> get_all_loaded_textures() {
    std::vector<Engine::Graphics::Texture> textures;

    for (const auto& [path, weak_tex] : Engine::Resources::texture_cache) {
      if (auto tex = weak_tex.lock()) {
        textures.push_back(tex);
      }
    }

    // Sort by path for consistent ordering
    std::sort(textures.begin(), textures.end(),
      [](const Engine::Graphics::Texture& a, const Engine::Graphics::Texture& b) {
        return a->path < b->path;
      });

    return textures;
  }

  std::vector<Engine::Graphics::Font> get_all_loaded_fonts() {
    std::vector<Engine::Graphics::Font> fonts;

    for (const auto& [path, weak_font] : Engine::Resources::font_cache) {
      if (auto font = weak_font.lock()) {
        fonts.push_back(font);
      }
    }

    // Sort by path, then by size
    std::sort(fonts.begin(), fonts.end(),
      [](const Engine::Graphics::Font& a, const Engine::Graphics::Font& b) {
        if (a->path == b->path) return a->font_size < b->font_size;
        return a->path < b->path;
      });

    return fonts;
  }

  bool texture_picker_popup(const char* popup_id, std::string& selected_path,
    const std::string& current_path,
    int grid_columns, float preview_size) {
    bool selection_made = false;

    ImGui::SetNextWindowSize(ImVec2(600, 400), ImGuiCond_FirstUseEver);

    if (ImGui::BeginPopupModal(popup_id, nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
      ImGui::Text("Select a Texture");
      ImGui::Separator();

      // Get all loaded textures
      auto textures = get_all_loaded_textures();

      // Debug: Show count
      ImGui::Text("Loaded textures: %zu", textures.size());

      if (textures.empty()) {
        ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.0f, 1.0f), "No textures loaded");
        ImGui::Separator();
        if (ImGui::Button("Close")) {
          ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
        return false;
      }

      // Search filter
      static char search_buffer[256] = "";
      ImGui::InputText("Search", search_buffer, sizeof(search_buffer));
      std::string search_str(search_buffer);
      std::transform(search_str.begin(), search_str.end(), search_str.begin(), ::tolower);

      ImGui::Separator();

      // Scrollable region for grid
      ImGui::BeginChild("TextureGrid", ImVec2(0, -30), true);

      int column = 0;
      int rendered_count = 0;
      for (const auto& texture : textures) {
        // Extract filename from path for display and search
        std::filesystem::path fs_path(texture->path);
        std::string filename = fs_path.filename().string();

        // Filter by search
        if (!search_str.empty()) {
          std::string path_lower = texture->path;
          std::transform(path_lower.begin(), path_lower.end(), path_lower.begin(), ::tolower);
          if (path_lower.find(search_str) == std::string::npos) {
            continue;
          }
        }

        rendered_count++;
        ImGui::BeginGroup();

        // Highlight if currently selected
        bool is_current = (texture->path == current_path);
        if (is_current) {
          ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.6f, 1.0f, 0.8f));
          ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.4f, 0.7f, 1.0f, 1.0f));
          ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.5f, 0.8f, 1.0f, 1.0f));
        }

        // Texture preview button
        ImTextureID tex_id = texture->id;

        // Calculate aspect ratio for preview
        float aspect = static_cast<float>(texture->width) / static_cast<float>(texture->height);
        ImVec2 button_size;
        if (aspect > 1.0f) {
          button_size = ImVec2(preview_size, preview_size / aspect);
        }
        else {
          button_size = ImVec2(preview_size * aspect, preview_size);
        }

        // Center the image in the preview area
        float offset_x = (preview_size - button_size.x) * 0.5f;
        float offset_y = (preview_size - button_size.y) * 0.5f;
        if (offset_x > 0) ImGui::SetCursorPosX(ImGui::GetCursorPosX() + offset_x);
        if (offset_y > 0) ImGui::SetCursorPosY(ImGui::GetCursorPosY() + offset_y);

        std::string button_id = "##tex_" + std::to_string(reinterpret_cast<uintptr_t>(texture.get()));
        if (ImGui::ImageButton(button_id.c_str(), tex_id, button_size)) {
          selected_path = texture->path;
          selection_made = true;
          ImGui::CloseCurrentPopup();
        }

        if (is_current) {
          ImGui::PopStyleColor(3);
        }

        // Tooltip with details
        if (ImGui::IsItemHovered()) {
          ImGui::BeginTooltip();
          ImGui::Text("%s", filename.c_str());
          ImGui::Text("Size: %dx%d", texture->width, texture->height);
          ImGui::Text("Path: %s", texture->path.c_str());
          ImGui::EndTooltip();
        }

        // Label under preview
        ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + preview_size);
        ImGui::TextWrapped("%s", filename.c_str());
        ImGui::PopTextWrapPos();
        ImGui::Text("%dx%d", texture->width, texture->height);

        ImGui::EndGroup();

        // Grid layout
        column++;
        if (column < grid_columns) {
          ImGui::SameLine();
        }
        else {
          column = 0;
        }
      }

      ImGui::EndChild();

      // Debug: Show how many were rendered
      ImGui::Text("Rendered: %d", rendered_count);

      ImGui::Separator();
      if (ImGui::Button("Close", ImVec2(120, 0))) {
        ImGui::CloseCurrentPopup();
      }

      ImGui::EndPopup();
    }

    return selection_made;
  }

  bool font_picker_popup(const char* popup_id, std::string& selected_path,
    int& selected_size, const std::string& current_path,
    int current_size) {
    bool selection_made = false;

    ImGui::SetNextWindowSize(ImVec2(600, 400), ImGuiCond_FirstUseEver);

    if (ImGui::BeginPopupModal(popup_id, nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
      ImGui::Text("Select a Font");
      ImGui::Separator();

      // Get all loaded fonts
      auto fonts = get_all_loaded_fonts();

      // Debug: Show count
      ImGui::Text("Loaded fonts: %zu", fonts.size());

      if (fonts.empty()) {
        ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.0f, 1.0f), "No fonts loaded");
        ImGui::Separator();
        if (ImGui::Button("Close")) {
          ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
        return false;
      }

      // Search filter
      static char search_buffer[256] = "";
      ImGui::InputText("Search", search_buffer, sizeof(search_buffer));
      std::string search_str(search_buffer);
      std::transform(search_str.begin(), search_str.end(), search_str.begin(), ::tolower);

      ImGui::Separator();

      // Scrollable region for grid
      ImGui::BeginChild("FontGrid", ImVec2(0, -30), true);

      int grid_columns = 4;
      float preview_size = 64.0f;
      int column = 0;
      int rendered_count = 0;

      for (const auto& font : fonts) {
        // Extract filename from path for display
        std::filesystem::path fs_path(font->path);
        std::string filename = fs_path.filename().string();

        // Filter by search
        if (!search_str.empty()) {
          std::string path_lower = font->path;
          std::transform(path_lower.begin(), path_lower.end(), path_lower.begin(), ::tolower);
          if (path_lower.find(search_str) == std::string::npos) {
            continue;
          }
        }

        rendered_count++;
        ImGui::BeginGroup();

        // Highlight if currently selected
        bool is_current = (font->path == current_path && font->font_size == static_cast<unsigned int>(current_size));
        if (is_current) {
          ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.6f, 1.0f, 0.8f));
          ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.4f, 0.7f, 1.0f, 1.0f));
          ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.5f, 0.8f, 1.0f, 1.0f));
        }

        // Font atlas preview button
        ImTextureID tex_id = font->texture_atlas.id;

        // Calculate aspect ratio for preview
        float aspect = static_cast<float>(font->texture_atlas.width) / static_cast<float>(font->texture_atlas.height);
        ImVec2 button_size;
        if (aspect > 1.0f) {
          button_size = ImVec2(preview_size, preview_size / aspect);
        }
        else {
          button_size = ImVec2(preview_size * aspect, preview_size);
        }

        // Center the image in the preview area
        float offset_x = (preview_size - button_size.x) * 0.5f;
        float offset_y = (preview_size - button_size.y) * 0.5f;
        if (offset_x > 0) ImGui::SetCursorPosX(ImGui::GetCursorPosX() + offset_x);
        if (offset_y > 0) ImGui::SetCursorPosY(ImGui::GetCursorPosY() + offset_y);

        std::string button_id = "##font_" + std::to_string(reinterpret_cast<uintptr_t>(font.get()));
        if (ImGui::ImageButton(button_id.c_str(), tex_id, button_size)) {
          selected_path = font->path;
          selected_size = font->font_size;
          selection_made = true;
          ImGui::CloseCurrentPopup();
        }

        if (is_current) {
          ImGui::PopStyleColor(3);
        }

        // Tooltip with details
        if (ImGui::IsItemHovered()) {
          ImGui::BeginTooltip();
          ImGui::Text("%s", filename.c_str());
          ImGui::Text("Size: %dpx", font->font_size);
          ImGui::Text("Atlas: %dx%d", font->texture_atlas.width, font->texture_atlas.height);
          ImGui::Text("Path: %s", font->path.c_str());
          ImGui::EndTooltip();
        }

        // Label under preview
        ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + preview_size);
        ImGui::TextWrapped("%s", filename.c_str());
        ImGui::PopTextWrapPos();
        ImGui::Text("%dpx", font->font_size);

        ImGui::EndGroup();

        // Grid layout
        column++;
        if (column < grid_columns) {
          ImGui::SameLine();
        }
        else {
          column = 0;
        }
      }

      ImGui::EndChild();

      // Debug: Show how many were rendered
      ImGui::Text("Rendered: %d", rendered_count);

      ImGui::Separator();
      if (ImGui::Button("Close", ImVec2(120, 0))) {
        ImGui::CloseCurrentPopup();
      }

      ImGui::EndPopup();
    }

    return selection_made;
  }

} // namespace ResourcePicker
