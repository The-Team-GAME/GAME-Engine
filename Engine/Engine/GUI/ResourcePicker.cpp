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
#include "../Precompiled.h"
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

    ImGui::SetNextWindowSize(ImVec2(900, 600), ImGuiCond_FirstUseEver);

    if (ImGui::BeginPopupModal(popup_id, nullptr, 0)) {
      // Header with title and stats
      ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[0]); // Use default/larger font if available
      ImGui::Text("Texture Browser");
      ImGui::PopFont();

      auto textures = get_all_loaded_textures();
      ImGui::SameLine();
      ImGui::TextDisabled("(%zu assets)", textures.size());

      ImGui::Separator();

      if (textures.empty()) {
        ImGui::Dummy(ImVec2(0, 50));
        ImGui::Indent(300);
        ImGui::TextColored(ImVec4(0.8f, 0.6f, 0.2f, 1.0f), "No textures loaded");
        ImGui::Unindent(300);
        ImGui::Dummy(ImVec2(0, 50));
        ImGui::Separator();
        if (ImGui::Button("Close", ImVec2(120, 0))) {
          ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
        return false;
      }

      // Toolbar - Search and Controls
      ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8, 4));
      ImGui::PushItemWidth(250);
      static char search_buffer[256] = "";
      ImGui::InputTextWithHint("##search", "Search textures...", search_buffer, sizeof(search_buffer));
      ImGui::PopItemWidth();
      ImGui::PopStyleVar();

      std::string search_str(search_buffer);
      std::transform(search_str.begin(), search_str.end(), search_str.begin(),
        [](char c) { return static_cast<char>(std::tolower(static_cast<int>(c))); });

      ImGui::SameLine();

      // Sort options
      static int sort_mode = 0; // 0=Name, 1=Size(Dimensions), 2=Width, 3=Height
      ImGui::PushItemWidth(120);
      const char* sort_items[] = { "Name", "Dimensions", "Width", "Height" };
      ImGui::Combo("##sort", &sort_mode, sort_items, IM_ARRAYSIZE(sort_items));
      ImGui::PopItemWidth();

      ImGui::SameLine();

      // Thumbnail size slider
      static float thumb_size = 128.0f;
      ImGui::PushItemWidth(150);
      ImGui::SliderFloat("##thumbsize", &thumb_size, 64.0f, 256.0f, "Size: %.0f");
      ImGui::PopItemWidth();

      // Filter by dimensions
      ImGui::SameLine();
      static bool filter_by_dimensions = false;
      ImGui::Checkbox("Filter Size", &filter_by_dimensions);

      static int min_width = 0, min_height = 0;
      if (filter_by_dimensions) {
        ImGui::SameLine();
        ImGui::PushItemWidth(80);
        ImGui::InputInt("Min W##minw", &min_width, 0);
        ImGui::SameLine();
        ImGui::InputInt("Min H##minh", &min_height, 0);
        ImGui::PopItemWidth();
      }

      ImGui::Separator();

      // Sort textures based on selected mode
      std::vector<Engine::Graphics::Texture> sorted_textures = textures;
      switch (sort_mode) {
      case 0: // Name
        std::sort(sorted_textures.begin() + 1, sorted_textures.end(),
          [](const Engine::Graphics::Texture& a, const Engine::Graphics::Texture& b) {
            std::filesystem::path pa(a->path), pb(b->path);
            return pa.filename().string() < pb.filename().string();
          });
        break;
      case 1: // Dimensions (total pixels)
        std::sort(sorted_textures.begin() + 1, sorted_textures.end(),
          [](const Engine::Graphics::Texture& a, const Engine::Graphics::Texture& b) {
            return (a->width * a->height) > (b->width * b->height);
          });
        break;
      case 2: // Width
        std::sort(sorted_textures.begin() + 1, sorted_textures.end(),
          [](const Engine::Graphics::Texture& a, const Engine::Graphics::Texture& b) {
            return a->width > b->width;
          });
        break;
      case 3: // Height
        std::sort(sorted_textures.begin() + 1, sorted_textures.end(),
          [](const Engine::Graphics::Texture& a, const Engine::Graphics::Texture& b) {
            return a->height > b->height;
          });
        break;
      }

      // Main content area - split into grid and preview panel
      float preview_panel_width = 250.0f;

      // Scrollable grid region
      ImGui::BeginChild("TextureGrid", ImVec2(-preview_panel_width - 10, -40), true,
        ImGuiWindowFlags_HorizontalScrollbar);

      static Engine::Graphics::Texture hovered_texture = nullptr;
      static Engine::Graphics::Texture selected_preview = nullptr;

      // Calculate dynamic columns based on thumb size
      float available_width = ImGui::GetContentRegionAvail().x;
      int dynamic_columns = std::max(1, static_cast<int>(available_width / (thumb_size + 20)));

      int column = 0;
      int rendered_count = 0;

      for (const auto& texture : sorted_textures) {
        std::filesystem::path fs_path(texture->path);
        std::string filename = fs_path.filename().string();

        // Search filter - search in full path
        if (!search_str.empty()) {
          std::string path_lower = texture->path;
          std::transform(path_lower.begin(), path_lower.end(), path_lower.begin(),
            [](char c) { return static_cast<char>(std::tolower(static_cast<int>(c))); });
          if (path_lower.find(search_str) == std::string::npos) {
            continue;
          }
        }

        // Dimension filter
        if (filter_by_dimensions) {
          if (static_cast<int>(texture->width) < min_width ||
            static_cast<int>(texture->height) < min_height) {
            continue;
          }
        }

        rendered_count++;

        // Item background
        ImVec2 item_pos = ImGui::GetCursorScreenPos();
        ImVec2 item_size(thumb_size + 10, thumb_size + 60);

        bool is_current = (texture->path == current_path);
        bool is_selected = (selected_preview == texture);

        ImDrawList* draw_list = ImGui::GetWindowDrawList();

        // Background highlight
        if (is_current || is_selected) {
          draw_list->AddRectFilled(item_pos,
            ImVec2(item_pos.x + item_size.x, item_pos.y + item_size.y),
            IM_COL32(60, 120, 200, 80), 3.0f);
        }

        ImGui::BeginGroup();
        ImGui::PushID(texture.get());

        // Create clickable area
        ImGui::InvisibleButton("##texarea", item_size);
        bool is_hovered = ImGui::IsItemHovered();
        bool is_clicked = ImGui::IsItemClicked(0);
        bool is_double_clicked = ImGui::IsItemClicked(0) && ImGui::IsMouseDoubleClicked(0);

        if (is_hovered) {
          hovered_texture = texture;
          draw_list->AddRect(item_pos,
            ImVec2(item_pos.x + item_size.x, item_pos.y + item_size.y),
            IM_COL32(100, 150, 255, 200), 3.0f, 0, 2.0f);
        }

        if (is_clicked) {
          selected_preview = texture;
        }

        if (is_double_clicked) {
          selected_path = texture->path;
          selection_made = true;
          ImGui::CloseCurrentPopup();
        }

        // Draw texture preview centered
        float aspect = static_cast<float>(texture->width) / static_cast<float>(texture->height);
        ImVec2 img_size;
        if (aspect > 1.0f) {
          img_size = ImVec2(thumb_size, thumb_size / aspect);
        }
        else {
          img_size = ImVec2(thumb_size * aspect, thumb_size);
        }

        ImVec2 img_pos = ImVec2(
          item_pos.x + (item_size.x - img_size.x) * 0.5f,
          item_pos.y + 5
        );

        // Checkerboard background for transparency
        draw_list->AddRectFilled(img_pos,
          ImVec2(img_pos.x + img_size.x, img_pos.y + img_size.y),
          IM_COL32(80, 80, 80, 255));

        draw_list->AddImage(texture->id, img_pos,
          ImVec2(img_pos.x + img_size.x, img_pos.y + img_size.y));

        // Border around image
        draw_list->AddRect(img_pos,
          ImVec2(img_pos.x + img_size.x, img_pos.y + img_size.y),
          IM_COL32(100, 100, 100, 255), 0.0f, 0, 1.0f);

        // Text label
        ImVec2 text_pos = ImVec2(item_pos.x + 5, img_pos.y + img_size.y + 8);
        draw_list->AddText(text_pos, IM_COL32(220, 220, 220, 255),
          filename.length() > 18 ? (filename.substr(0, 15) + "...").c_str() : filename.c_str());

        // Dimensions label
        std::string dims = std::to_string(texture->width) + "x" + std::to_string(texture->height);
        ImVec2 dims_pos = ImVec2(item_pos.x + 5, text_pos.y + 16);
        draw_list->AddText(dims_pos, IM_COL32(150, 150, 150, 255), dims.c_str());

        ImGui::PopID();
        ImGui::EndGroup();

        // Grid layout
        column++;
        if (column < dynamic_columns) {
          ImGui::SameLine();
        }
        else {
          column = 0;
        }
      }

      ImGui::EndChild();

      // Preview panel
      ImGui::SameLine();
      ImGui::BeginChild("PreviewPanel", ImVec2(preview_panel_width, -40), true);

      ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "PREVIEW");
      ImGui::Separator();

      Engine::Graphics::Texture preview_tex = hovered_texture ? hovered_texture : selected_preview;

      if (preview_tex) {
        std::filesystem::path fs_path(preview_tex->path);

        // Large preview
        float preview_w = preview_panel_width - 20;
        float preview_aspect = static_cast<float>(preview_tex->width) / static_cast<float>(preview_tex->height);
        ImVec2 large_preview_size;
        if (preview_aspect > 1.0f) {
          large_preview_size = ImVec2(preview_w, preview_w / preview_aspect);
        }
        else {
          large_preview_size = ImVec2(preview_w * preview_aspect, preview_w);
        }

        ImGui::Image(preview_tex->id, large_preview_size);

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // Details
        ImGui::TextWrapped("%s", fs_path.filename().string().c_str());
        ImGui::Spacing();

        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Dimensions:");
        ImGui::Text("%d x %d", preview_tex->width, preview_tex->height);
        ImGui::Spacing();

        float aspect_ratio = static_cast<float>(preview_tex->width) / static_cast<float>(preview_tex->height);
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Aspect Ratio:");
        ImGui::Text("%.3f", aspect_ratio);
        ImGui::Spacing();

        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Path:");
        ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + preview_panel_width - 20);
        ImGui::TextWrapped("%s", preview_tex->path.c_str());
        ImGui::PopTextWrapPos();

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // Select button
        if (ImGui::Button("Select This", ImVec2(-1, 30))) {
          selected_path = preview_tex->path;
          selection_made = true;
          ImGui::CloseCurrentPopup();
        }
      }
      else {
        ImGui::Dummy(ImVec2(0, 50));
        ImGui::TextWrapped("Hover or click a texture to preview");
      }

      ImGui::EndChild();

      // Bottom bar
      ImGui::Separator();
      ImGui::Text("%d / %zu textures", rendered_count, textures.size());
      ImGui::SameLine(ImGui::GetWindowWidth() - 250);
      ImGui::TextDisabled("Double-click to select");
      ImGui::SameLine(ImGui::GetWindowWidth() - 130);
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
