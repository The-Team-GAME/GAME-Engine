#pragma once

#include "../Graphics/RendererInternal.h"
#include "../Graphics/Renderer.h"
#include <string>
#include <limits>
#include "../Reflection/ReflectionLibrary.h"
#include "Time.h"
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
          r.path = tex.texture_handle->path;
        }
        return r;
      }

      static Texture create(const Reflector& r) {
        return Engine::Resources::load_texture(r.path);
      }
    };
  };

  struct TextureAtlas
  {
    std::vector<Engine::Graphics::SubTexture> subTextures;
    size_t drawIndex = 0;

    TextureAtlas(int subTextureWidth, int subTextureHeight, size_t rows, size_t cols, size_t totalSubTextures = 0)
      : subTextures(), drawIndex(0)
    {
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
      std::vector<std::array<glm::vec2, 4>> tex_coords_list;
      size_t drawIndex;

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
    f32 startTime = Engine::Time::get_time();
    float totalDuration = 0.0f;

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
          r.path = font.font_handle->path;
          r.font_size = font.font_handle->font_size;
        }
        return r;
      }

      static Font create(const Reflector& r) {
        return Engine::Resources::load_font(r.path, r.font_size);
      }
    };
  };

  struct Text
  {
    std::string content;

    Text() : content("") {}
    Text(const std::string& str) : content(str) {}
  };

  struct Color : public Math::Vec4D {
    f32& r;
    f32& g;
    f32& b;
    f32& a;

    Color(f32 red = 1.0f, f32 green = 1.0f, f32 blue = 1.0f, f32 alpha = 1.0f)
      : Math::Vec4D(red, green, blue, alpha), r(x), g(y), b(z), a(w) {
    }

    Color(const Color& other)
      : Math::Vec4D(other), r(x), g(y), b(z), a(w) {
    }

    Color(const Math::Vec4D& v)
      : Math::Vec4D(v), r(x), g(y), b(z), a(w) {
    }

    Color(const glm::vec4& v)
      : Math::Vec4D(v), r(x), g(y), b(z), a(w) {
    }

    Color& operator=(const Color& other) {
      if (this != &other) {
        Math::Vec4D::operator=(other);
      }
      return *this;
    }

    Color& operator=(const Math::Vec4D& v) {
      Math::Vec4D::operator=(v);
      return *this;
    }

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

    void clamp() {
      r = (r < 0.0f) ? 0.0f : (r > 1.0f) ? 1.0f : r;
      g = (g < 0.0f) ? 0.0f : (g > 1.0f) ? 1.0f : g;
      b = (b < 0.0f) ? 0.0f : (b > 1.0f) ? 1.0f : b;
      a = (a < 0.0f) ? 0.0f : (a > 1.0f) ? 1.0f : a;
    }

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

REFLECT_FIELDS(Components::Text, content)
REFLECT_CUSTOM(Components::Color, r, g, b, a)
REFLECT_CUSTOM(Components::Texture, path)
REFLECT_CUSTOM(Components::Font, path, font_size)
REFLECT_CUSTOM(Components::TextureAtlas, tex_coords_list, drawIndex)
REFLECT_CUSTOM(Components::Animation, durations, before_times, startTime, totalDuration)