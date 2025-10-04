#include "RendererInternal.h"

// Custom deleter for FreeType library
struct FreeTypeDeleter
{
  void operator()(FT_Library* lib) const
  {
    if (lib && *lib)
    {
      FT_Done_FreeType(*lib);
      Log::info("FreeType library cleaned up");
    }
    delete lib;
  }
};

FT_Library& Engine::Graphics::get_freetype_library()
{
  static std::unique_ptr<FT_Library, FreeTypeDeleter> ft_ptr = []() {
    FT_Library* lib = new FT_Library();
    if (FT_Init_FreeType(lib))
    {
      Log::warning("Could not initialize FreeType library");
      *lib = nullptr;
    }
    else
    {
      Log::info("FreeType library initialized");
    }
    return std::unique_ptr<FT_Library, FreeTypeDeleter>(lib);
    }();

  return *ft_ptr;
}
