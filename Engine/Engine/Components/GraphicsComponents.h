#pragma once

#include "../Graphics/RendererInternal.h"
#include "../Graphics/Renderer.h"
#include <string>
#include <limits>
#include "../Reflection/ReflectionLibrary.h"
#include "../ECS/Macros.h"  // Add this line
#include "../Time.h"
#include "../Resources.h"


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

    Animation() : frames(), startTime(Engine::Time::get_time()), totalDuration(0.0f) {} // TODO: remove default constructor

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

    struct Reflector {
      std::vector<float> durations;
      std::vector<float> before_times;
      float startTime;
      float totalDuration;

      static Reflector reflect(const Animation& anim) {
        Reflector r;
        r.startTime = anim.startTime;
        r.totalDuration = anim.totalDuration;
        r.durations.reserve(anim.frames.size());
        r.before_times.reserve(anim.frames.size());
        for (const auto& frame : anim.frames) {
          r.durations.push_back(frame.duration);
          r.before_times.push_back(frame.before_time);
        }
        return r;
      }

      static Animation create(const Reflector& r) {
        Animation anim(0, 0.0f);
        anim.startTime = r.startTime;
        anim.totalDuration = r.totalDuration;
        anim.frames.resize(r.durations.size());
        for (size_t i = 0; i < r.durations.size(); ++i) {
          anim.frames[i].duration = r.durations[i];
          anim.frames[i].before_time = r.before_times[i];
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

  struct Text
  {
    std::string content;

    Text() : content("") {}
    Text(const std::string& str) : content(str) {}
  };

  struct Camera
  {
    std::string name = "Camera";
    Math::Vec2D position = { 0.0f, 0.0f };
    f32 rotation = 0.0f;  // In degrees
    f32 zoom = 1.0f;
    bool is_active = false;  // Whether this camera is currently active

    Camera() : name("Camera"), position(0.0f, 0.0f), rotation(0.0f), zoom(1.0f), is_active(false) {}
    Camera(const std::string& cam_name, const Math::Vec2D& pos = { 0.0f, 0.0f }, f32 rot = 0.0f, f32 zm = 1.0f, bool active = false)
      : name(cam_name), position(pos), rotation(rot), zoom(zm), is_active(active) {
    }
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

REGISTER_COMPONENT(Components::Text, content)
REGISTER_COMPONENT(Components::Camera, name, position, rotation, zoom, is_active)
REGISTER_COMPONENT_CUSTOM(Components::Color, r, g, b, a)
REGISTER_COMPONENT_CUSTOM(Components::Texture, path)
REGISTER_COMPONENT_CUSTOM(Components::Font, path, font_size)
REGISTER_COMPONENT_CUSTOM(Components::TextureAtlas, tex_coords_list, drawIndex)
REGISTER_COMPONENT_CUSTOM(Components::Animation, durations, before_times, startTime, totalDuration)