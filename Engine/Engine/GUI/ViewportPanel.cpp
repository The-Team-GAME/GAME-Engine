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
#include <imgui.h>
#include "../Precompiled.h"
#include "../Logger.h"
#include "../Graphics/Renderer.h"
#include "../WindowManager.h"

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

    Log::info(std::string("ViewportPanel initialized with size ") + std::to_string(fb_width) + " x " + std::to_string(fb_height));
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

  void draw_panel() {
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::Begin("Viewport");

    // Get the size of the window
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

    // Check if viewport is hovered or focused
    viewport_hovered = ImGui::IsItemHovered();
    viewport_focused = ImGui::IsWindowFocused();

    ImGui::End();
    ImGui::PopStyleVar();
  }

  bool is_viewport_hovered() {
    return viewport_hovered;
  }

  bool is_viewport_focused() {
    return viewport_focused;
  }

} // namespace ViewportPanel
