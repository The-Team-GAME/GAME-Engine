//-----------------------------------------------------------------------------------
// file:    ViewportPanel.cpp
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   Implementation of ViewportPanel for displaying the game viewport inside
//          an ImGui window. Renders the game scene to a framebuffer texture and
//          displays it with proper aspect ratio preservation.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#include "ViewportPanel.h"
#include "../Precompiled.h"
#include "../Logger.h"
#include "../Graphics/Renderer.h"
#include "../WindowManager.h"
#include "../Math/Vector2D.h"
#include "../FileIO/Settings.h"
#include "../LevelTabManager.h"
#include "../FileIO/FileDialog.h"
#include "../Resources.h"
#include "../EventManager/EventManager.h"
#include "../Events/EngineEvents.h"
#include <algorithm>

namespace ViewportPanel {

  // Framebuffer and texture
  namespace {
    GLuint framebuffer = 0;
    GLuint texture_color_buffer = 0;
    GLuint renderbuffer = 0;
    int fb_width = 1600;
    int fb_height = 900;
    bool viewport_hovered = false;
    bool viewport_focused = false;

    // Store viewport image bounds for input conversion
    ImVec2 viewport_image_min = { 0, 0 };
    ImVec2 viewport_image_max = { 0, 0 };

    // Tab close confirmation state
    bool show_close_confirmation = false;
  }

  void init() {
    // Create framebuffer
    glGenFramebuffers(1, &framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);

    // Create texture to render to
    glGenTextures(1, &texture_color_buffer);
    glBindTexture(GL_TEXTURE_2D, texture_color_buffer);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, fb_width, fb_height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture_color_buffer, 0);

    // Create renderbuffer for depth and stencil
    glGenRenderbuffers(1, &renderbuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, renderbuffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, fb_width, fb_height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, renderbuffer);

    // Check framebuffer completeness
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
      Log::error("Framebuffer is not complete!");
    }

    // Unbind framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    Log::info("ViewportPanel initialized with size {} x {}", fb_width, fb_height);
  }

  void exit() {
    if (framebuffer != 0) {
      glDeleteFramebuffers(1, &framebuffer);
      framebuffer = 0;
    }
    if (texture_color_buffer != 0) {
      glDeleteTextures(1, &texture_color_buffer);
      texture_color_buffer = 0;
    }
    if (renderbuffer != 0) {
      glDeleteRenderbuffers(1, &renderbuffer);
      renderbuffer = 0;
    }
  }

  void begin_render() {
    // Bind framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);

    // Set viewport to framebuffer size
    glViewport(0, 0, fb_width, fb_height);

    // Clear the framebuffer
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  }

  void end_render() {
    // Unbind framebuffer (render to screen again)
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Restore viewport to window size
    int window_width = Engine::WindowManager::get_width();
    int window_height = Engine::WindowManager::get_height();
    glViewport(0, 0, window_width, window_height);
  }

  // Helper function to draw level tabs
  static void draw_level_tabs() {
    const auto& tabs = Engine::LevelTabManager::get_tabs();
    const auto& tab_order = Engine::LevelTabManager::get_tab_order();
    const std::string& active_tab_id = Engine::LevelTabManager::get_active_tab_id();

    // Track if we need to programmatically select a tab (e.g., after opening a new level)
    static std::string last_known_active;
    static bool needs_selection = false;

    if (active_tab_id != last_known_active) {
      needs_selection = true;
      last_known_active = active_tab_id;
    }

    if (tabs.empty()) {
      ImGui::TextDisabled("No levels open");
      needs_selection = false;
      last_known_active = "";
      return;
    }

    // Track which tab to close (by ID now)
    static std::string tab_id_pending_close;

    ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_Reorderable |
      ImGuiTabBarFlags_FittingPolicyScroll;

    if (ImGui::BeginTabBar("LevelTabs", tab_bar_flags)) {
      for (size_t i = 0; i < tab_order.size(); ++i) {
        const std::string& tab_id = tab_order[i];
        Engine::LevelTab* tab = Engine::LevelTabManager::get_tab(tab_id);
        if (!tab) continue;

        // Build tab label with dirty indicator
        std::string tab_label = tab->display_name;
        if (tab->is_dirty) {
          tab_label += " *";
        }
        tab_label += "###Tab" + std::to_string(i); // Unique ID for ImGui

        // Only use SetSelected when we need to programmatically select this tab
        ImGuiTabItemFlags item_flags = ImGuiTabItemFlags_None;
        if ((needs_selection && tab_id == active_tab_id) || tab->needs_selection) {
          item_flags |= ImGuiTabItemFlags_SetSelected;
          tab->needs_selection = false;  // Clear the flag after using it
        }

        bool tab_open = true;
        bool is_tab_selected = ImGui::BeginTabItem(tab_label.c_str(), &tab_open, item_flags);

        if (is_tab_selected) {
          ImGui::EndTabItem();

          // User clicked this tab - switch our internal state to match
          if (tab_id != active_tab_id) {
            Engine::LevelTabManager::switch_to_tab(tab_id);
            last_known_active = tab_id;
          }
        }

        // Handle tab close
        if (!tab_open) {
          if (tab->is_dirty) {
            // Show confirmation dialog
            tab_id_pending_close = tab_id;
            show_close_confirmation = true;
          }
          else {
            Engine::LevelTabManager::close_tab(tab_id);
          }
        }

        // Right-click context menu for tab
        if (ImGui::BeginPopupContextItem()) {
          bool is_new_level = tab_id.starts_with("__new_");

          if (ImGui::MenuItem("Save", "Ctrl+S")) {
            if (is_new_level) {
              // Need to prompt for save location
              auto save_path = Engine::FileIO::show_save_file_dialog("Save Level", "untitled.json", "Level Files (*.json)\0*.json\0", "json", Engine::Resources::resource_path + "/levels");
              if (save_path.has_value()) {
                Engine::LevelTabManager::save_tab_as(tab_id, save_path.value(), false);
              }
            }
            else {
              Engine::LevelTabManager::save_tab(tab_id);
            }
          }

          if (ImGui::MenuItem("Save As...")) {
            auto save_path = Engine::FileIO::show_save_file_dialog("Save Level As", "untitled.json", "Level Files (*.json)\0*.json\0", "json", Engine::Resources::resource_path + "/levels");
            if (save_path.has_value()) {
              Engine::LevelTabManager::save_tab_as(tab_id, save_path.value(), false);
            }
          }

          ImGui::Separator();

          if (ImGui::MenuItem("Close")) {
            if (tab->is_dirty) {
              tab_id_pending_close = tab_id;
              show_close_confirmation = true;
            }
            else {
              Engine::LevelTabManager::close_tab(tab_id);
            }
          }

          if (ImGui::MenuItem("Close Others")) {
            // Close all tabs except this one
            for (auto it2 = tab_order.rbegin(); it2 != tab_order.rend(); ++it2) {
              if (*it2 != tab_id) {
                Engine::LevelTabManager::close_tab(*it2, true);
              }
            }
          }

          if (ImGui::MenuItem("Close All")) {
            Engine::LevelTabManager::clear_all();
          }

          ImGui::EndPopup();
        }
      }

      // "+" button to create new level
      if (ImGui::TabItemButton("+", ImGuiTabItemFlags_Trailing | ImGuiTabItemFlags_NoTooltip)) {
        Engine::LevelTabManager::create_new_tab();
        needs_selection = true;
        last_known_active = Engine::LevelTabManager::get_active_tab_id();
      }
      if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Create New Level");
      }

      ImGui::EndTabBar();

      // Clear the selection request after processing
      needs_selection = false;
    }

    // Close confirmation modal
    if (show_close_confirmation && !tab_id_pending_close.empty()) {
      ImGui::OpenPopup("Close Level?");
    }

    if (ImGui::BeginPopupModal("Close Level?", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
      Engine::LevelTab* tab_to_close = Engine::LevelTabManager::get_tab(tab_id_pending_close);
      if (tab_to_close) {
        ImGui::Text("'%s' has unsaved changes.", tab_to_close->display_name.c_str());
        ImGui::Text("Do you want to save before closing?");
        ImGui::Separator();

        if (ImGui::Button("Save", ImVec2(80, 0))) {
          bool is_new_level = tab_id_pending_close.starts_with("__new_");
          if (is_new_level) {
            auto save_path = Engine::FileIO::show_save_file_dialog("Save Level", "untitled.json", "Level Files (*.json)\0*.json\0", "json", Engine::Resources::resource_path + "/levels");
            if (save_path.has_value()) {
              std::string new_id = Engine::LevelTabManager::save_tab_as(tab_id_pending_close, save_path.value(), false);
              if (!new_id.empty()) {
                Engine::LevelTabManager::close_tab(new_id);
              }
            }
          }
          else {
            Engine::LevelTabManager::save_tab(tab_id_pending_close);
            Engine::LevelTabManager::close_tab(tab_id_pending_close);
          }
          tab_id_pending_close = "";
          show_close_confirmation = false;
          ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();

        if (ImGui::Button("Don't Save", ImVec2(80, 0))) {
          Engine::LevelTabManager::close_tab(tab_id_pending_close, true);
          tab_id_pending_close = "";
          show_close_confirmation = false;
          ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();

        if (ImGui::Button("Cancel", ImVec2(80, 0))) {
          tab_id_pending_close = "";
          show_close_confirmation = false;
          ImGui::CloseCurrentPopup();
        }
      }
      else {
        tab_id_pending_close = "";
        show_close_confirmation = false;
        ImGui::CloseCurrentPopup();
      }
      ImGui::EndPopup();
    }
  }

  void draw_panel() {
    // Only show the viewport panel if panel viewport mode is enabled
    if (!Engine::FileIO::SettingsManager<Engine::FileIO::EngineSettings>::get().panel_viewport_mode) {
      return;
    }

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(5, 5));
    ImGui::SetNextWindowSize(ImVec2(1280, 720), ImGuiCond_FirstUseEver);
    ImGui::Begin("Viewport");

    // Draw level tabs at the top
    draw_level_tabs();

    // Get the remaining size for the viewport
    ImVec2 viewport_panel_size = ImGui::GetContentRegionAvail();

    // Calculate aspect ratio (16:9)
    float target_aspect = 16.0f / 9.0f;
    float panel_aspect = viewport_panel_size.x / viewport_panel_size.y;

    ImVec2 image_size = viewport_panel_size;

    // Adjust size to maintain aspect ratio
    if (panel_aspect > target_aspect) {
      // Panel is wider than target, fit to height
      image_size.x = viewport_panel_size.y * target_aspect;
    }
    else {
      // Panel is taller than target, fit to width
      image_size.y = viewport_panel_size.x / target_aspect;
    }

    // Center the image
    ImVec2 cursor_pos = ImGui::GetCursorPos();
    cursor_pos.x += (viewport_panel_size.x - image_size.x) * 0.5f;
    cursor_pos.y += (viewport_panel_size.y - image_size.y) * 0.5f;
    ImGui::SetCursorPos(cursor_pos);

    // Resize framebuffer if needed (limit resizing frequency for performance)
    static float resize_timer = 0.0f;
    static const float RESIZE_DELAY = 0.1f; // Resize at most every 0.1 seconds
    resize_timer += ImGui::GetIO().DeltaTime;

    if (resize_timer >= RESIZE_DELAY) {
      int new_width = static_cast<int>(image_size.x);
      int new_height = static_cast<int>(image_size.y);

      if (new_width != fb_width || new_height != fb_height) {
        fb_width = new_width > 0 ? new_width : 1;
        fb_height = new_height > 0 ? new_height : 1;

        // Resize framebuffer texture
        glBindTexture(GL_TEXTURE_2D, texture_color_buffer);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, fb_width, fb_height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);

        // Resize renderbuffer
        glBindRenderbuffer(GL_RENDERBUFFER, renderbuffer);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, fb_width, fb_height);

        resize_timer = 0.0f;
      }
    }

    // Display the framebuffer texture (flip vertically with UV coords)
    ImGui::Image(
      (ImTextureID)(intptr_t)texture_color_buffer,
      image_size,
      ImVec2(0, 1), // Top-left UV (flipped)
      ImVec2(1, 0)  // Bottom-right UV (flipped)
    );

    // Store the bounds after the image is drawn
    viewport_image_min = ImGui::GetItemRectMin();
    viewport_image_max = ImGui::GetItemRectMax();

    // Check if viewport is hovered or focused
    viewport_hovered = ImGui::IsItemHovered();
    viewport_focused = ImGui::IsWindowFocused();

    ImGui::End();
    ImGui::PopStyleVar();
  }


  Math::Vec4D get_viewport_bounds() {
    return Math::Vec4D(viewport_image_min.x, viewport_image_min.y,
      viewport_image_max.x, viewport_image_max.y);
  }

  bool is_viewport_hovered() {
    return viewport_hovered;
  }

  bool is_viewport_focused() {
    return viewport_focused;
  }

} // namespace ViewportPanel
