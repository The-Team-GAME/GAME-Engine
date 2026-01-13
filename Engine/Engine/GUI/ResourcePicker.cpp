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
#include "../pch.h"
#include "ResourcePicker.h"
#include "../Resources.h"
#include <algorithm>
#include <filesystem>
#include <cctype>

namespace ResourcePicker {

  std::vector<Engine::Graphics::Texture> get_all_loaded_textures() {
    std::vector<Engine::Graphics::Texture> textures;

    textures.reserve(Engine::Resources::texture_cache.size() + 1); // +1 for unit_rect

    textures.push_back(Engine::Graphics::get_rect_texture());

    for (const auto& [path, weak_tex] : Engine::Resources::texture_cache) {
      if (auto tex = weak_tex.lock()) {
        textures.push_back(tex);
      }
    }

    // Sort by path for consistent ordering
    std::sort(textures.begin() + 1, textures.end(), // +1 to skip unit_rect
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
      std::transform(search_str.begin(), search_str.end(), search_str.begin(),
        [](char c) { return static_cast<char>(std::tolower(static_cast<int>(c))); });

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
          std::transform(path_lower.begin(), path_lower.end(), path_lower.begin(),
            [](char c) { return static_cast<char>(std::tolower(static_cast<int>(c))); });
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
    int current_size, int grid_columns, float preview_size) {
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
      std::transform(search_str.begin(), search_str.end(), search_str.begin(),
        [](char c) { return static_cast<char>(std::tolower(static_cast<int>(c))); });

      ImGui::Separator();

      // Scrollable region for grid
      ImGui::BeginChild("FontGrid", ImVec2(0, -30), true);

      int column = 0;
      int rendered_count = 0;

      for (const auto& font : fonts) {
        // Extract filename from path for display
        std::filesystem::path fs_path(font->path);
        std::string filename = fs_path.filename().string();

        // Filter by search
        if (!search_str.empty()) {
          std::string path_lower = font->path;
          std::transform(path_lower.begin(), path_lower.end(), path_lower.begin(),
            [](char c) { return static_cast<char>(std::tolower(static_cast<int>(c))); });
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

        // Create a button with "ABC" text preview using the actual font
        std::string button_id = "##font_" + std::to_string(reinterpret_cast<uintptr_t>(font.get()));
        ImVec2 button_size(preview_size, preview_size);

        // Draw button background
        ImVec2 button_pos = ImGui::GetCursorScreenPos();
        if (ImGui::InvisibleButton(button_id.c_str(), button_size)) {
          selected_path = font->path;
          selected_size = font->font_size;
          selection_made = true;
          ImGui::CloseCurrentPopup();
        }

        // Draw "ABC" text centered in the button using the font's texture atlas
        ImDrawList* draw_list = ImGui::GetWindowDrawList();

        // Calculate scale factor to fit text in preview
        float base_char_height = static_cast<float>(font->font_size);
        float base_char_width = base_char_height * 0.6f; // Approximate width per character
        float base_total_width = base_char_width * 3; // "ABC" = 3 characters

        // Scale down if necessary to fit in preview (with some padding)
        float max_preview_dimension = preview_size * 0.8f; // Use 80% of preview size for padding
        float scale = 1.0f;
        if (base_char_height > max_preview_dimension || base_total_width > max_preview_dimension) {
          float height_scale = max_preview_dimension / base_char_height;
          float width_scale = max_preview_dimension / base_total_width;
          scale = std::min(height_scale, width_scale);
        }

        float char_height = base_char_height * scale;
        float total_width = base_total_width * scale;

        // Center the text
        ImVec2 text_pos = ImVec2(
          button_pos.x + (preview_size - total_width) * 0.5f,
          button_pos.y + (preview_size - char_height) * 0.5f
        );

        // Draw "ABC" using individual glyphs from the font
        const char* preview_text = "ABC";
        float cursor_x = text_pos.x;
        for (int i = 0; preview_text[i] != '\0'; ++i) {
          auto glyph_it = font->glyphs.find(preview_text[i]);
          if (glyph_it != font->glyphs.end()) {
            const auto& glyph = glyph_it->second;

            // Calculate glyph position and size with scaling
            ImVec2 glyph_pos(
              cursor_x + glyph.bearing.x * scale,
              text_pos.y + (char_height - glyph.bearing.y * scale)
            );
            ImVec2 glyph_size(
              static_cast<float>(glyph.size.x) * scale,
              static_cast<float>(glyph.size.y) * scale
            );

            // Draw the glyph
            draw_list->AddImage(
              (ImTextureID)(intptr_t)font->texture_atlas.id,
              glyph_pos,
              ImVec2(glyph_pos.x + glyph_size.x, glyph_pos.y + glyph_size.y),
              ImVec2(glyph.tex_coords[3].x, glyph.tex_coords[3].y), // top-left
              ImVec2(glyph.tex_coords[1].x, glyph.tex_coords[1].y), // bottom-right
              IM_COL32(255, 255, 255, 255)
            );

            cursor_x += (glyph.advance >> 6) * scale; // Advance is in 1/64 pixels, scaled
          }
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
