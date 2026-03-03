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
#include "../Path.h"
#include "../Graphics/CubeLUT.h"
#include "../Graphics/PostProcessing.h"
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
    const std::string& current_path) {
    bool selection_made = false;

    ImGui::SetNextWindowSize(ImVec2(900, 600), ImGuiCond_FirstUseEver);

    if (ImGui::BeginPopupModal(popup_id, nullptr, 0)) {

      // ---- Persistent state ----
      struct TextureEntry {
        std::string full_path;    // absolute path used for loading
        std::string display_path; // relative path returned on selection
        std::string filename;
        Engine::Graphics::Texture texture = nullptr;
      };

      static std::vector<TextureEntry> texture_entries;
      static bool needs_scan = true;
      static Engine::Graphics::Texture hovered_texture = nullptr;
      static Engine::Graphics::Texture selected_preview = nullptr;

      // ---- Scan assets/images directory ----
      if (needs_scan) {
        texture_entries.clear();
        hovered_texture = nullptr;
        selected_preview = nullptr;

        // Always include the built-in unit rect first
        if (auto rect = Engine::Graphics::get_rect_texture()) {
          TextureEntry e;
          e.full_path    = rect->path;
          e.display_path = rect->path;
          e.filename     = "[unit rect]";
          e.texture      = rect;
          texture_entries.push_back(std::move(e));
        }

        // Recursively scan the images sub-folder
        namespace fs = std::filesystem;
        std::string images_dir = Engine::Resources::resource_path + "images";
        if (fs::exists(images_dir)) {
          const char* valid_exts[] = { ".png", ".jpg", ".jpeg", ".bmp", ".tga", ".gif", nullptr };
          try {
            for (const auto& fse : fs::recursive_directory_iterator(
              images_dir, fs::directory_options::skip_permission_denied)) {
              if (!fse.is_regular_file()) continue;

              std::string ext = fse.path().extension().string();
              std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

              bool valid = false;
              for (int ei = 0; valid_exts[ei]; ++ei) {
                if (ext == valid_exts[ei]) { valid = true; break; }
              }
              if (!valid) continue;

              TextureEntry te;
              te.full_path    = fse.path().string();
              te.filename     = fse.path().filename().string();
              te.display_path = Engine::Resources::get_relative_path(te.full_path);
              // Load now (uses cache if already present; non-persistent)
              te.texture = Engine::Resources::load_texture(te.full_path, false, false);
              texture_entries.push_back(std::move(te));
            }
          }
          catch (...) {}
        }

        needs_scan = false;
      }

      // ---- Header ----
      ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[0]);
      ImGui::Text("Texture Browser");
      ImGui::PopFont();
      ImGui::SameLine();
      ImGui::TextDisabled("(%zu assets)", texture_entries.size());
      ImGui::SameLine();
      if (ImGui::SmallButton("Refresh")) { needs_scan = true; }

      ImGui::Separator();

      if (texture_entries.empty()) {
        ImGui::Dummy(ImVec2(0, 50));
        ImGui::Indent(300);
        ImGui::TextColored(ImVec4(0.8f, 0.6f, 0.2f, 1.0f), "No textures found in assets/images/");
        ImGui::Unindent(300);
        ImGui::Dummy(ImVec2(0, 50));
        ImGui::Separator();
        if (ImGui::Button("Close", ImVec2(120, 0))) { ImGui::CloseCurrentPopup(); }
        ImGui::EndPopup();
        return false;
      }

      // ---- Toolbar ----
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

      static int sort_mode = 0; // 0=Name, 1=Dimensions, 2=Width, 3=Height
      ImGui::PushItemWidth(120);
      const char* sort_items[] = { "Name", "Dimensions", "Width", "Height" };
      ImGui::Combo("##sort", &sort_mode, sort_items, IM_ARRAYSIZE(sort_items));
      ImGui::PopItemWidth();

      ImGui::SameLine();

      static float thumb_size = 128.0f;
      ImGui::PushItemWidth(150);
      ImGui::SliderFloat("##thumbsize", &thumb_size, 64.0f, 256.0f, "Size: %.0f");
      ImGui::PopItemWidth();

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

      // ---- Build sorted index list (index 0 = unit rect, always stays first) ----
      std::vector<int> sorted_indices(texture_entries.size());
      for (int i = 0; i < static_cast<int>(texture_entries.size()); ++i) sorted_indices[i] = i;

      auto get_tex = [&](int i) -> const Engine::Graphics::Texture& {
        return texture_entries[i].texture;
      };

      // Sort everything after index 0
      auto sort_begin = sorted_indices.begin() + (texture_entries.size() > 1 ? 1 : 0);
      switch (sort_mode) {
      case 0: // Name
        std::sort(sort_begin, sorted_indices.end(),
          [&](int a, int b) { return texture_entries[a].filename < texture_entries[b].filename; });
        break;
      case 1: // Dimensions (total pixels, loaded entries first)
        std::sort(sort_begin, sorted_indices.end(), [&](int a, int b) {
          const auto& ta = get_tex(a); const auto& tb = get_tex(b);
          if (!ta && !tb) return false;
          if (!ta) return false; if (!tb) return true;
          return (ta->width * ta->height) > (tb->width * tb->height);
        });
        break;
      case 2: // Width
        std::sort(sort_begin, sorted_indices.end(), [&](int a, int b) {
          const auto& ta = get_tex(a); const auto& tb = get_tex(b);
          if (!ta && !tb) return false;
          if (!ta) return false; if (!tb) return true;
          return ta->width > tb->width;
        });
        break;
      case 3: // Height
        std::sort(sort_begin, sorted_indices.end(), [&](int a, int b) {
          const auto& ta = get_tex(a); const auto& tb = get_tex(b);
          if (!ta && !tb) return false;
          if (!ta) return false; if (!tb) return true;
          return ta->height > tb->height;
        });
        break;
      }

      // ---- Main content: grid + preview panel ----
      float preview_panel_width = 250.0f;

      ImGui::BeginChild("TextureGrid", ImVec2(-preview_panel_width - 10, -40), true,
        ImGuiWindowFlags_HorizontalScrollbar);

      float available_width = ImGui::GetContentRegionAvail().x;
      int dynamic_columns = std::max(1, static_cast<int>(available_width / (thumb_size + 20)));

      int column = 0;
      int rendered_count = 0;

      for (int idx : sorted_indices) {
        const TextureEntry& entry   = texture_entries[idx];
        const Engine::Graphics::Texture& texture = entry.texture;

        // Search filter
        if (!search_str.empty()) {
          std::string path_lower = entry.full_path;
          std::transform(path_lower.begin(), path_lower.end(), path_lower.begin(),
            [](char c) { return static_cast<char>(std::tolower(static_cast<int>(c))); });
          if (path_lower.find(search_str) == std::string::npos) continue;
        }

        // Dimension filter (only for loaded textures)
        if (filter_by_dimensions && texture) {
          if (static_cast<int>(texture->width)  < min_width ||
              static_cast<int>(texture->height) < min_height) continue;
        }

        rendered_count++;

        ImVec2 item_pos  = ImGui::GetCursorScreenPos();
        ImVec2 item_size(thumb_size + 10, thumb_size + 60);

        bool is_current  = (entry.display_path == current_path || entry.full_path == current_path);
        bool is_selected = (texture && selected_preview == texture);

        ImDrawList* draw_list = ImGui::GetWindowDrawList();

        if (is_current || is_selected) {
          draw_list->AddRectFilled(item_pos,
            ImVec2(item_pos.x + item_size.x, item_pos.y + item_size.y),
            IM_COL32(60, 120, 200, 80), 3.0f);
        }

        ImGui::BeginGroup();
        ImGui::PushID(idx);

        ImGui::InvisibleButton("##texarea", item_size);
        bool is_hovered       = ImGui::IsItemHovered();
        bool is_double_clicked = is_hovered && ImGui::IsMouseDoubleClicked(0);

        if (is_hovered) {
          hovered_texture = texture;
          draw_list->AddRect(item_pos,
            ImVec2(item_pos.x + item_size.x, item_pos.y + item_size.y),
            IM_COL32(100, 150, 255, 200), 3.0f, 0, 2.0f);
        }

        if (ImGui::IsItemClicked(0)) {
          selected_preview = texture;
        }

        if (is_double_clicked) {
          selected_path  = entry.display_path;
          selection_made = true;
          ImGui::CloseCurrentPopup();
        }

        // Draw thumbnail
        if (texture && texture->id != 0) {
          float aspect   = static_cast<float>(texture->width) / static_cast<float>(texture->height);
          ImVec2 img_size = (aspect > 1.0f)
            ? ImVec2(thumb_size, thumb_size / aspect)
            : ImVec2(thumb_size * aspect, thumb_size);

          ImVec2 img_pos(
            item_pos.x + (item_size.x - img_size.x) * 0.5f,
            item_pos.y + 5);

          draw_list->AddRectFilled(img_pos,
            ImVec2(img_pos.x + img_size.x, img_pos.y + img_size.y),
            IM_COL32(80, 80, 80, 255));
          draw_list->AddImage(texture->id, img_pos,
            ImVec2(img_pos.x + img_size.x, img_pos.y + img_size.y));
          draw_list->AddRect(img_pos,
            ImVec2(img_pos.x + img_size.x, img_pos.y + img_size.y),
            IM_COL32(100, 100, 100, 255), 0.0f, 0, 1.0f);

          std::string disp = entry.filename.length() > 18
            ? entry.filename.substr(0, 15) + "..." : entry.filename;
          std::string dims = std::to_string(texture->width) + "x" + std::to_string(texture->height);

          draw_list->AddText(ImVec2(item_pos.x + 5, img_pos.y + img_size.y + 8),
            IM_COL32(220, 220, 220, 255), disp.c_str());
          draw_list->AddText(ImVec2(item_pos.x + 5, img_pos.y + img_size.y + 24),
            IM_COL32(150, 150, 150, 255), dims.c_str());
        }
        else {
          // Placeholder for failed/unloaded textures
          ImVec2 ph_pos(item_pos.x + 5, item_pos.y + 5);
          draw_list->AddRectFilled(ph_pos,
            ImVec2(ph_pos.x + thumb_size, ph_pos.y + thumb_size),
            IM_COL32(60, 40, 40, 200));

          std::string disp = entry.filename.length() > 18
            ? entry.filename.substr(0, 15) + "..." : entry.filename;
          draw_list->AddText(ImVec2(item_pos.x + 5, ph_pos.y + thumb_size + 8),
            IM_COL32(180, 100, 100, 255), disp.c_str());
          draw_list->AddText(ImVec2(item_pos.x + 5, ph_pos.y + thumb_size + 24),
            IM_COL32(150, 80, 80, 255), "(load failed)");
        }

        ImGui::PopID();
        ImGui::EndGroup();

        column++;
        if (column < dynamic_columns) ImGui::SameLine();
        else column = 0;
      }

      ImGui::EndChild();

      // ---- Preview panel ----
      ImGui::SameLine();
      ImGui::BeginChild("PreviewPanel", ImVec2(preview_panel_width, -40), true);

      ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "PREVIEW");
      ImGui::Separator();

      Engine::Graphics::Texture preview_tex = hovered_texture ? hovered_texture : selected_preview;

      if (preview_tex) {
        std::filesystem::path fs_path(preview_tex->path);

        float preview_w      = preview_panel_width - 20;
        float preview_aspect = static_cast<float>(preview_tex->width) / static_cast<float>(preview_tex->height);
        ImVec2 large_preview_size = (preview_aspect > 1.0f)
          ? ImVec2(preview_w, preview_w / preview_aspect)
          : ImVec2(preview_w * preview_aspect, preview_w);

        ImGui::Image(preview_tex->id, large_preview_size);
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::TextWrapped("%s", fs_path.filename().string().c_str());
        ImGui::Spacing();
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Dimensions:");
        ImGui::Text("%d x %d", preview_tex->width, preview_tex->height);
        ImGui::Spacing();
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Aspect Ratio:");
        ImGui::Text("%.3f", static_cast<float>(preview_tex->width) / static_cast<float>(preview_tex->height));
        ImGui::Spacing();
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Path:");
        ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + preview_panel_width - 20);
        ImGui::TextWrapped("%s", preview_tex->path.c_str());
        ImGui::PopTextWrapPos();
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        if (ImGui::Button("Select This", ImVec2(-1, 30))) {
          // Find the display_path for this texture
          for (const auto& e : texture_entries) {
            if (e.texture == preview_tex) {
              selected_path = e.display_path;
              break;
            }
          }
          selection_made = true;
          ImGui::CloseCurrentPopup();
        }
      }
      else {
        ImGui::Dummy(ImVec2(0, 50));
        ImGui::TextWrapped("Hover or click a texture to preview");
      }

      ImGui::EndChild();

      // ---- Bottom bar ----
      ImGui::Separator();
      ImGui::Text("%d / %zu textures", rendered_count, texture_entries.size());
      ImGui::SameLine(ImGui::GetWindowWidth() - 250);
      ImGui::TextDisabled("Double-click to select");
      ImGui::SameLine(ImGui::GetWindowWidth() - 130);
      if (ImGui::Button("Close", ImVec2(120, 0))) { ImGui::CloseCurrentPopup(); }

      ImGui::EndPopup();
    }

    return selection_made;
  }

  bool font_picker_popup(const char* popup_id, std::string& selected_path,
    int& selected_size, const std::string& current_path,
    int current_size, int grid_columns, float preview_size) {
    bool selection_made = false;

    ImGui::SetNextWindowSize(ImVec2(700, 500), ImGuiCond_FirstUseEver);

    if (ImGui::BeginPopupModal(popup_id, nullptr, 0)) {

      // ---- Persistent state ----
      struct FontEntry {
        std::string full_path;
        std::string display_path;  // relative path returned on selection
        std::string filename;
        Engine::Graphics::Font font = nullptr;  // loaded at preview_px size
        int loaded_at_size = 0;
      };

      static std::vector<FontEntry> font_entries;
      static bool needs_scan = true;
      static int preview_px = 32;   // size used to render the "ABC" preview
      static int export_size = 32;  // size the caller receives on selection
      static int highlighted = -1;

      // ---- Scan fonts directory ----
      if (needs_scan) {
        font_entries.clear();
        highlighted = -1;

        namespace fs = std::filesystem;
        std::string fonts_dir = Engine::Resources::resource_path + "fonts";
        if (fs::exists(fonts_dir)) {
          const char* valid_exts[] = { ".ttf", ".otf", nullptr };
          try {
            for (const auto& fse : fs::recursive_directory_iterator(
              fonts_dir, fs::directory_options::skip_permission_denied)) {
              if (!fse.is_regular_file()) continue;

              std::string ext = fse.path().extension().string();
              std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

              bool valid = false;
              for (int ei = 0; valid_exts[ei]; ++ei) {
                if (ext == valid_exts[ei]) { valid = true; break; }
              }
              if (!valid) continue;

              FontEntry fe;
              fe.full_path    = fse.path().string();
              fe.filename     = fse.path().filename().string();
              fe.display_path = Engine::Resources::get_relative_path(fe.full_path);
              // Load at chosen preview size (non-persistent)
              fe.font = Engine::Resources::load_font(fe.full_path, static_cast<unsigned int>(preview_px), false, false);
              fe.loaded_at_size = preview_px;
              font_entries.push_back(std::move(fe));
            }
          }
          catch (...) {}
        }

        // Sort alphabetically by filename
        std::sort(font_entries.begin(), font_entries.end(),
          [](const FontEntry& a, const FontEntry& b) { return a.filename < b.filename; });

        needs_scan = false;
      }

      // ---- Header ----
      ImGui::Text("Font Browser");
      ImGui::SameLine();
      ImGui::TextDisabled("(%zu fonts)", font_entries.size());
      ImGui::SameLine();
      if (ImGui::SmallButton("Refresh")) { needs_scan = true; }

      ImGui::Separator();

      if (font_entries.empty()) {
        ImGui::Dummy(ImVec2(0, 40));
        ImGui::Indent(200);
        ImGui::TextColored(ImVec4(0.8f, 0.6f, 0.2f, 1.0f), "No font files found in assets/fonts/");
        ImGui::Unindent(200);
        ImGui::Dummy(ImVec2(0, 40));
        ImGui::Separator();
        if (ImGui::Button("Close", ImVec2(120, 0))) { ImGui::CloseCurrentPopup(); }
        ImGui::EndPopup();
        return false;
      }

      // ---- Toolbar ----
      ImGui::PushItemWidth(250);
      static char search_buffer[256] = "";
      ImGui::InputTextWithHint("##fontsearch", "Search fonts...", search_buffer, sizeof(search_buffer));
      ImGui::PopItemWidth();

      std::string search_str(search_buffer);
      std::transform(search_str.begin(), search_str.end(), search_str.begin(),
        [](char c) { return static_cast<char>(std::tolower(static_cast<int>(c))); });

      ImGui::SameLine();

      // Preview size (used to render "ABC" thumbnails)
      ImGui::PushItemWidth(140);
      int prev_preview_px = preview_px;
      ImGui::SliderInt("Preview px##fprev", &preview_px, 12, 72);
      ImGui::PopItemWidth();
      if (preview_px != prev_preview_px) {
        // Reload fonts at new preview size
        for (auto& fe : font_entries) {
          fe.font = Engine::Resources::load_font(fe.full_path, static_cast<unsigned int>(preview_px), false, false);
          fe.loaded_at_size = preview_px;
        }
      }

      ImGui::SameLine();

      // Export / output size
      ImGui::PushItemWidth(110);
      ImGui::InputInt("Size##fsize", &export_size, 1, 8);
      ImGui::PopItemWidth();
      if (export_size < 4) export_size = 4;
      if (export_size > 256) export_size = 256;

      ImGui::Separator();

      // ---- Scrollable grid ----
      ImGui::BeginChild("FontGrid", ImVec2(0, -50), true);

      int column = 0;
      int rendered_count = 0;

      for (int fi = 0; fi < static_cast<int>(font_entries.size()); ++fi) {
        FontEntry& entry = font_entries[fi];

        // Search filter
        if (!search_str.empty()) {
          std::string lower = entry.full_path;
          std::transform(lower.begin(), lower.end(), lower.begin(),
            [](char c) { return static_cast<char>(std::tolower(static_cast<int>(c))); });
          if (lower.find(search_str) == std::string::npos) continue;
        }

        rendered_count++;

        bool is_current     = (entry.display_path == current_path || entry.full_path == current_path);
        bool is_highlighted = (fi == highlighted);

        ImVec2 item_pos  = ImGui::GetCursorScreenPos();
        ImVec2 item_size(preview_size + 10, preview_size + 38);

        ImDrawList* draw_list = ImGui::GetWindowDrawList();

        // Highlight background
        if (is_current || is_highlighted) {
          draw_list->AddRectFilled(item_pos,
            ImVec2(item_pos.x + item_size.x, item_pos.y + item_size.y),
            is_current ? IM_COL32(60, 120, 200, 100) : IM_COL32(80, 80, 80, 120), 3.0f);
        }

        ImGui::BeginGroup();
        ImGui::PushID(fi);

        // Dark card background
        draw_list->AddRectFilled(item_pos,
          ImVec2(item_pos.x + item_size.x, item_pos.y + item_size.y),
          IM_COL32(35, 35, 35, 255), 4.0f);

        ImGui::InvisibleButton("##fontbtn", item_size);
        bool is_hovered       = ImGui::IsItemHovered();
        bool is_double_clicked = is_hovered && ImGui::IsMouseDoubleClicked(0);

        if (ImGui::IsItemClicked(0)) {
          highlighted = fi;
        }

        if (is_double_clicked) {
          selected_path = entry.display_path;
          selected_size = export_size;
          selection_made = true;
          ImGui::CloseCurrentPopup();
        }

        if (is_hovered) {
          draw_list->AddRect(item_pos,
            ImVec2(item_pos.x + item_size.x, item_pos.y + item_size.y),
            IM_COL32(100, 150, 255, 200), 3.0f, 0, 2.0f);
          ImGui::BeginTooltip();
          ImGui::Text("%s", entry.filename.c_str());
          ImGui::Text("Path: %s", entry.display_path.c_str());
          ImGui::EndTooltip();
        }

        // Draw "ABC" preview
        if (entry.font) {
          float base_h = static_cast<float>(entry.font->font_size);
          float base_w = base_h * 1.8f; // 3 chars × ~0.6× width
          float max_dim = preview_size * 0.85f;
          float scale = 1.0f;
          if (base_h > max_dim || base_w > max_dim) {
            scale = std::min(max_dim / base_h, max_dim / base_w);
          }

          float ch    = base_h * scale;
          float tw    = base_w * scale;
          ImVec2 tp(
            item_pos.x + (item_size.x - tw)  * 0.5f,
            item_pos.y + (preview_size - ch) * 0.5f + 5
          );

          const char* preview_text = "Abc";
          float cx = tp.x;
          for (int i = 0; preview_text[i] != '\0'; ++i) {
            auto git = entry.font->glyphs.find(preview_text[i]);
            if (git == entry.font->glyphs.end()) continue;
            const auto& g = git->second;

            ImVec2 gp(cx + g.bearing.x * scale,
              tp.y + (ch - g.bearing.y * scale));
            ImVec2 gs(static_cast<float>(g.size.x) * scale,
              static_cast<float>(g.size.y) * scale);

            draw_list->AddImage(
              (ImTextureID)(intptr_t)entry.font->texture_atlas.id,
              gp, ImVec2(gp.x + gs.x, gp.y + gs.y),
              ImVec2(g.tex_coords[3].x, g.tex_coords[3].y),
              ImVec2(g.tex_coords[1].x, g.tex_coords[1].y),
              IM_COL32(255, 255, 255, 255));

            cx += (g.advance >> 6) * scale;
          }
        }
        else {
          // Placeholder for failed loads
          ImVec2 ph(item_pos.x + 4, item_pos.y + 4);
          draw_list->AddRectFilled(ph,
            ImVec2(ph.x + preview_size + 2, ph.y + preview_size),
            IM_COL32(60, 30, 30, 200));
          draw_list->AddText(ImVec2(ph.x + 4, ph.y + preview_size * 0.4f),
            IM_COL32(180, 80, 80, 255), "N/A");
        }

        // Filename label
        std::string disp = entry.filename.length() > 14
          ? entry.filename.substr(0, 11) + "..." : entry.filename;
        draw_list->AddText(
          ImVec2(item_pos.x + 5, item_pos.y + preview_size + 10),
          IM_COL32(210, 210, 210, 255), disp.c_str());

        ImGui::PopID();
        ImGui::EndGroup();

        column++;
        if (column < grid_columns) ImGui::SameLine();
        else column = 0;
      }

      ImGui::EndChild();

      // ---- Bottom bar ----
      ImGui::Separator();
      ImGui::Text("%d / %zu fonts", rendered_count, font_entries.size());
      ImGui::SameLine();
      ImGui::TextDisabled(" | Double-click or use button below to select");

      ImGui::SameLine(ImGui::GetWindowWidth() - 250);
      bool has_sel = (highlighted >= 0 && highlighted < static_cast<int>(font_entries.size()));
      if (!has_sel) ImGui::BeginDisabled();
      if (ImGui::Button("Select", ImVec2(110, 0)) && has_sel) {
        selected_path = font_entries[highlighted].display_path;
        selected_size = export_size;
        selection_made = true;
        ImGui::CloseCurrentPopup();
      }
      if (!has_sel) ImGui::EndDisabled();

      ImGui::SameLine();
      if (ImGui::Button("Close", ImVec2(110, 0))) { ImGui::CloseCurrentPopup(); }

      ImGui::EndPopup();
    }

    return selection_made;
  }

  bool lut_picker_popup(const char* popup_id, std::string& selected_path,
    const std::string& current_path, const std::string& lut_directory) {
    bool selection_made = false;

    ImGui::SetNextWindowSize(ImVec2(700, 500), ImGuiCond_FirstUseEver);

    if (ImGui::BeginPopupModal(popup_id, nullptr, 0)) {

      // ---- State (persistent across frames) ----
      struct LutEntry {
        std::string path;
        std::string filename;
        GLuint preview_texture = 0;   // 1D color ramp texture
        bool parsed = false;
      };

      static std::vector<LutEntry> lut_entries;
      static bool needs_scan = true;
      static int selected_index = -1;

      // ---- Scan directory for .cube files ----
      if (needs_scan) {
        // Clean up previous preview textures
        for (auto& entry : lut_entries) {
          if (entry.preview_texture) {
            glDeleteTextures(1, &entry.preview_texture);
          }
        }
        lut_entries.clear();
        selected_index = -1;

        namespace fs = std::filesystem;
        if (fs::exists(lut_directory) && fs::is_directory(lut_directory)) {
          for (const auto& dir_entry : fs::directory_iterator(lut_directory)) {
            if (dir_entry.is_regular_file()) {
              std::string ext = dir_entry.path().extension().string();
              std::transform(ext.begin(), ext.end(), ext.begin(),
                [](char c) { return static_cast<char>(std::tolower(static_cast<int>(c))); });
              if (ext == ".cube") {
                LutEntry entry;
                entry.path = dir_entry.path().string();
                // Normalize backslashes to forward slashes
                std::replace(entry.path.begin(), entry.path.end(), '\\', '/');
                entry.filename = dir_entry.path().filename().string();
                lut_entries.push_back(entry);
              }
            }
          }
        }

        // Sort by filename
        std::sort(lut_entries.begin(), lut_entries.end(),
          [](const LutEntry& a, const LutEntry& b) { return a.filename < b.filename; });

        needs_scan = false;
      }

      // ---- Generate preview textures lazily ----
      for (auto& entry : lut_entries) {
        if (entry.parsed) continue;
        entry.parsed = true;

        // Parse the LUT to generate a color ramp preview
        auto lut_data = Engine::Graphics::parse_cube_lut(entry.path);
        if (lut_data.size == 0) continue;

        // Sample along the diagonal (r=g=b) to create a 1D gradient
        constexpr int RAMP_WIDTH = 256;
        std::vector<unsigned char> ramp_pixels(RAMP_WIDTH * 4); // RGBA

        for (int i = 0; i < RAMP_WIDTH; ++i) {
          float t = static_cast<float>(i) / static_cast<float>(RAMP_WIDTH - 1);

          // Trilinear sample the 3D LUT along the diagonal
          float coord = t * (lut_data.size - 1);
          int idx = static_cast<int>(coord);
          float frac = coord - idx;
          int next = std::min(idx + 1, lut_data.size - 1);

          // Index into flat array: (r + g*size + b*size*size) * 3
          auto sample = [&](int r, int g, int b) -> glm::vec3 {
            int flat = (r + g * lut_data.size + b * lut_data.size * lut_data.size) * 3;
            return glm::vec3(lut_data.data[flat], lut_data.data[flat + 1], lut_data.data[flat + 2]);
            };

          // Sample along diagonal r=g=b
          glm::vec3 c0 = sample(idx, idx, idx);
          glm::vec3 c1 = sample(next, next, next);
          glm::vec3 color = glm::mix(c0, c1, frac);

          // Clamp and convert to 8-bit
          ramp_pixels[i * 4 + 0] = static_cast<unsigned char>(glm::clamp(color.r, 0.0f, 1.0f) * 255.0f);
          ramp_pixels[i * 4 + 1] = static_cast<unsigned char>(glm::clamp(color.g, 0.0f, 1.0f) * 255.0f);
          ramp_pixels[i * 4 + 2] = static_cast<unsigned char>(glm::clamp(color.b, 0.0f, 1.0f) * 255.0f);
          ramp_pixels[i * 4 + 3] = 255;
        }

        // Upload as a 256x1 texture
        glGenTextures(1, &entry.preview_texture);
        glBindTexture(GL_TEXTURE_2D, entry.preview_texture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, RAMP_WIDTH, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, ramp_pixels.data());
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glBindTexture(GL_TEXTURE_2D, 0);
      }

      // ---- Header ----
      ImGui::Text("LUT Browser");
      ImGui::SameLine();
      ImGui::TextDisabled("(%zu files)", lut_entries.size());
      ImGui::SameLine();
      if (ImGui::SmallButton("Refresh")) { needs_scan = true; }
      ImGui::Separator();

      if (lut_entries.empty()) {
        ImGui::TextColored(ImVec4(0.8f, 0.6f, 0.2f, 1.0f),
          "No .cube files found in %s", lut_directory.c_str());
        ImGui::Separator();
        if (ImGui::Button("Close", ImVec2(120, 0))) {
          needs_scan = true; // Re-scan next time
          ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
        return false;
      }

      // ---- Main content: list with color ramp previews ----
      float preview_panel_width = 280.0f;

      ImGui::BeginChild("LUTList", ImVec2(-preview_panel_width - 10, -40), true);

      for (int i = 0; i < static_cast<int>(lut_entries.size()); ++i) {
        auto& entry = lut_entries[i];
        bool is_current = (entry.path == current_path);
        bool is_selected = (selected_index == i);

        ImGui::PushID(i);

        // Selectable row with highlight
        if (is_current) {
          ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.2f, 0.5f, 0.2f, 0.6f));
        }

        if (ImGui::Selectable("##lut_entry", is_selected || is_current,
          ImGuiSelectableFlags_AllowDoubleClick, ImVec2(0, 50))) {
          selected_index = i;

          if (ImGui::IsMouseDoubleClicked(0)) {
            selected_path = entry.path;
            selection_made = true;
            needs_scan = true;
            ImGui::CloseCurrentPopup();
          }
        }

        if (is_current) {
          ImGui::PopStyleColor();
        }

        // Draw content over the selectable
        ImVec2 item_min = ImGui::GetItemRectMin();
        ImVec2 item_max = ImGui::GetItemRectMax();
        ImDrawList* draw_list = ImGui::GetWindowDrawList();

        // Filename text
        draw_list->AddText(ImVec2(item_min.x + 8, item_min.y + 4),
          IM_COL32(220, 220, 220, 255), entry.filename.c_str());

        // Color ramp preview (stretched across width)
        if (entry.preview_texture) {
          float ramp_y = item_min.y + 24;
          float ramp_height = 20.0f;
          float ramp_width = (item_max.x - item_min.x) - 16.0f;

          ImVec2 ramp_min(item_min.x + 8, ramp_y);
          ImVec2 ramp_max(ramp_min.x + ramp_width, ramp_y + ramp_height);

          draw_list->AddImage(entry.preview_texture, ramp_min, ramp_max);
          draw_list->AddRect(ramp_min, ramp_max, IM_COL32(80, 80, 80, 200));
        }

        // Current indicator
        if (is_current) {
          draw_list->AddText(ImVec2(item_max.x - 70, item_min.y + 4),
            IM_COL32(100, 200, 100, 255), "[ACTIVE]");
        }

        ImGui::PopID();
      }

      ImGui::EndChild();

      // ---- Preview panel ----
      ImGui::SameLine();
      ImGui::BeginChild("LUTPreview", ImVec2(preview_panel_width, -40), true);

      ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "PREVIEW");
      ImGui::Separator();

      if (selected_index >= 0 && selected_index < static_cast<int>(lut_entries.size())) {
        auto& entry = lut_entries[selected_index];

        ImGui::Text("%s", entry.filename.c_str());
        ImGui::Spacing();

        // Large color ramp preview
        if (entry.preview_texture) {
          float ramp_width = preview_panel_width - 20.0f;
          ImGui::Image(entry.preview_texture, ImVec2(ramp_width, 40.0f));
          ImGui::Spacing();

          // Also show a 2D grid preview: rows = R values, columns = G values, at B=0.5
          // This gives a better view of the full color grade
          ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Color Matrix (R vs G at B=0.5):");

          // Generate a small 2D preview if not already done
          static GLuint grid_preview_tex = 0;
          static int grid_for_index = -1;
          constexpr int GRID_SIZE = 64;

          if (grid_for_index != selected_index) {
            auto lut_data = Engine::Graphics::parse_cube_lut(entry.path);
            if (lut_data.size > 0) {
              std::vector<unsigned char> grid_pixels(GRID_SIZE * GRID_SIZE * 4);
              int mid_b = lut_data.size / 2;

              for (int y = 0; y < GRID_SIZE; ++y) {
                for (int x = 0; x < GRID_SIZE; ++x) {
                  float r_coord = static_cast<float>(x) / (GRID_SIZE - 1) * (lut_data.size - 1);
                  float g_coord = static_cast<float>(y) / (GRID_SIZE - 1) * (lut_data.size - 1);

                  int ri = std::min(static_cast<int>(r_coord), lut_data.size - 1);
                  int gi = std::min(static_cast<int>(g_coord), lut_data.size - 1);

                  int flat = (ri + gi * lut_data.size + mid_b * lut_data.size * lut_data.size) * 3;
                  int pixel_idx = (y * GRID_SIZE + x) * 4;
                  grid_pixels[pixel_idx + 0] = static_cast<unsigned char>(glm::clamp(lut_data.data[flat], 0.0f, 1.0f) * 255.0f);
                  grid_pixels[pixel_idx + 1] = static_cast<unsigned char>(glm::clamp(lut_data.data[flat + 1], 0.0f, 1.0f) * 255.0f);
                  grid_pixels[pixel_idx + 2] = static_cast<unsigned char>(glm::clamp(lut_data.data[flat + 2], 0.0f, 1.0f) * 255.0f);
                  grid_pixels[pixel_idx + 3] = 255;
                }
              }

              if (grid_preview_tex) glDeleteTextures(1, &grid_preview_tex);
              glGenTextures(1, &grid_preview_tex);
              glBindTexture(GL_TEXTURE_2D, grid_preview_tex);
              glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, GRID_SIZE, GRID_SIZE, 0, GL_RGBA, GL_UNSIGNED_BYTE, grid_pixels.data());
              glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
              glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
              glBindTexture(GL_TEXTURE_2D, 0);
              grid_for_index = selected_index;
            }
          }

          if (grid_preview_tex) {
            float grid_display_size = preview_panel_width - 20.0f;
            ImGui::Image(grid_preview_tex, ImVec2(grid_display_size, grid_display_size));
          }
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Path:");
        ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + preview_panel_width - 20);
        ImGui::TextWrapped("%s", entry.path.c_str());
        ImGui::PopTextWrapPos();

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // Select button
        if (ImGui::Button("Select This", ImVec2(-1, 30))) {
          selected_path = entry.path;
          selection_made = true;
          needs_scan = true;
          ImGui::CloseCurrentPopup();
        }
      }
      else {
        ImGui::Dummy(ImVec2(0, 50));
        ImGui::TextWrapped("Click a LUT file to preview");
      }

      ImGui::EndChild();

      // ---- Bottom bar ----
      ImGui::Separator();
      ImGui::Text("%zu LUT files in %s", lut_entries.size(), lut_directory.c_str());
      ImGui::SameLine(ImGui::GetWindowWidth() - 250);
      ImGui::TextDisabled("Double-click to select");
      ImGui::SameLine(ImGui::GetWindowWidth() - 130);
      if (ImGui::Button("Close", ImVec2(120, 0))) {
        needs_scan = true;
        ImGui::CloseCurrentPopup();
      }

      ImGui::EndPopup();
    }

    return selection_made;
  }

  // ===========================================================================
  // Preset Picker
  // ===========================================================================

  bool preset_picker_popup(const char* popup_id, std::string& selected_path,
    const std::string& current_path, const std::string& preset_directory) {
    bool selection_made = false;

    ImGui::SetNextWindowSize(ImVec2(780, 520), ImGuiCond_FirstUseEver);

    if (ImGui::BeginPopupModal(popup_id, nullptr, 0)) {

      // ---- Cached state (persistent across frames) ----
      struct PresetEntry {
        std::string path;       // full relative path e.g. "postprocessing/cinematic.json"
        std::string filename;   // just "cinematic.json"
        std::string name;       // stem: "cinematic"
        Engine::Graphics::PostProcessing::Settings settings{};
        bool parsed = false;
        bool valid = false;
      };

      static std::vector<PresetEntry> preset_entries;
      static bool needs_scan = true;
      static int selected_index = -1;

      // ---- Scan directory for .json files ----
      if (needs_scan) {
        preset_entries.clear();
        selected_index = -1;

        std::string scan_dir = Engine::Resources::resource_path + preset_directory;
        namespace fs = std::filesystem;

        if (fs::exists(scan_dir) && fs::is_directory(scan_dir)) {
          for (const auto& dir_entry : fs::directory_iterator(scan_dir)) {
            if (!dir_entry.is_regular_file()) continue;
            std::string ext = dir_entry.path().extension().string();
            std::transform(ext.begin(), ext.end(), ext.begin(),
              [](char c) { return static_cast<char>(std::tolower(static_cast<int>(c))); });
            if (ext != ".json") continue;

            PresetEntry entry;
            entry.filename = dir_entry.path().filename().string();
            entry.name = dir_entry.path().stem().string();
            entry.path = preset_directory + "/" + entry.filename;

            // Parse the preset immediately for preview
            auto loaded = Engine::Resources::load_postprocessing_settings(entry.path);
            entry.settings = loaded;
            entry.parsed = true;
            entry.valid = true; // If load failed, settings will be default
            preset_entries.push_back(entry);
          }
        }

        std::sort(preset_entries.begin(), preset_entries.end(),
          [](const PresetEntry& a, const PresetEntry& b) { return a.name < b.name; });

        needs_scan = false;
      }

      // ---- Header ----
      ImGui::Text("Preset Browser");
      ImGui::SameLine();
      ImGui::TextDisabled("(%zu presets)", preset_entries.size());
      ImGui::SameLine();
      if (ImGui::SmallButton("Refresh")) { needs_scan = true; }
      ImGui::Separator();

      if (preset_entries.empty()) {
        ImGui::TextColored(ImVec4(0.8f, 0.6f, 0.2f, 1.0f),
          "No .json presets found in %s/", preset_directory.c_str());
        ImGui::TextDisabled("Save a preset first to populate this list.");
        ImGui::Separator();
        if (ImGui::Button("Close", ImVec2(120, 0))) {
          needs_scan = true;
          ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
        return false;
      }

      // ---- Helper: effect badge (colored inline label) ----
      auto draw_badge = [](const char* label, ImVec4 color) {
        ImVec2 pos = ImGui::GetCursorScreenPos();
        ImVec2 text_size = ImGui::CalcTextSize(label);
        float pad = 4.0f;
        ImVec2 box_min(pos.x, pos.y);
        ImVec2 box_max(pos.x + text_size.x + pad * 2, pos.y + text_size.y + pad);

        ImDrawList* dl = ImGui::GetWindowDrawList();
        ImU32 bg = ImGui::ColorConvertFloat4ToU32(ImVec4(color.x * 0.3f, color.y * 0.3f, color.z * 0.3f, 0.8f));
        ImU32 border = ImGui::ColorConvertFloat4ToU32(ImVec4(color.x * 0.7f, color.y * 0.7f, color.z * 0.7f, 0.9f));
        ImU32 text_col = ImGui::ColorConvertFloat4ToU32(color);

        dl->AddRectFilled(box_min, box_max, bg, 3.0f);
        dl->AddRect(box_min, box_max, border, 3.0f);
        dl->AddText(ImVec2(box_min.x + pad, box_min.y + pad * 0.5f), text_col, label);

        ImGui::Dummy(ImVec2(text_size.x + pad * 2 + 2, text_size.y + pad));
        };

      // ---- Helper: draw all enabled effect badges inline ----
      auto draw_effect_badges = [&draw_badge](const Engine::Graphics::PostProcessing::Settings& s) {
        float start_x = ImGui::GetCursorPosX();
        float wrap_x = ImGui::GetContentRegionAvail().x + start_x;

        auto maybe_sameline = [&]() {
          float next_x = ImGui::GetCursorPosX();
          if (next_x > start_x && next_x < wrap_x - 60.0f) {
            ImGui::SameLine();
          }
          };

        if (s.bloom_enabled) { draw_badge("Bloom", ImVec4(1.0f, 0.8f, 0.3f, 1.0f));   maybe_sameline(); }
        if (s.lut_enabled) { draw_badge("LUT", ImVec4(0.6f, 0.9f, 0.4f, 1.0f));     maybe_sameline(); }
        if (s.vignette_enabled) { draw_badge("Vignette", ImVec4(0.7f, 0.5f, 1.0f, 1.0f)); maybe_sameline(); }
        if (s.chromatic_aberration_enabled) { draw_badge("CA", ImVec4(1.0f, 0.4f, 0.4f, 1.0f));     maybe_sameline(); }
        if (s.crt_enabled) { draw_badge("CRT", ImVec4(0.3f, 1.0f, 0.8f, 1.0f));     maybe_sameline(); }
        if (s.quantize_enabled) { draw_badge("Quantize", ImVec4(1.0f, 0.6f, 0.9f, 1.0f)); maybe_sameline(); }
        if (s.film_grain_enabled) { draw_badge("Grain", ImVec4(0.8f, 0.8f, 0.6f, 1.0f));   maybe_sameline(); }
        if (s.tonemapping_enabled) { draw_badge("Tonemap", ImVec4(0.4f, 0.7f, 1.0f, 1.0f)); maybe_sameline(); }
        if (s.sharpen_enabled) { draw_badge("Sharpen", ImVec4(0.9f, 0.9f, 0.9f, 1.0f)); maybe_sameline(); }
        if (s.msaa_enabled) { draw_badge("MSAA", ImVec4(0.5f, 0.8f, 0.5f, 1.0f)); }
        };

      // ---- Main content: list + preview ----
      float preview_panel_width = 280.0f;

      ImGui::BeginChild("PresetList", ImVec2(-preview_panel_width - 10, -40), true);

      for (int i = 0; i < static_cast<int>(preset_entries.size()); ++i) {
        auto& entry = preset_entries[i];
        bool is_current = (entry.path == current_path);
        bool is_selected = (selected_index == i);

        ImGui::PushID(i);

        if (is_current) {
          ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.2f, 0.5f, 0.2f, 0.6f));
        }

        // Count enabled effects for the summary line
        int effect_count = 0;
        const auto& s = entry.settings;
        if (s.bloom_enabled) effect_count++;
        if (s.lut_enabled) effect_count++;
        if (s.vignette_enabled) effect_count++;
        if (s.chromatic_aberration_enabled) effect_count++;
        if (s.crt_enabled) effect_count++;
        if (s.quantize_enabled) effect_count++;
        if (s.film_grain_enabled) effect_count++;
        if (s.tonemapping_enabled) effect_count++;
        if (s.sharpen_enabled) effect_count++;
        if (s.msaa_enabled) effect_count++;

        if (ImGui::Selectable("##preset_entry", is_selected || is_current,
          ImGuiSelectableFlags_AllowDoubleClick, ImVec2(0, 46))) {
          selected_index = i;

          if (ImGui::IsMouseDoubleClicked(0)) {
            selected_path = entry.path;
            selection_made = true;
            needs_scan = true;
            ImGui::CloseCurrentPopup();
          }
        }

        if (is_current) {
          ImGui::PopStyleColor();
        }

        // Draw overlay content on the selectable
        ImVec2 item_min = ImGui::GetItemRectMin();
        ImVec2 item_max = ImGui::GetItemRectMax();
        ImDrawList* draw_list = ImGui::GetWindowDrawList();

        // Preset name (larger)
        draw_list->AddText(ImVec2(item_min.x + 8, item_min.y + 4),
          IM_COL32(230, 230, 230, 255), entry.name.c_str());

        // Effect count summary
        char summary[64];
        snprintf(summary, sizeof(summary), "%d effect%s | gamma %.1f",
          effect_count, effect_count != 1 ? "s" : "", s.gamma);
        draw_list->AddText(ImVec2(item_min.x + 8, item_min.y + 24),
          IM_COL32(160, 160, 160, 200), summary);

        // Current indicator
        if (is_current) {
          draw_list->AddText(ImVec2(item_max.x - 70, item_min.y + 4),
            IM_COL32(100, 200, 100, 255), "[ACTIVE]");
        }

        ImGui::PopID();
      }

      ImGui::EndChild();

      // ---- Preview panel ----
      ImGui::SameLine();
      ImGui::BeginChild("PresetPreview", ImVec2(preview_panel_width, -40), true);

      ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "PREVIEW");
      ImGui::Separator();

      if (selected_index >= 0 && selected_index < static_cast<int>(preset_entries.size())) {
        auto& entry = preset_entries[selected_index];
        const auto& s = entry.settings;

        // Name
        ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[0]);
        ImGui::Text("%s", entry.name.c_str());
        ImGui::PopFont();
        ImGui::Spacing();

        // Effect badges
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Active Effects:");
        draw_effect_badges(s);

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // Detailed settings for each enabled effect
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Settings:");
        ImGui::Spacing();

        ImGui::Text("Gamma: %.2f", s.gamma);

        if (s.bloom_enabled) {
          ImGui::Spacing();
          ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.3f, 1.0f), "Bloom");
          ImGui::Text("  Threshold: %.2f", s.bloom_threshold);
          ImGui::Text("  Intensity: %.2f", s.bloom_intensity);
          ImGui::Text("  Iterations: %d", s.bloom_iterations);
        }

        if (s.lut_enabled) {
          ImGui::Spacing();
          ImGui::TextColored(ImVec4(0.6f, 0.9f, 0.4f, 1.0f), "LUT Color Grading");
          ImGui::Text("  Intensity: %.2f", s.lut_intensity);
        }

        if (s.vignette_enabled) {
          ImGui::Spacing();
          ImGui::TextColored(ImVec4(0.7f, 0.5f, 1.0f, 1.0f), "Vignette");
          ImGui::Text("  Intensity: %.2f", s.vignette_intensity);
          ImGui::Text("  Radius: %.2f", s.vignette_radius);
          ImGui::Text("  Roundness: %.2f", s.vignette_roundness);
        }

        if (s.chromatic_aberration_enabled) {
          ImGui::Spacing();
          ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "Chromatic Aberration");
          ImGui::Text("  Mode: %s", s.chromatic_aberration_mode == 0 ? "Simple" : "Spectral");
          ImGui::Text("  Intensity: %.4f", s.chromatic_aberration_intensity);
          if (s.chromatic_aberration_mode == 1)
            ImGui::Text("  Samples: %d", s.chromatic_aberration_samples);
        }

        if (s.crt_enabled) {
          ImGui::Spacing();
          ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.8f, 1.0f), "CRT Emulation");
          ImGui::Text("  Scanlines: %.0f @ %.0f%%", s.crt_scanline_count, s.crt_scanline_intensity * 100.0f);
          ImGui::Text("  Curvature: %.3f", s.crt_curvature);
          if (s.crt_subpixel_enabled) ImGui::Text("  Subpixels: On");
          if (s.crt_glow_enabled) ImGui::Text("  Glow: %.0f%%", s.crt_glow_intensity * 100.0f);
        }

        if (s.quantize_enabled) {
          ImGui::Spacing();
          ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.9f, 1.0f), "Color Quantization");
          const char* palettes[] = { "Custom", "NES", "Game Boy", "PICO-8" };
          int pal = glm::clamp(s.quantize_palette, 0, 3);
          ImGui::Text("  Palette: %s", palettes[pal]);
          if (pal == 0) ImGui::Text("  Levels: %d", s.quantize_levels);
          ImGui::Text("  Dither: %s", s.quantize_dither_enabled ? "On" : "Off");
        }

        if (s.film_grain_enabled) {
          ImGui::Spacing();
          ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.6f, 1.0f), "Film Grain");
          ImGui::Text("  Mode: %s", s.film_grain_mode == 0 ? "Fine" : "Coarse");
          ImGui::Text("  Intensity: %.2f", s.film_grain_intensity);
          ImGui::Text("  Size: %.2f", s.film_grain_size);
        }

        if (s.tonemapping_enabled) {
          ImGui::Spacing();
          ImGui::TextColored(ImVec4(0.4f, 0.7f, 1.0f, 1.0f), "Tone Mapping");
          const char* modes[] = { "ACES", "Reinhard", "Uncharted2" };
          int m = glm::clamp(s.tonemapping_mode, 0, 2);
          ImGui::Text("  Mode: %s", modes[m]);
          ImGui::Text("  Exposure: %.2f", s.tonemapping_exposure);
        }

        if (s.sharpen_enabled) {
          ImGui::Spacing();
          ImGui::TextColored(ImVec4(0.9f, 0.9f, 0.9f, 1.0f), "Sharpen");
          ImGui::Text("  Intensity: %.2f", s.sharpen_intensity);
        }

        if (s.msaa_enabled) {
          ImGui::Spacing();
          ImGui::TextColored(ImVec4(0.5f, 0.8f, 0.5f, 1.0f), "MSAA");
          ImGui::Text("  Samples: %dx", s.msaa_samples);
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Path:");
        ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + preview_panel_width - 20);
        ImGui::TextWrapped("%s", entry.path.c_str());
        ImGui::PopTextWrapPos();

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // Select button
        if (ImGui::Button("Load This Preset", ImVec2(-1, 30))) {
          selected_path = entry.path;
          selection_made = true;
          needs_scan = true;
          ImGui::CloseCurrentPopup();
        }
      }
      else {
        ImGui::Dummy(ImVec2(0, 50));
        ImGui::TextWrapped("Click a preset to preview its settings");
      }

      ImGui::EndChild();

      // ---- Bottom bar ----
      ImGui::Separator();
      ImGui::Text("%zu preset(s) in %s/", preset_entries.size(), preset_directory.c_str());
      ImGui::SameLine(ImGui::GetWindowWidth() - 250);
      ImGui::TextDisabled("Double-click to select");
      ImGui::SameLine(ImGui::GetWindowWidth() - 130);
      if (ImGui::Button("Close", ImVec2(120, 0))) {
        needs_scan = true;
        ImGui::CloseCurrentPopup();
      }

      ImGui::EndPopup();
    }

    return selection_made;
  }

  bool prefab_picker_popup(const char* popup_id, std::string& selected_path, bool& is_new_file) {
    bool selection_made = false;

    ImGui::SetNextWindowSize(ImVec2(500, 450), ImGuiCond_FirstUseEver);

    if (ImGui::BeginPopupModal(popup_id, nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
      ImGui::Text("Save as Prefab");
      ImGui::Separator();

      // --- Create New Prefab section ---
      static char new_prefab_name[256] = "";
      ImGui::Text("Create New:");
      ImGui::PushItemWidth(-120);
      ImGui::InputTextWithHint("##NewPrefabName", "Enter prefab name...", new_prefab_name, sizeof(new_prefab_name));
      ImGui::PopItemWidth();
      ImGui::SameLine();
      if (ImGui::Button("Create", ImVec2(110, 0))) {
        if (strlen(new_prefab_name) > 0) {
          std::string filename = std::string(new_prefab_name);
          if (filename.find(".json") == std::string::npos) {
            filename += ".json";
          }
          selected_path = "prefabs/" + filename;
          is_new_file = true;
          selection_made = true;
          new_prefab_name[0] = '\0';
          ImGui::CloseCurrentPopup();
        }
      }

      ImGui::Separator();

      // --- Update Existing Prefab section ---
      ImGui::Text("Or update existing prefab:");

      static char search_buffer[256] = "";
      ImGui::InputTextWithHint("##PrefabSearch", "Search...", search_buffer, sizeof(search_buffer));

      ImGui::BeginChild("PrefabPickerList", ImVec2(0, 250), true);

      // Gather prefabs from cache
      std::vector<std::pair<std::string, Engine::ECS::Prefab>> prefab_list;
      for (const auto& [path, weak_prefab] : Engine::Resources::prefab_cache) {
        if (auto prefab_ptr = weak_prefab.lock()) {
          std::string relative_path = Engine::Resources::get_relative_path(path);
          if (strlen(search_buffer) > 0 && relative_path.find(search_buffer) == std::string::npos) {
            continue;
          }
          prefab_list.push_back({ relative_path, Engine::ECS::Prefab(prefab_ptr) });
        }
      }

      std::sort(prefab_list.begin(), prefab_list.end(),
        [](const auto& a, const auto& b) { return a.first < b.first; });

      static std::string highlighted_prefab;
      for (const auto& [relative_path, prefab] : prefab_list) {
        bool is_highlighted = (highlighted_prefab == relative_path);
        if (ImGui::Selectable(relative_path.c_str(), is_highlighted)) {
          highlighted_prefab = relative_path;
        }
        // Double-click to confirm
        if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0)) {
          selected_path = relative_path;
          is_new_file = false;
          selection_made = true;
          highlighted_prefab.clear();
          search_buffer[0] = '\0';
          ImGui::CloseCurrentPopup();
        }
      }

      ImGui::EndChild();

      // Bottom bar
      ImGui::Separator();

      // Update button for single-click selection
      bool has_selection = !highlighted_prefab.empty();
      if (!has_selection) {
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.5f);
      }
      if (ImGui::Button("Update Selected", ImVec2(140, 0)) && has_selection) {
        selected_path = highlighted_prefab;
        is_new_file = false;
        selection_made = true;
        highlighted_prefab.clear();
        search_buffer[0] = '\0';
        ImGui::CloseCurrentPopup();
      }
      if (!has_selection) {
        ImGui::PopStyleVar();
      }

      ImGui::SameLine();
      ImGui::TextDisabled("Double-click to update");

      ImGui::SameLine(ImGui::GetWindowWidth() - 80);
      if (ImGui::Button("Cancel", ImVec2(70, 0))) {
        highlighted_prefab.clear();
        search_buffer[0] = '\0';
        ImGui::CloseCurrentPopup();
      }

      ImGui::EndPopup();
    }

    return selection_made;
  }

} // namespace ResourcePicker
