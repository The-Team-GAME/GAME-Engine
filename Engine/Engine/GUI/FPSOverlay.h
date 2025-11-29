//-----------------------------------------------------------------------------------
// file:    FPSOverlay.h
// project: Monkey Stole My GPA
//
// brief:   This file defines a simple overlay for displaying FPS using the game's
//          rendering system (draw_text_ui).
//
//-----------------------------------------------------------------------------------
#pragma once
#include "Graphics/Renderer.h"
#include "Resources.h"
#include "WindowManager.h"
#include "Time.h"
#include <string>
#include <iomanip>
#include <sstream>

namespace Engine {
  namespace GUI {

    inline void RenderFPSOverlay()
    {
      // Static variables to smooth the display updates (same as PerformanceViewerPanel)
      static f32 display_fps = 0.0f;
      static f32 update_timer = 0.0f;
      static s32 frame_count = 0;
      static f32 update_interval = 0.25f;

      update_timer += Engine::Time::get_real_dt();
      frame_count++;

      if (update_timer >= update_interval) {
        display_fps = frame_count / update_timer;
        update_timer = 0.0f;
        frame_count = 0;
      }

      // Load font (static so it's only loaded once)
      static auto font = Engine::Resources::load_font("fonts/Roboto-Regular.ttf", 24);

      if (!font) return; // Safety check

      // Format FPS text
      std::stringstream ss;
      ss << "FPS: " << std::fixed << std::setprecision(1) << display_fps;
      std::string fps_text = ss.str();

      // Determine color
      Math::Vec4D color = { 1.0f, 0.0f, 0.0f, 1.0f }; // Red
      if (display_fps >= 60.0f) {
        color = { 0.0f, 1.0f, 0.0f, 1.0f }; // Green
      }
      else if (display_fps >= 30.0f) {
        color = { 1.0f, 1.0f, 0.0f, 1.0f }; // Yellow
      }

      // Calculate position (Top Right)
      f32 scale = 1.0f;
      Math::Vec2D text_size = Engine::Graphics::measure_text(font, fps_text, scale);
      
      // 10px padding from right and top
      f32 x = Engine::Graphics::get_virtual_resolution().x - text_size.x - 10.0f;
      f32 y = 10.0f; 

      // Render text
      Engine::Graphics::draw_text_ui(font, fps_text, { x, y }, scale, color);
    }

  }
}
