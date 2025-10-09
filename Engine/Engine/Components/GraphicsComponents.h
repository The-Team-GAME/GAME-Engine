#pragma once

#include "../Graphics/RendererInternal.h"
#include "../Graphics/Renderer.h"
#include <string>
#include <limits>
#include "../Reflection/Reflect.h"
#include "Time.h"
#include "../Resources.h"


namespace Components
{
  struct Texture
  {
    Engine::Graphics::Texture texture_handle;

    Texture() : texture_handle(nullptr) {}
    Texture(Engine::Graphics::Texture tex) : texture_handle(tex) {}


    struct Adaptor {
      std::string path;

      Adaptor serialize(const Texture& tex) const {
        Adaptor view;
        if (tex.texture_handle) {
          view.path = tex.texture_handle->path;
        }
        return view;
      }

      Texture deserialize() const {
        return Engine::Resources::load_texture(path);
      }
    };
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

    struct Adaptor {
      std::vector<std::array<glm::vec2, 4>> tex_coords_list;
      size_t drawIndex;

      Adaptor serialize(const TextureAtlas& atlas) const {
        Adaptor adaptor;
        adaptor.drawIndex = atlas.drawIndex;
        adaptor.tex_coords_list.reserve(atlas.subTextures.size());
        for (const auto& subTex : atlas.subTextures) {
          adaptor.tex_coords_list.push_back(subTex.tex_coords);
        }
        return adaptor;
      }

      TextureAtlas deserialize() const {
        TextureAtlas atlas(0, 0, 0, 0, 0);
        atlas.drawIndex = drawIndex;
        atlas.subTextures.reserve(tex_coords_list.size());
        for (const auto& coords : tex_coords_list) {
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

    struct Adaptor {
      std::vector<float> durations;
      std::vector<float> before_times;
      float startTime;
      float totalDuration;

      Adaptor serialize(const Animation& anim) const {
        Adaptor adaptor;
        adaptor.startTime = anim.startTime;
        adaptor.totalDuration = anim.totalDuration;
        adaptor.durations.reserve(anim.frames.size());
        adaptor.before_times.reserve(anim.frames.size());
        for (const auto& frame : anim.frames) {
          adaptor.durations.push_back(frame.duration);
          adaptor.before_times.push_back(frame.before_time);
        }
        return adaptor;
      }

      Animation deserialize() const {
        Animation anim(0, 0.0f);
        anim.startTime = startTime;
        anim.totalDuration = totalDuration;
        anim.frames.resize(durations.size());
        for (size_t i = 0; i < durations.size(); ++i) {
          anim.frames[i].duration = durations[i];
          anim.frames[i].before_time = before_times[i];
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

    struct Adaptor {
      std::string path;
      int font_size;

      Adaptor serialize(const Font& font) const {
        Adaptor adaptor;
        if (font.font_handle) {
          adaptor.path = font.font_handle->path;
          adaptor.font_size = font.font_handle->font_size;
        }
        return adaptor;
      }

      Font deserialize() const {
        return Engine::Resources::load_font(path, font_size);
      }
    };
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
    union
    {
      Math::Vec4D color;

      struct RGBA {
        float r;
        float g;
        float b;
        float a;
      } rgba;

    };

    Color() : color(1.0f, 1.0f, 1.0f, 1.0f) {}

    Color(const Math::Vec4D& c) : color(c) {}

    Color(float r_val, float g_val, float b_val, float a_val = 1.0f)
      : color(r_val, g_val, b_val, a_val) {
    }
  };

}

REFLECT(Components::Color, color)
REFLECT(Components::Transform2D, position, rotation, scale)
REFLECT(Components::Text, content)
REFLECT_ADAPTOR(Components::Texture, path)
REFLECT_ADAPTOR(Components::Font, path, font_size)
REFLECT_ADAPTOR(Components::TextureAtlas, tex_coords_list, drawIndex)
REFLECT_ADAPTOR(Components::Animation, durations, before_times, startTime, totalDuration)