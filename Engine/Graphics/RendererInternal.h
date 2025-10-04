#pragma once

#include "../Precompiled.h"
#include <memory> 
#include <unordered_map>
#include <array>
#include <string>
#include "../Logger.h"
#include "../TypeDef.h"

namespace Engine {
  namespace Graphics
  {
    FT_Library ft;

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

      TextureData(const std::string& path)
      {
        // Load image using stb_image
        int channels;
        unsigned char* data = stbi_load(path.c_str(), &width, &height, &channels, 4);
        if (!data)
        {
          Log::warning("Failed to load texture: " + path);
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
      TextureData(const TextureData&&) = delete;

      // Allow move semantics
      /*TextureData(TextureData&& other) noexcept : id(other.id), width(other.width), height(other.height)
      {
        other.id = 0;
      }*/

      /*TextureData& operator=(TextureData&& other) noexcept
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
      }*/

      // Default constructor
      TextureData() : id(0), width(0), height(0) {}
      // TextureData(unsigned int id, int width, int height) : id(id), width(width), height(height) {}
    };


    // Font resource data structure
    struct FontData
    {
      TextureData texture_atlas;                                 // Glyph atlas texture
      std::unordered_map<unsigned int, Graphics::Glyph> glyphs;  // Character -> glyph mapping
      unsigned int font_size = 0;                                    // Font size in pixels
      float baseline = 0.0f;                                            // Baseline offset

      FontData(const std::string& path, unsigned int size)
      {
        // FreeType should already be initialized

        // Load font face
        FT_Face face;
        if (FT_New_Face(ft, path.c_str(), 0, &face))
        {
          Log::warning("ERROR::FREETYPE: Failed to load font: " + path);
        }

        FT_Set_Pixel_Sizes(face, 0, size);

        s32 atlas_width = 0;
        s32 atlas_height = 0;

        // First pass: calculate required atlas size
        for (unsigned char c = 0; c < 128; c++)
        {
          if (FT_Load_Char(face, c, FT_LOAD_RENDER))
            continue;
          atlas_width += face->glyph->bitmap.width + 1; // +1 for padding
          atlas_height = std::max(atlas_height, static_cast<int>(face->glyph->bitmap.rows));
        }

        // Create RGBA atlas data
        std::vector<unsigned char> atlas_data(atlas_width * atlas_height * 4, 0);

        // Create atlas texture
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        u32 atlas_texture;
        glGenTextures(1, &atlas_texture);
        glBindTexture(GL_TEXTURE_2D, atlas_texture);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        font_size = size;
        baseline = 0.0f;
        texture_atlas.width = atlas_width;
        texture_atlas.height = atlas_height;

        // Second pass: pack characters into atlas
        s32 currentX = 0;
        for (unsigned char c = 0; c < 128; c++)
        {
          if (FT_Load_Char(face, c, FT_LOAD_RENDER))
            continue;

          // Copy glyph data to RGBA atlas
          if (face->glyph->bitmap.width > 0 && face->glyph->bitmap.rows > 0)
          {
            for (u32 y = 0; y < face->glyph->bitmap.rows; y++)
            {
              for (u32 x = 0; x < face->glyph->bitmap.width; x++)
              {
                s32 atlas_index = ((y * atlas_width) + (currentX + x)) * 4;
                s32 glyph_index = y * face->glyph->bitmap.width + x;

                unsigned char alpha = face->glyph->bitmap.buffer[glyph_index];

                // Set RGBA: white color with glyph alpha
                atlas_data[atlas_index + 0] = 255; // R
                atlas_data[atlas_index + 1] = 255; // G  
                atlas_data[atlas_index + 2] = 255; // B
                atlas_data[atlas_index + 3] = alpha; // A
              }
            }
          }

          // Calculate UV coordinates
          f32 x0 = static_cast<f32>(currentX) / atlas_width;
          f32 x1 = static_cast<f32>(currentX + face->glyph->bitmap.width) / atlas_width;
          f32 y0 = 0.0f;
          f32 y1 = static_cast<f32>(face->glyph->bitmap.rows) / atlas_height;

          // Store glyph info
          Glyph g = {
              { {x0, y0}, {x1, y0}, {x1, y1}, {x0, y1} }, // UV coords
              glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows),
              glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
              static_cast<u32>(face->glyph->advance.x)
          };

          glyphs.insert(std::pair<char, Glyph>(c, g));
          baseline = std::max(baseline, static_cast<f32>(g.bearing.y));

          currentX += face->glyph->bitmap.width + 1; // +1 for padding
        }

        // Upload the complete RGBA atlas to GPU
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, atlas_width, atlas_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, atlas_data.data());

        FT_Done_Face(face);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
      }


    };





    using Texture = std::shared_ptr<TextureData>;
    using Font = std::shared_ptr<FontData>;
  }
}