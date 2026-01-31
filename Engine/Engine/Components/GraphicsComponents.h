//-----------------------------------------------------------------------------------
// file:    GraphicsComponents.h
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   ECS component definitions for graphics rendering. Includes Texture,
//          Color, Animation, TextureAtlas, Text, and Distortion components.
//          Provides the data structures needed for sprite rendering, text display,
//          animation playback, and visual effects in the graphics pipeline.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include "../Graphics/RendererInternal.h"
#include "../Graphics/Renderer.h"
#include <string>
#include <limits>
#include "../Reflection/ReflectionLibrary.h"
#include "../ECS/Macros.h"  // Add this line
#include "../Time.h"
#include "../Resources.h"
#include "../Logger.h"

namespace Components
{
  struct Texture
  {
    Engine::Graphics::Texture texture_handle;

    Texture() : texture_handle(nullptr) {}
    Texture(Engine::Graphics::Texture tex) : texture_handle(tex) {}

    struct Reflector {
      std::string path;

      static Reflector reflect(const Texture& tex) {
        Reflector r;
        if (tex.texture_handle) {
          r.path = Engine::Resources::get_relative_path(tex.texture_handle->path);
        }
        return r;
      }

      static Texture create(const Reflector& r) {
        return Engine::Resources::load_texture(r.path, true);
      }
    };
  };

  struct TextureAtlas
  {
    std::vector<Engine::Graphics::SubTexture> subTextures;
    int drawIndex = -1; // Current subTexture index to draw, -1 means disabled

    TextureAtlas() : subTextures(), drawIndex(-1) {} // TODO: remove default constructor

    // Constructor to initialize a texture atlas with evenly divided subTextures, works with multiple rows and columns
    TextureAtlas(int subTextureWidth, int subTextureHeight, size_t rows, size_t cols, size_t totalSubTextures = 0)
      : subTextures(), drawIndex(-1)
    {
      // If totalSubTextures is 0, fill the entire atlas
      if (totalSubTextures == 0)
      {
        totalSubTextures = rows * cols;
      }
      subTextures.reserve(totalSubTextures);
      for (size_t row = 0; row < rows; ++row)
      {
        for (size_t col = 0; col < cols; ++col)
        {
          if (subTextures.size() >= totalSubTextures)
            break;

          float u0 = static_cast<float>(col * subTextureWidth) / (cols * subTextureWidth);
          float v0 = static_cast<float>(row * subTextureHeight) / (rows * subTextureHeight);
          float u1 = static_cast<float>((col + 1) * subTextureWidth) / (cols * subTextureWidth);
          float v1 = static_cast<float>((row + 1) * subTextureHeight) / (rows * subTextureHeight);

          Engine::Graphics::SubTexture subTexture;
          subTexture.tex_coords = { glm::vec2(u0, v1), glm::vec2(u1, v1), glm::vec2(u1, v0), glm::vec2(u0, v0) };
          subTextures.push_back(subTexture);
        }
      }
    }

    struct Reflector {
      std::vector<std::array<Math::Vec2D, 4>> tex_coords_list;
      int drawIndex;

      static Reflector reflect(const TextureAtlas& atlas) {
        Reflector r;
        r.drawIndex = atlas.drawIndex;
        r.tex_coords_list.reserve(atlas.subTextures.size());
        for (const auto& subTex : atlas.subTextures) {
          r.tex_coords_list.push_back(subTex.tex_coords);
        }
        return r;
      }

      static TextureAtlas create(const Reflector& r) {
        TextureAtlas atlas(0, 0, 0, 0, 0);
        atlas.drawIndex = r.drawIndex;
        atlas.subTextures.reserve(r.tex_coords_list.size());
        for (const auto& coords : r.tex_coords_list) {
          Engine::Graphics::SubTexture subTex;
          subTex.tex_coords = coords;
          atlas.subTextures.push_back(subTex);
        }
        return atlas;
      }
    };
  };

  struct Animation
  {
    std::vector<Engine::Graphics::Frame> frames;
    f32 startTime = Engine::Time::get_time(); // To get the elapsed time to calculate which frame to show
    float totalDuration = 0.0f; // Total duration of the animation
    bool is_looping = true; // Whether the animation should loop or stay at the last frame

    Animation() : frames(), startTime(Engine::Time::get_time()), totalDuration(0.0f), is_looping(true) {} // TODO: remove default constructor

    // Evenly spaced frames with sequential atlas indices starting from 0
    Animation(size_t frameCount, float frameDuration, bool looping = true) : frames(), startTime(0.0f), totalDuration(0.0f), is_looping(looping)
    {
      if (frameCount == 0)
        return;

      startTime = Engine::Time::get_time();
      frames.resize(frameCount);

      // Default: sequential indices 0, 1, 2, ...
      for (size_t i = 0; i < frameCount; ++i) {
        frames[i].atlas_index = static_cast<int>(i);
      }

      set_frame_even(frameDuration);
    }

    // Evenly spaced frames with specified atlas index range
    Animation(int startIndex, int endIndex, float frameDuration, bool looping = true)
      : frames(), startTime(0.0f), totalDuration(0.0f), is_looping(looping)
    {
      if (startIndex > endIndex) {
        Log::warning("Animation: startIndex > endIndex, swapping them");
        std::swap(startIndex, endIndex);
      }

      size_t frameCount = static_cast<size_t>(endIndex - startIndex + 1);
      startTime = Engine::Time::get_time();
      frames.resize(frameCount);

      // Fill with indices from startIndex to endIndex
      for (int i = 0; i < static_cast<int>(frameCount); ++i) {
        frames[i].atlas_index = startIndex + i;
      }

      set_frame_even(frameDuration);
    }

    // Custom frame durations with sequential atlas indices starting from 0
    Animation(std::initializer_list<float> durations, bool looping = true)
      : frames(), startTime(Engine::Time::get_time()), totalDuration(0.0f), is_looping(looping)
    {
      // Create frames based on the number of durations
      frames.resize(durations.size());

      // Default: sequential indices 0, 1, 2, ...
      for (size_t i = 0; i < durations.size(); ++i) {
        frames[i].atlas_index = static_cast<int>(i);
      }

      set_frame_durations(durations);
    }

    // Custom frame durations with specified atlas index range
    Animation(int startIndex, int endIndex, std::initializer_list<float> durations, bool looping = true)
      : frames(), startTime(Engine::Time::get_time()), totalDuration(0.0f), is_looping(looping)
    {
      if (startIndex > endIndex) {
        Log::warning("Animation: startIndex > endIndex, swapping them");
        std::swap(startIndex, endIndex);
      }

      size_t frameCount = static_cast<size_t>(endIndex - startIndex + 1);

      if (durations.size() != frameCount) {
        Log::warning("Animation: duration count doesn't match frame range, adjusting");
      }

      size_t actualFrameCount = std::min(frameCount, durations.size());
      frames.resize(actualFrameCount);

      // Fill with indices from startIndex to endIndex
      for (size_t i = 0; i < actualFrameCount; ++i) {
        frames[i].atlas_index = startIndex + static_cast<int>(i);
      }

      set_frame_durations(durations);
    }

    // Custom frame indices with even duration
    Animation(const std::vector<int>& indices, float frameDuration, bool looping = true)
      : frames(), startTime(Engine::Time::get_time()), totalDuration(0.0f), is_looping(looping)
    {
      frames.resize(indices.size());
      for (size_t i = 0; i < indices.size(); ++i) {
        frames[i].atlas_index = indices[i];
      }
      set_frame_even(frameDuration);
    }

    // Custom frame indices with custom durations
    Animation(const std::vector<int>& indices, const std::vector<float>& durations, bool looping = true)
      : frames(), startTime(Engine::Time::get_time()), totalDuration(0.0f), is_looping(looping)
    {
      if (indices.size() != durations.size()) {
        Log::warning("Animation: indices and durations size mismatch");
      }

      size_t frameCount = std::min(indices.size(), durations.size());
      frames.resize(frameCount);

      for (size_t i = 0; i < frameCount; ++i) {
        frames[i].atlas_index = indices[i];
      }

      set_frame_durations(durations);
    }

    // Check if the animation has completed one full cycle
    bool has_ended() const {
      float elapsed = Engine::Time::get_time() - startTime;
      return elapsed >= totalDuration;
    }

    // Get the elapsed time since animation started
    float get_elapsed_time() const {
      return Engine::Time::get_time() - startTime;
    }

    // Get the current frame index
    int get_current_frame_index() const {
      if (frames.empty()) {
        return 0;
      }

      float elapsed = Engine::Time::get_time() - startTime;
      float time_to_use = elapsed;

      // Handle looping vs non-looping animations
      if (is_looping) {
        time_to_use = fmod(elapsed, totalDuration);
      }
      else {
        if (time_to_use > totalDuration) {
          time_to_use = totalDuration;
        }
      }

      // Find which frame should be displayed based on elapsed time
      int current_frame = 0;
      for (int i = 0; i < static_cast<int>(frames.size()); ++i) {
        if (time_to_use >= frames[i].before_time) {
          current_frame = i;
        }
        else {
          break;
        }
      }

      return current_frame;
    }

    // Get the current atlas index for rendering
    int get_current_atlas_index() const {
      int frame_idx = get_current_frame_index();
      if (frame_idx >= 0 && frame_idx < static_cast<int>(frames.size())) {
        return frames[frame_idx].atlas_index;
      }
      return 0; // Default to first frame
    }

    // Set specific frame durations (allows different duration per frame)
    void set_frame_durations(const std::vector<float>& durations) {
      if (durations.empty() || durations.size() != frames.size()) {
        return; // Invalid input - must match frame count
      }

      totalDuration = 0.0f;
      for (size_t i = 0; i < frames.size(); ++i) {
        frames[i].duration = durations[i];
        frames[i].before_time = totalDuration;
        totalDuration += durations[i];
      }
    }

    // Set all frames to have the same duration (evenly distributed)
    void set_frame_even(float frameDuration) {
      if (frames.empty()) {
        return;
      }

      std::vector<float> even_durations(frames.size(), frameDuration);
      set_frame_durations(even_durations);
    }

    // Set specific frame durations using initializer list for convenience
    void set_frame_durations(std::initializer_list<float> durations) {
      set_frame_durations(std::vector<float>(durations));
    }

    struct Reflector {
      std::vector<float> durations;
      std::vector<float> before_times;
      std::vector<int> atlas_indices;
      float startTime;
      float totalDuration;
      bool is_looping;

      static Reflector reflect(const Animation& anim) {
        Reflector r;
        r.startTime = anim.startTime;
        r.totalDuration = anim.totalDuration;
        r.is_looping = anim.is_looping;
        r.durations.reserve(anim.frames.size());
        r.before_times.reserve(anim.frames.size());
        r.atlas_indices.reserve(anim.frames.size());
        for (const auto& frame : anim.frames) {
          r.durations.push_back(frame.duration);
          r.before_times.push_back(frame.before_time);
          r.atlas_indices.push_back(frame.atlas_index);
        }
        return r;
      }

      static Animation create(const Reflector& r) {
        Animation anim{};  // Use braced initialization to explicitly call default constructor
        anim.startTime = Engine::Time::get_time(); // Reset to current time when instantiating
        anim.totalDuration = r.totalDuration;
        anim.is_looping = r.is_looping;
        anim.frames.resize(r.durations.size());
        for (size_t i = 0; i < r.durations.size(); ++i) {
          anim.frames[i].duration = r.durations[i];
          anim.frames[i].before_time = r.before_times[i];
          anim.frames[i].atlas_index = r.atlas_indices[i];
          // // Use atlas_indices if available, otherwise default to sequential indices
          // // TODO: HOLIDAY: backwards compatibility, remove later
          // if (i < r.atlas_indices.size()) {
          //   anim.frames[i].atlas_index = r.atlas_indices[i];
          // }
          // else {
          //   anim.frames[i].atlas_index = static_cast<int>(i);
          // }
        }
        return anim;
      }
    };
  };

  struct Font
  {
    Engine::Graphics::Font font_handle;

    Font() : font_handle(nullptr) {}
    Font(Engine::Graphics::Font fnt) : font_handle(fnt) {}

    struct Reflector {
      std::string path;
      int font_size;

      static Reflector reflect(const Font& font) {
        Reflector r;
        if (font.font_handle) {
          r.path = Engine::Resources::get_relative_path(font.font_handle->path);
          r.font_size = font.font_handle->font_size;
        }
        return r;
      }

      static Font create(const Reflector& r) {
        return Engine::Resources::load_font(r.path, r.font_size, true);
      }
    };
  };

  // Text alignment options for wrapped text
  enum class TextAlignment {
    Left = 0,
    Center = 1,
    Right = 2
  };

  struct Text
  {
    std::string content;
    f32 font_scale = 1.0f;       // Scale of the text (independent from bounding box)
    f32 line_spacing = 1.2f;     // Line height multiplier (1.0 = no extra spacing)
    TextAlignment alignment = TextAlignment::Left;  // Text alignment for wrapped lines

    Text() : content(""), font_scale(1.0f), line_spacing(1.2f), alignment(TextAlignment::Left) {}
    Text(const std::string& str) : content(str), font_scale(1.0f), line_spacing(1.2f), alignment(TextAlignment::Left) {}
    Text(const std::string& str, f32 scale) : content(str), font_scale(scale), line_spacing(1.2f), alignment(TextAlignment::Left) {}
  };

  struct Camera
  {
    std::string name = "Camera";
    bool is_active = false;  // Whether this camera is currently active

    Camera() : name("Camera"), is_active(false) {}
    Camera(const std::string& cam_name, bool active = false)
      : name(cam_name), is_active(active) {
    }
  };

  struct Distortion
  {
    f32 intensity = 1.0f;      // Strength of the texture distortion effect (0.0 to 1.0+)
    f32 speed = 1.0f;          // Speed of the glitch animation
    f32 mesh_intensity = 0.0f; // Strength of the mesh/vertex distortion (0.0 to 1.0+)

    Distortion() : intensity(1.0f), speed(1.0f), mesh_intensity(0.0f) {}
    Distortion(f32 inten, f32 spd, f32 mesh_inten = 0.0f) : intensity(inten), speed(spd), mesh_intensity(mesh_inten) {}
  };

  struct Flip
  {
    Math::Vec2D value = { 1.0f, 1.0f }; // 1.0f = no flip, -1.0f = flip (x for horizontal, y for vertical)

    Flip() : value(1.0f, 1.0f) {}
    Flip(f32 x, f32 y) : value(x, y) {}
    Flip(const Math::Vec2D& v) : value(v) {}
  };

  struct Color : public Math::Vec4D {
    f32& r;
    f32& g;
    f32& b;
    f32& a;

    // Default constructor - creates white color (1, 1, 1, 1)
    Color(f32 red = 1.0f, f32 green = 1.0f, f32 blue = 1.0f, f32 alpha = 1.0f)
      : Math::Vec4D(red, green, blue, alpha), r(x), g(y), b(z), a(w) {
    }

    // Copy constructor
    Color(const Color& other)
      : Math::Vec4D(other), r(x), g(y), b(z), a(w) {
    }

    // Copy constructor from Vec4D
    Color(const Math::Vec4D& v)
      : Math::Vec4D(v), r(x), g(y), b(z), a(w) {
    }

    // Copy constructor from glm::vec4
    Color(const glm::vec4& v)
      : Math::Vec4D(v), r(x), g(y), b(z), a(w) {
    }

    // Copy assignment operator
    Color& operator=(const Color& other) {
      if (this != &other) {
        Math::Vec4D::operator=(other);
      }
      return *this;
    }

    // Assignment from Vec4D
    Color& operator=(const Math::Vec4D& v) {
      Math::Vec4D::operator=(v);
      return *this;
    }

    /* ===================== //
    // Convenience functions //
    // ===================== */
    void set_rgb(f32 red, f32 green, f32 blue) {
      r = red;
      g = green;
      b = blue;
    }

    void set_rgba(f32 red, f32 green, f32 blue, f32 alpha) {
      r = red;
      g = green;
      b = blue;
      a = alpha;
    }

    Math::Vec4D to_vec4d() const {
      return Math::Vec4D(r, g, b, a);
    }

    // Clamp color values to [0, 1] range
    void clamp() {
      r = (r < 0.0f) ? 0.0f : (r > 1.0f) ? 1.0f : r;
      g = (g < 0.0f) ? 0.0f : (g > 1.0f) ? 1.0f : g;
      b = (b < 0.0f) ? 0.0f : (b > 1.0f) ? 1.0f : b;
      a = (a < 0.0f) ? 0.0f : (a > 1.0f) ? 1.0f : a;
    }

    /* ================= //
    // Common colors     //
    // ================= */
    static Color White() { return Color(1.0f, 1.0f, 1.0f, 1.0f); }
    static Color Black() { return Color(0.0f, 0.0f, 0.0f, 1.0f); }
    static Color Red() { return Color(1.0f, 0.0f, 0.0f, 1.0f); }
    static Color Green() { return Color(0.0f, 1.0f, 0.0f, 1.0f); }
    static Color Blue() { return Color(0.0f, 0.0f, 1.0f, 1.0f); }
    static Color Yellow() { return Color(1.0f, 1.0f, 0.0f, 1.0f); }
    static Color Cyan() { return Color(0.0f, 1.0f, 1.0f, 1.0f); }
    static Color Magenta() { return Color(1.0f, 0.0f, 1.0f, 1.0f); }
    static Color Clear() { return Color(0.0f, 0.0f, 0.0f, 0.0f); }

    struct Reflector {
      f32 r;
      f32 g;
      f32 b;
      f32 a;

      static Reflector reflect(const Color& color) {
        Reflector ref = { color.r, color.g, color.b, color.a };
        return ref;
      }

      static Color create(const Reflector& ref) {
        return Color(ref.r, ref.g, ref.b, ref.a);
      }
    };
  };


}

REFLECT_ENUM(Components::TextAlignment, Left, Center, Right);
REGISTER_COMPONENT(Components::Text, content, font_scale, line_spacing, alignment)
REGISTER_COMPONENT(Components::Camera, name, is_active)
REGISTER_COMPONENT(Components::Distortion, intensity, speed, mesh_intensity)
REGISTER_COMPONENT(Components::Flip, value)
REGISTER_COMPONENT_CUSTOM(Components::Color, r, g, b, a)
REGISTER_COMPONENT_CUSTOM(Components::Texture, path)
REGISTER_COMPONENT_CUSTOM(Components::Font, path, font_size)
REGISTER_COMPONENT_CUSTOM(Components::TextureAtlas, tex_coords_list, drawIndex)
REGISTER_COMPONENT_CUSTOM(Components::Animation, durations, before_times, atlas_indices, startTime, totalDuration, is_looping)
