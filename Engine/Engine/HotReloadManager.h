//-----------------------------------------------------------------------------------
// file:    HotReloadManager.h
// project: Monkey Stole My GPA
//
// brief:   Manages loading, unloading, and hot-reloading of the GameLogic DLL
//          at runtime. Watches for file changes and automatically reloads the
//          DLL when it detects a new build. Only active in Debug builds.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include "EngineExport.h"
#include <string>

namespace Engine
{
  namespace HotReload
  {
    // DLL function signatures
    using RegisterGameLogicFn = void(*)();
    using UnregisterGameLogicFn = void(*)();

    /**
     * @brief Loads GameLogic.dll from the output directory.
     *        Calls RegisterGameLogic() after loading.
     * @return True if loaded successfully.
     */
    ENGINE_API bool load_game_logic();

    /**
     * @brief Unloads GameLogic.dll.
     *        Calls UnregisterGameLogic() before unloading.
     */
    ENGINE_API void unload_game_logic();

    /**
     * @brief Checks if the DLL file has been modified since last load.
     *        If so, performs a hot-reload cycle:
     *        UnregisterGameLogic → FreeLibrary → copy DLL → LoadLibrary → RegisterGameLogic
     *        Should be called once per frame (e.g., from the main loop).
     */
    ENGINE_API void check_and_reload();

    /**
     * @brief Returns true if GameLogic.dll is currently loaded.
     */
    ENGINE_API bool is_loaded();

    /**
     * @brief Returns true if an msbuild rebuild is currently in progress.
     */
    ENGINE_API bool is_building();

    /**
     * @brief Returns the elapsed seconds since the current build started.
     *        Returns 0 if no build is in progress.
     */
    ENGINE_API float get_build_elapsed_seconds();

    /**
     * @brief Returns the duration (in seconds) of the last completed build.
     *        Returns 0 if no build has completed yet.
     */
    ENGINE_API float get_last_build_duration();

    /**
     * @brief Returns true if the last build succeeded, false if it failed.
     */
    ENGINE_API bool last_build_succeeded();

    /**
     * @brief Returns true if GameLogic.dll exists on disk in the output directory.
     */
    ENGINE_API bool dll_exists();

    /**
     * @brief Synchronously builds GameLogic.dll using msbuild.
     *        Blocks until the build completes.
     */
    ENGINE_API void build_game_logic();

    /**
     * @brief Asynchronously builds GameLogic.dll in a background thread.
     *        Use is_building() to check progress and check_and_reload()
     *        to detect when the new DLL is ready.
     */
    ENGINE_API void build_game_logic_async();

    /**
     * @brief Starts a background thread that watches GameLogic source files
     *        for changes and automatically triggers an msbuild rebuild.
     *        The existing check_and_reload() will then detect the new DLL.
     */
    ENGINE_API void start_source_watcher();

    /**
     * @brief Stops the source file watcher thread.
     */
    ENGINE_API void stop_source_watcher();
  }
}
