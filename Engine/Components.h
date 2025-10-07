#pragma once

#include "Graphics/RendererInternal.h"
#include "Graphics/Renderer.h"
#include <string> 
#include "Reflection/Reflect.h"
#include "Time.h"

/* ======================================= //
// Fixed string for compile-time tags     //
// ======================================= */
template<size_t N>
struct fixed_string {
  constexpr fixed_string(const char(&str)[N]) {
    std::copy_n(str, N, value);
  }
  char value[N];
};

namespace Components
{
  /* ====================== //
  // Identification tags    //
  // ====================== */
  template <fixed_string id>
  struct Tag {};

  struct ObjectId : std::basic_string<char> {
    using std::basic_string<char>::basic_string;
  };

  /* ====================== //
  // Rendering components   //
  // ====================== */
  struct Texture
  {
    Engine::Graphics::Texture texture_handle;

    Texture() : texture_handle(nullptr) {}
    Texture(Engine::Graphics::Texture tex) : texture_handle(tex) {}
  };

  struct TextureAtlas
  {
    std::vector<Engine::Graphics::SubTexture> subTextures;
    size_t drawIndex = 0; // Current subTexture index to draw

    // Constructor to initialize a texture atlas with evenly divided subTextures, works with multiple rows and columns
    TextureAtlas(int subTextureWidth, int subTextureHeight, size_t rows, size_t cols, size_t totalSubTextures = 0)
      : subTextures(), drawIndex(0)
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
  };

  struct Animation
  {
    std::vector<Engine::Graphics::Frame> frames;
    f32 startTime = Engine::Time::get_time();     // To get the elapsed time to calculate which frame to show
    float totalDuration = 0.0f; // Total duration of the animation

    // Evenly spaced frames
    Animation(size_t frameCount, float frameDuration) : frames(), startTime(0.0f), totalDuration(frameDuration* frameCount)
    {
      if (frameCount == 0)
        return;

      startTime = Engine::Time::get_time();

      frames.resize(frameCount);
      for (size_t i = 0; i < frameCount; ++i)
      {
        frames[i].duration = frameDuration;
        frames[i].before_time = i * frameDuration;
      }
    }
  };

  struct Font
  {
    Engine::Graphics::Font font_handle;

    Font() : font_handle(nullptr) {}
    Font(Engine::Graphics::Font fnt) : font_handle(fnt) {}
  };

  struct Text
  {

    std::string content;

    Text() : content("") {}
    Text(const std::string& str) : content(str) {}
  };


  struct Transform2D
  {

    Math::Vec2D position;
    float rotation = 0.0f; // In degrees
    Math::Vec2D scale = { 1.0f, 1.0f };

    Transform2D() : position(0.0f, 0.0f), rotation(0.0f), scale(1.0f, 1.0f) {}
    Transform2D(const Math::Vec2D& pos, float rot = 0.0f, const Math::Vec2D& scl = Math::Vec2D(1.0f, 1.0f))
      : position(pos), rotation(rot), scale(scl) {
    }
    Transform2D(float x, float y, float rot, float scaleX, float scaleY)
      : position(x, y), rotation(rot), scale(scaleX, scaleY) {
    }
  };


  struct Color
  {
    Math::Vec4D color;

    Color() : color(255.0f, 255.0f, 255.0f, 255.0f) {}
    Color(const Math::Vec4D& c) : color(c) {}
    Color(float r, float g, float b, float a = 255.0f) : color(r, g, b, a) {}
  };


  /* ====================== //
  // Physics components     //
  // ====================== */

  struct RigidBody {
    // Note to MX, feel free to change anything here im just adding this so all the demos work
    bool detect_collisions = true;
    Math::Vec2D linear_velocity = { 0.0f, 0.0f }; // Velocity in units per second
    bool is_kinematic = false; // If true, not affected by physics forces
    float mass = 1.0f; // Mass in kg
    Math::Vec2D linear_terminal_velocity = { 1000.0f, 1000.0f }; // TODO: Use the std library to get the max float value for default
    float rotation = 0.0f; // In degrees
    float rotation_terminal_velocity = 360.0f; // In degrees per second
    float angular_velocity = 0.0f; // In degrees per second
  };




  /* ====================== //
  // Gameplay components    //
  // ====================== */

  struct Health {
    s32 value, max;
    Health(s32 value = 100, s32 max = 100) : value(value), max(max) {}

    void heal(s32 amount) {
      value = std::min(value + amount, max);
    }

    void damage(s32 amount) {
      value = std::max(value - amount, 0);
    }

    bool is_dead() const {
      return value <= 0;
    }
  };

}

REFLECT(Components::Color, color)
REFLECT(Components::Transform2D, position, rotation, scale)
REFLECT(Components::Text, content)
REFLECT(Components::Animation, frames, startTime, totalDuration)
