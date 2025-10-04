#include "RendererInternal.h"
#include <stdexcept>

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