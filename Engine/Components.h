#pragma once

#include "Graphics/RendererInternal.h"
#include "Graphics/Renderer.h"
#include <string>
#include <refl.hpp>


// create a class to hold runtime type information
class TypeInfo
{
public:

  // instances can be obtained only through calls to Get()
  template <typename T>
  static const TypeInfo& Get()
  {
    // here we create the singleton instance for this particular type
    static const TypeInfo ti(refl::reflect<T>());
    return ti;
  }

  const std::string& Name() const
  {
    return name_;
  }

private:

  // were only storing the name for demonstration purposes,
  // but this can be extended to hold other properties as well
  std::string name_;

  // given a type_descriptor, we construct a TypeInfo
  // with all the metadata we care about (currently only name)
  template <typename T, typename... Fields>
  TypeInfo(refl::type_descriptor<T> td)
    : name_(td.name)
  {
  }
};

// we will be using this base interface to specify that a type is
// reflectable at runtime using our custom system above
class Reflectable
{
public:
  virtual const TypeInfo& GetTypeInfo() const = 0;
};

// define a convenience macro to autoimplement GetTypeInfo()
#define MYLIB_REFLECTABLE() \
    virtual const TypeInfo& GetTypeInfo() const override \
    { \
        return TypeInfo::Get<::refl::trait::remove_qualifiers_t<decltype(*this)>>(); \
    }


namespace Components
{
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
    float startTime = 0.0f;     // To get the elapsed time to calculate which frame to show
    float totalDuration = 0.0f; // Total duration of the animation

    // Evenly spaced frames
    Animation(size_t frameCount, float frameDuration) : frames(), startTime(0.0f), totalDuration(frameDuration* frameCount)
    {
      if (frameCount == 0)
        return;

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

  struct Text : Reflectable
  {
    MYLIB_REFLECTABLE();

    std::string content;

    Text() : content("") {}
    Text(const std::string& str) : content(str) {}
  };


  struct Transform2D : Reflectable
  {
    MYLIB_REFLECTABLE();

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

}

REFL_AUTO(type(Components::Text))
REFL_AUTO(type(Components::Transform2D))
