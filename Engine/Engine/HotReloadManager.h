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
    }
}
