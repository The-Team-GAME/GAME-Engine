//-----------------------------------------------------------------------------------
// file:    HotReloadManager.cpp
// project: Monkey Stole My GPA
//
// brief:   Implementation of the hot-reload system for GameLogic DLL.
//          Uses Win32 LoadLibrary/FreeLibrary for DLL management and
//          file timestamps for change detection.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------

#include "HotReloadManager.h"
#include "Logger.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <filesystem>

namespace Engine
{
  namespace HotReload
  {
    // Internal state
    static HMODULE               s_dll_handle = nullptr;
    static RegisterGameLogicFn   s_register_fn = nullptr;
    static UnregisterGameLogicFn s_unregister_fn = nullptr;
    static std::filesystem::file_time_type s_last_write_time{};

    // File paths
    static const char* DLL_NAME = "GameLogic.dll";
    static const char* DLL_COPY_NAME = "GameLogic_live.dll";
    static const char* PDB_NAME = "GameLogic.pdb";
    static const char* PDB_COPY_NAME = "GameLogic_live.pdb";

    // --------------------------------------------------------------------------

    static std::filesystem::path get_dll_source_path()
    {
      // GameLogic.dll is built to the same output directory as Engine.exe
      char exe_path[MAX_PATH];
      GetModuleFileNameA(nullptr, exe_path, MAX_PATH);
      return std::filesystem::path(exe_path).parent_path() / DLL_NAME;
    }

    static std::filesystem::path get_dll_copy_path()
    {
      char exe_path[MAX_PATH];
      GetModuleFileNameA(nullptr, exe_path, MAX_PATH);
      return std::filesystem::path(exe_path).parent_path() / DLL_COPY_NAME;
    }

    static std::filesystem::path get_pdb_source_path()
    {
      char exe_path[MAX_PATH];
      GetModuleFileNameA(nullptr, exe_path, MAX_PATH);
      return std::filesystem::path(exe_path).parent_path() / PDB_NAME;
    }

    static std::filesystem::path get_pdb_copy_path()
    {
      char exe_path[MAX_PATH];
      GetModuleFileNameA(nullptr, exe_path, MAX_PATH);
      return std::filesystem::path(exe_path).parent_path() / PDB_COPY_NAME;
    }

    // --------------------------------------------------------------------------

    bool load_game_logic()
    {
      auto source = get_dll_source_path();

      if (!std::filesystem::exists(source))
      {
        Log::error("[HotReload] GameLogic.dll not found at: {}", source.string());
        return false;
      }

      // Copy DLL and PDB so the originals remain unlocked for rebuilds
      auto copy_path = get_dll_copy_path();
      auto pdb_src = get_pdb_source_path();
      auto pdb_copy = get_pdb_copy_path();

      try
      {
        std::filesystem::copy_file(source, copy_path,
          std::filesystem::copy_options::overwrite_existing);

        if (std::filesystem::exists(pdb_src))
        {
          std::filesystem::copy_file(pdb_src, pdb_copy,
            std::filesystem::copy_options::overwrite_existing);
        }
      }
      catch (const std::filesystem::filesystem_error& e)
      {
        Log::error("[HotReload] Failed to copy DLL: {}", e.what());
        return false;
      }

      // Record timestamp
      s_last_write_time = std::filesystem::last_write_time(source);

      // Load the copy
      s_dll_handle = LoadLibraryA(copy_path.string().c_str());
      if (!s_dll_handle)
      {
        Log::error("[HotReload] LoadLibrary failed (error {})", GetLastError());
        return false;
      }

      // Resolve exports
      s_register_fn = reinterpret_cast<RegisterGameLogicFn>(
        GetProcAddress(s_dll_handle, "RegisterGameLogic"));
      s_unregister_fn = reinterpret_cast<UnregisterGameLogicFn>(
        GetProcAddress(s_dll_handle, "UnregisterGameLogic"));

      if (!s_register_fn)
      {
        Log::error("[HotReload] RegisterGameLogic not found in DLL");
        FreeLibrary(s_dll_handle);
        s_dll_handle = nullptr;
        return false;
      }

      // Call the registration function
      s_register_fn();

      Log::info("[HotReload] GameLogic.dll loaded successfully");
      return true;
    }

    // --------------------------------------------------------------------------

    void unload_game_logic()
    {
      if (!s_dll_handle) return;

      if (s_unregister_fn)
      {
        s_unregister_fn();
      }

      FreeLibrary(s_dll_handle);
      s_dll_handle = nullptr;
      s_register_fn = nullptr;
      s_unregister_fn = nullptr;

      Log::info("[HotReload] GameLogic.dll unloaded");
    }

    // --------------------------------------------------------------------------

    void check_and_reload()
    {
      auto source = get_dll_source_path();

      if (!std::filesystem::exists(source)) return;

      auto current_write_time = std::filesystem::last_write_time(source);
      if (current_write_time <= s_last_write_time) return;

      Log::info("[HotReload] Change detected in GameLogic.dll — reloading...");

      // Small delay to ensure the compiler has finished writing
      Sleep(200);

      // Unload old DLL
      unload_game_logic();

      // Load new DLL (copies and loads)
      if (load_game_logic())
      {
        Log::info("[HotReload] Hot-reload complete!");
      }
      else
      {
        Log::error("[HotReload] Hot-reload failed — game logic unavailable");
      }
    }

    // --------------------------------------------------------------------------

    bool is_loaded()
    {
      return s_dll_handle != nullptr;
    }
  }
}
