#pragma once

#include "../Precompiled.h"
#include <memory> 
#include <unordered_map>
#include <array>
#include <string>

namespace Engine {
  namespace Graphics
  {
    // Used by Animation component
    struct Frame
    {
      float duration;   // Duration of this frame in seconds
      float before_time; // Time before this frame starts (for internal calculations)
    };

    // Used by TextureAtlas and Font components
    struct SubTexture
    {
      std::array<glm::vec2, 4> tex_coords; // UV coordinates in the atlas
    };

    // Used by Font component
    struct Glyph
    {
      std::array<glm::vec2, 4> tex_coords; // UV coordinates in the atlas
      glm::ivec2 size;
      glm::ivec2 bearing;
      unsigned int advance;
    };

    // Holds information about a loaded texture including its OpenGL ID and dimensions.
    struct TextureData
    {
      unsigned int id = 0; // OpenGL handle (0 means invalid)
      int width = 0;
      int height = 0;

      TextureData(std::string path);
      {
        // Load image using stb_image
        int channels;
        unsigned char* data = stbi_load(path.c_str(), &width, &height, &channels, 4);
        if (!data)
        {
          std::cerr << "Failed to load texture: " << path << std::endl;
          id = 0;
          width = height = 0;
          return;
        }

        // Generate OpenGL texture
        glGenTextures(1, &id);
        glBindTexture(GL_TEXTURE_2D, id);

        // Set texture parameters
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        // Upload texture data to GPU
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        // Free image memory on CPU
        stbi_image_free(data);
      }

      // Destructor to cleanup OpenGL texture
      ~TextureData()
      {
        if (id != 0)
        {
          glDeleteTextures(1, &id);
        }
      }

      // Delete copy constructor and assignment operator to prevent accidental copies
      TextureData(const TextureData&) = delete;
      TextureData& operator=(const TextureData&) = delete;

      // Allow move semantics
      TextureData(TextureData&& other) noexcept : id(other.id), width(other.width), height(other.height)
      {
        other.id = 0;
      }

      TextureData& operator=(TextureData&& other) noexcept
      {
        if (this != &other)
        {
          if (id != 0)
          {
            glDeleteTextures(1, &id);
          }
          id = other.id;
          width = other.width;
          height = other.height;
          other.id = 0;
        }
        return *this;
      }

      // Default constructor
      // TextureData() : id(0), width(0), height(0) {}
      // TextureData(unsigned int id, int width, int height) : id(id), width(width), height(height) {}
    };


    // Font resource data structure
    struct FontData
    {
      TextureData texture_atlas;                                 // Glyph atlas texture
      std::unordered_map<unsigned int, Graphics::Glyph> glyphs;  // Character -> glyph mapping
      unsigned int font_size = 0;                                    // Font size in pixels
      float baseline = 0.0f;                                            // Baseline offset
    };


    using Texture = std::shared_ptr<TextureData>;
    using Font = std::shared_ptr<FontData>;

  }
}