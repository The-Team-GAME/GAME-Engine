//-----------------------------------------------------------------------------------
// file:    RendererInternal.cpp
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   Internal implementation details of the graphics renderer.
//          Contains FreeType font library management, texture data structures,
//          and low-level graphics utilities. Handles font loading and rendering
//          through the FreeType library integration.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#include "RendererInternal.h"
#include "../Resources.h"
#include <stdexcept>
#include <algorithm>
#include <vector>

#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "../../../Dependencies/stb/stb_image_resize2.h"

struct FreeTypeDeleter
{
  void operator()(FT_Library lib) const
  {
    if (lib)
    {
      FT_Done_FreeType(lib);
      // Log::info("FreeType library cleaned up");
    }
  }
};

// Use std::unique_ptr with the raw FT_Library handle
using UniqueFTLibrary = std::unique_ptr<
  std::remove_pointer_t<FT_Library>, // FT_Library is a pointer typedef, so we remove the pointer
  FreeTypeDeleter
>;

FT_Library Engine::Graphics::get_freetype_library()
{
  // The static variable stores the FT_Library handle
  static UniqueFTLibrary ft_ptr = []() -> UniqueFTLibrary {
    FT_Library lib_handle = nullptr; // Initialize a handle variable

    if (FT_Init_FreeType(&lib_handle))
    {
      return nullptr;
    }
    else
    {
      return UniqueFTLibrary(lib_handle);
    }
    }(); // Immediately execute the lambda

  // Throw an error if initialization failed, rather than returning a bad handle
  if (!ft_ptr) {
    throw std::runtime_error("FreeType library failed to initialize.");
  }

  // Return the raw handle (FT_Library is a typedef to a pointer)
  return ft_ptr.get();
}

// TextureData constructor
Engine::Graphics::TextureData::TextureData(const std::string& path, TextureFilter filtering)
  : filter(filtering), path(path)
{
  // Load image using stb_image
  int channels;
  unsigned char* data = stbi_load(path.c_str(), &width, &height, &channels, 4);
  if (!data)
  {
    Log::warning("Failed to load texture: {}", path);
    id = 0;
    width = 0;
    height = 0;
    return;
  }

  const int source_width = width;
  const int source_height = height;
  const unsigned char* upload_pixels = data;
  std::vector<unsigned char> resized_pixels;

  GLint max_texture_size = 0;
  glGetIntegerv(GL_MAX_TEXTURE_SIZE, &max_texture_size);
  if (max_texture_size > 0 && (width > max_texture_size || height > max_texture_size))
  {
    float scale = std::min(
      static_cast<float>(max_texture_size) / static_cast<float>(width),
      static_cast<float>(max_texture_size) / static_cast<float>(height)
    );
    int resized_width = std::max(1, static_cast<int>(width * scale));
    int resized_height = std::max(1, static_cast<int>(height * scale));
    resized_pixels.resize(static_cast<size_t>(resized_width) * static_cast<size_t>(resized_height) * 4u);

    if (!stbir_resize_uint8_linear(
      data,
      width,
      height,
      0,
      resized_pixels.data(),
      resized_width,
      resized_height,
      0,
      STBIR_RGBA))
    {
      Log::warning(
        "Failed to resize oversized texture before upload ({}x{}, max {}): {}",
        source_width,
        source_height,
        max_texture_size,
        path
      );
      stbi_image_free(data);
      id = 0;
      width = 0;
      height = 0;
      return;
    }

    Log::warning(
      "Texture exceeds GL_MAX_TEXTURE_SIZE ({}x{}, max {}). Downscaling to {}x{}: {}",
      source_width,
      source_height,
      max_texture_size,
      resized_width,
      resized_height,
      path
    );

    width = resized_width;
    height = resized_height;
    upload_pixels = resized_pixels.data();
  }

  // Generate OpenGL texture
  glGenTextures(1, &id);
  glBindTexture(GL_TEXTURE_2D, id);

  // Set texture parameters
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

  // Set filtering based on mode
  if (filtering == TextureFilter::Nearest) {
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  }
  else {
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  }

  // Upload texture data to GPU
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, upload_pixels);

  GLenum upload_error = glGetError();
  if (upload_error != GL_NO_ERROR)
  {
    Log::warning("Failed to upload texture to GPU (OpenGL error {}): {}", static_cast<unsigned int>(upload_error), path);
    glDeleteTextures(1, &id);
    stbi_image_free(data);
    id = 0;
    width = 0;
    height = 0;
    return;
  }

  // Only generate mipmaps for linear filtering
  if (filtering == TextureFilter::Linear) {
    glGenerateMipmap(GL_TEXTURE_2D);
  }

  // Free image memory on CPU
  stbi_image_free(data);
}

// TextureData default constructor
Engine::Graphics::TextureData::TextureData() : id(0), width(0), height(0) {}

// TextureData destructor
Engine::Graphics::TextureData::~TextureData()
{
  if (id != 0)
  {
    glDeleteTextures(1, &id);
  }
}

void Engine::Graphics::TextureData::set_filter(TextureFilter filtering)
{
  if (id == 0) return;

  filter = filtering;
  glBindTexture(GL_TEXTURE_2D, id);

  if (filtering == TextureFilter::Nearest) {
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  }
  else {
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glGenerateMipmap(GL_TEXTURE_2D);
  }
}

// FontData constructor
Engine::Graphics::FontData::FontData(const std::string& path, unsigned int size)
  : font_size(size), baseline(0.0f), path(path)
{
  constexpr int PADDING = 1; // Padding between glyphs in the atlas

  // Load font face
  FT_Face face;
  if (FT_New_Face(get_freetype_library(), path.c_str(), 0, &face))
  {
    Log::error("ERROR::FREETYPE: Failed to load font: {}", path);
    return;
  }

  FT_Set_Pixel_Sizes(face, 0, size);

  s32 atlas_width = 0;
  s32 atlas_height = 0;

  // First pass: calculate required atlas size
  for (unsigned char c = 0; c < 128; c++)
  {
    if (FT_Load_Char(face, c, FT_LOAD_RENDER))
      continue;
    atlas_width += face->glyph->bitmap.width + PADDING;
    atlas_height = std::max(atlas_height, static_cast<int>(face->glyph->bitmap.rows));
  }

  // Create RGBA atlas data
  std::vector<unsigned char> atlas_data(atlas_width * atlas_height * 4, 0);

  // Generate and configure OpenGL texture
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glGenTextures(1, &texture_atlas.id);
  glBindTexture(GL_TEXTURE_2D, texture_atlas.id);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

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
        { Math::Vec2D(x0, y1), Math::Vec2D(x1, y1), Math::Vec2D(x1, y0), Math::Vec2D(x0, y0) }, // UV coords (bottom-left, bottom-right, top-right, top-left)
        glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows),
        glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
        static_cast<u32>(face->glyph->advance.x)
    };

    glyphs.insert(std::pair<char, Glyph>(c, g));
    baseline = std::max(baseline, static_cast<f32>(g.bearing.y));

    currentX += face->glyph->bitmap.width + PADDING;
  }

  // Upload the complete RGBA atlas to GPU
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, atlas_width, atlas_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, atlas_data.data());

  FT_Done_Face(face);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

  Log::info("Font atlas created: {}x{} with {} glyphs", atlas_width, atlas_height, glyphs.size());
}


