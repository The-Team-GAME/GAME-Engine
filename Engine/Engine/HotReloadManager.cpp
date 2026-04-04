//-----------------------------------------------------------------------------------
// file:    HotReloadManager.cpp
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   Implementation of the hot-reload system for GameLogic DLL.
//          Uses Win32 LoadLibrary/FreeLibrary for DLL management and
//          file timestamps for change detection.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#include "HotReloadManager.h"
#include "Logger.h"
#include "SceneManager.h"
#include "ECS/World.h"
#include "Resources.h"
#include "FileIO/Settings.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <filesystem>
#include <atomic>
#include <thread>
#include <string>
#include <chrono>
#include <deque>
#include <mutex>

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
    static const char* PDB_NAME = "GameLogic.pdb";

    // Monotonically increasing counter so each reload gets a unique filename.
    // This avoids conflicts with files that Windows hasn't fully released yet.
    static int s_copy_counter = 0;

    // --------------------------------------------------------------------------

    // Cached exe directory — the exe path never changes at runtime
    static const std::filesystem::path& get_exe_dir()
    {
      static std::filesystem::path dir = []() {
        char exe_path[MAX_PATH];
        GetModuleFileNameA(nullptr, exe_path, MAX_PATH);
        return std::filesystem::path(exe_path).parent_path();
        }();
      return dir;
    }

    static std::filesystem::path get_dll_source_path()
    {
      return get_exe_dir() / DLL_NAME;
    }

    static std::filesystem::path get_dll_copy_path(int counter)
    {
      std::string name = "GameLogic_live_" + std::to_string(counter) + ".dll";
      return get_exe_dir() / name;
    }

    static std::filesystem::path get_pdb_source_path()
    {
      return get_exe_dir() / PDB_NAME;
    }

    static std::filesystem::path get_pdb_copy_path(int counter)
    {
      std::string name = "GameLogic_live_" + std::to_string(counter) + ".pdb";
      return get_exe_dir() / name;
    }

    // Best-effort cleanup of old live copies (skip files that are still locked)
    static void cleanup_old_copies(int current_counter)
    {
      const auto& dir = get_exe_dir();
      for (int i = 0; i < current_counter; ++i)
      {
        std::error_code ec;
        std::filesystem::remove(dir / ("GameLogic_live_" + std::to_string(i) + ".dll"), ec);
        std::filesystem::remove(dir / ("GameLogic_live_" + std::to_string(i) + ".pdb"), ec);
      }
    }

    // --------------------------------------------------------------------------
    // Source-file watcher: monitors GameLogic source directory for changes
    // and automatically triggers a rebuild of GameLogic.dll in a background thread.
    // --------------------------------------------------------------------------

    static std::thread          s_watcher_thread;
    static std::atomic<bool>    s_watcher_running{ false };
    static std::atomic<bool>    s_build_in_progress{ false };

    // Build cancellation: allows the watcher to kill a running msbuild
    // and have the build thread automatically restart.
    static std::atomic<void*>  s_build_process{ nullptr };   // HANDLE to msbuild
    static std::atomic<bool>   s_rebuild_requested{ false };
    static std::atomic<bool>   s_build_cancelled{ false };    // set when a build is killed

    // Build timing
    using Clock = std::chrono::steady_clock;
    static Clock::time_point   s_build_start_time{};
    static std::atomic<float>  s_last_build_duration{ 0.0f };
    static std::atomic<bool>   s_last_build_success{ false };

    // Build output streaming: pipe-captured stdout/stderr from msbuild
    static std::mutex              s_build_output_mutex;
    static std::deque<std::string> s_build_output;
    static std::string             s_current_build_file;   // last .cpp being compiled
    static int                     s_build_error_count = 0;

    // Tracked build thread (replaces .detach() for graceful shutdown)
    static std::thread             s_build_thread;
    static std::atomic<bool>       s_shutdown_requested{ false };

    // Manual reload: when true, check_and_reload() runs once even if
    // auto-reload is disabled in LocalSettings.
    static std::atomic<bool>       s_manual_reload_requested{ false };

    // Resolve the solution directory (two levels up from the exe)
    static std::filesystem::path get_solution_dir()
    {
      char exe_path[MAX_PATH];
      GetModuleFileNameA(nullptr, exe_path, MAX_PATH);
      // exe is at  <SolutionDir>/x64/<Config>/Engine.exe
      return std::filesystem::path(exe_path).parent_path().parent_path().parent_path();
    }

    static std::string get_active_configuration()
    {
      char exe_path[MAX_PATH];
      GetModuleFileNameA(nullptr, exe_path, MAX_PATH);
      // The parent folder of the exe is the configuration name (Debug/Release/Game)
      return std::filesystem::path(exe_path).parent_path().filename().string();
    }

    // Locate MSBuild.exe via vswhere (cached after first call).
    // This is necessary because the VS debugger doesn't inherit the
    // Developer Command Prompt PATH.
    static std::string find_msbuild_path()
    {
      static std::string cached;
      if (!cached.empty()) return cached;

      // vswhere lives at a well-known location
      const char* vswhere =
        "C:\\Program Files (x86)\\Microsoft Visual Studio\\Installer\\vswhere.exe";

      std::string cmd = "\"" + std::string(vswhere) + "\""
        " -latest -requires Microsoft.Component.MSBuild"
        " -find MSBuild\\**\\Bin\\MSBuild.exe";

      // Read stdout from vswhere
      FILE* pipe = _popen(cmd.c_str(), "r");
      if (pipe)
      {
        char buf[MAX_PATH]{};
        if (fgets(buf, sizeof(buf), pipe))
        {
          cached = buf;
          // Trim trailing newline
          while (!cached.empty() && (cached.back() == '\n' || cached.back() == '\r'))
            cached.pop_back();
        }
        _pclose(pipe);
      }

      if (cached.empty())
      {
        Log::warning("[HotReload] vswhere could not find MSBuild — falling back to PATH");
        cached = "msbuild";
      }
      else
      {
        Log::info("[HotReload] Found MSBuild: {}", cached);
      }
      return cached;
    }

    static void rebuild_game_logic_dll()
    {
      if (s_build_in_progress.exchange(true)) return; // already building

      do {
        s_rebuild_requested = false;
        s_build_start_time = Clock::now();

        // Clear build output for this run
        {
          std::lock_guard<std::mutex> lock(s_build_output_mutex);
          s_build_output.clear();
          s_current_build_file.clear();
          s_build_error_count = 0;
        }

        auto solution_dir = get_solution_dir();
        auto config = get_active_configuration();
        auto vcxproj = solution_dir / "Engine" / "GameLogic" / "GameLogic.vcxproj";
        auto msbuild = find_msbuild_path();

        // Build log goes next to the exe for easy inspection on failure
        auto log_file = solution_dir / "x64" / config / "hotreload_build.log";

        // Use a unique PDB path each build so the linker doesn't conflict with the
        // debugger holding a lock on previous PDBs.
        // The vcxproj uses $(HotReloadPDB) for ProgramDatabaseFile.
        // Use tick count for uniqueness across both builds and engine restarts.
        auto temp_pdb = solution_dir / "x64" / config
          / ("GameLogic_build_" + std::to_string(GetTickCount64()) + ".pdb");

        std::string cmd = "\"" + msbuild + "\""
          + " \"" + vcxproj.string() + "\""
          + " /p:Configuration=" + config
          + " /p:Platform=x64"
          + " /p:SolutionDir=\"" + solution_dir.string() + "\\\\\""
          + " /p:HotReloadPDB=\"" + temp_pdb.string() + "\""
          + " /v:minimal /nologo"
          + " /fl /flp:logfile=\"" + log_file.string() + "\";verbosity=normal";

        Log::info("[HotReload] Rebuilding GameLogic.dll ({})...", config);

        // ── Create pipe for capturing msbuild stdout/stderr ──────
        SECURITY_ATTRIBUTES sa{};
        sa.nLength = sizeof(sa);
        sa.bInheritHandle = TRUE;
        sa.lpSecurityDescriptor = nullptr;

        HANDLE hReadPipe = nullptr, hWritePipe = nullptr;
        BOOL pipe_ok = CreatePipe(&hReadPipe, &hWritePipe, &sa, 0);

        // Ensure the read end is NOT inherited by the child
        if (pipe_ok)
          SetHandleInformation(hReadPipe, HANDLE_FLAG_INHERIT, 0);

        STARTUPINFOA si{};
        PROCESS_INFORMATION pi{};
        si.cb = sizeof(si);
        si.dwFlags = STARTF_USESHOWWINDOW;
        si.wShowWindow = SW_HIDE; // Hidden window

        // If pipe was created, redirect stdout+stderr
        if (pipe_ok)
        {
          si.dwFlags |= STARTF_USESTDHANDLES;
          si.hStdOutput = hWritePipe;
          si.hStdError = hWritePipe;
          si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
        }

        std::string mutable_cmd(cmd); // CreateProcess needs non-const
        if (CreateProcessA(nullptr, mutable_cmd.data(), nullptr, nullptr,
          pipe_ok ? TRUE : FALSE, CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi))
        {
          s_build_process = pi.hProcess;  // expose handle for cancellation

          // Close the write end of the pipe in the parent so ReadFile
          // will return when the child exits (pipe broken).
          if (pipe_ok)
          {
            CloseHandle(hWritePipe);
            hWritePipe = nullptr;
          }

          // ── Read pipe output line by line ────────────────────────
          if (pipe_ok)
          {
            std::string line_buffer;
            char read_buf[512];
            DWORD bytes_read;

            while (ReadFile(hReadPipe, read_buf, sizeof(read_buf) - 1, &bytes_read, nullptr)
              && bytes_read > 0)
            {
              read_buf[bytes_read] = '\0';
              line_buffer += read_buf;

              // Process complete lines
              size_t pos;
              while ((pos = line_buffer.find('\n')) != std::string::npos)
              {
                std::string line = line_buffer.substr(0, pos);
                if (!line.empty() && line.back() == '\r') line.pop_back();

                {
                  std::lock_guard<std::mutex> lock(s_build_output_mutex);
                  s_build_output.push_back(line);

                  // Track current file being compiled (MSBuild minimal output: "  filename.cpp")
                  if (line.size() > 2 && line.ends_with(".cpp"))
                  {
                    // Extract just the filename
                    auto last_slash = line.find_last_of("\\/");
                    s_current_build_file = (last_slash != std::string::npos)
                      ? line.substr(last_slash + 1) : line;
                    // Trim leading whitespace
                    auto first = s_current_build_file.find_first_not_of(" \t");
                    if (first != std::string::npos)
                      s_current_build_file = s_current_build_file.substr(first);
                  }

                  // Count errors (MSVC pattern: ": error C")
                  if (line.find(": error ") != std::string::npos)
                    ++s_build_error_count;
                }

                line_buffer.erase(0, pos + 1);
              }
            }

            // Process any remaining partial line
            if (!line_buffer.empty())
            {
              if (!line_buffer.empty() && line_buffer.back() == '\r')
                line_buffer.pop_back();
              if (!line_buffer.empty())
              {
                std::lock_guard<std::mutex> lock(s_build_output_mutex);
                s_build_output.push_back(line_buffer);
                if (line_buffer.find(": error ") != std::string::npos)
                  ++s_build_error_count;
              }
            }
          }

          // Wait for the process to fully exit (should be instant since pipe is broken)
          WaitForSingleObject(pi.hProcess, INFINITE);
          s_build_process = nullptr;

          DWORD exit_code = 1;
          GetExitCodeProcess(pi.hProcess, &exit_code);
          CloseHandle(pi.hProcess);
          CloseHandle(pi.hThread);

          // If a rebuild was requested (source changed mid-build), skip
          // result reporting and restart immediately.
          if (s_rebuild_requested)
          {
            Log::info("[HotReload] Build cancelled — source changed, restarting...");            s_build_cancelled = true;            Sleep(300); // debounce: let file writes finish
            continue;
          }

          s_last_build_success = (exit_code == 0);
          if (exit_code == 0)
          {
            Log::info("[HotReload] Build succeeded");
            std::lock_guard<std::mutex> lock(s_build_output_mutex);
            s_current_build_file.clear();
          }
          else
            Log::error("[HotReload] Build FAILED (exit code {}) — see {}", exit_code, log_file.string());
        }
        else
        {
          if (pipe_ok)
          {
            CloseHandle(hWritePipe);
            hWritePipe = nullptr;
          }

          if (s_rebuild_requested)
          {
            Log::info("[HotReload] Build cancelled — source changed, restarting...");
            Sleep(300);
            continue;
          }
          s_last_build_success = false;
          Log::error("[HotReload] Failed to launch msbuild (error {})", GetLastError());
        }

        // Clean up read pipe
        if (pipe_ok && hReadPipe)
          CloseHandle(hReadPipe);

      } while (s_rebuild_requested && !s_shutdown_requested);

      auto elapsed = std::chrono::duration<float>(Clock::now() - s_build_start_time);
      s_last_build_duration = elapsed.count();
      s_build_in_progress = false;
    }

    static void source_watcher_thread_fn()
    {
      auto solution_dir = get_solution_dir();
      auto watch_dir = solution_dir / "Engine" / "GameLogic";

      // Use Win32 ReadDirectoryChangesW for efficient file watching
      HANDLE dir_handle = CreateFileW(
        watch_dir.wstring().c_str(),
        FILE_LIST_DIRECTORY,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        nullptr,
        OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED,
        nullptr);

      if (dir_handle == INVALID_HANDLE_VALUE)
      {
        Log::error("[HotReload] Cannot watch source directory: {}", watch_dir.string());
        return;
      }

      HANDLE event = CreateEventW(nullptr, TRUE, FALSE, nullptr);
      OVERLAPPED overlapped{};
      overlapped.hEvent = event;

      alignas(DWORD) char buffer[4096];

      while (s_watcher_running)
      {
        BOOL result = ReadDirectoryChangesW(
          dir_handle,
          buffer, sizeof(buffer),
          TRUE, // watch subtree
          FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_FILE_NAME,
          nullptr,
          &overlapped,
          nullptr);

        if (!result) break;

        // Wait with a timeout so we can check s_watcher_running periodically
        DWORD wait = WaitForSingleObject(event, 500);
        if (wait == WAIT_OBJECT_0)
        {
          DWORD bytes_returned = 0;
          GetOverlappedResult(dir_handle, &overlapped, &bytes_returned, FALSE);
          ResetEvent(event);

          if (bytes_returned == 0) continue;

          // Check if any .cpp or .h file changed
          bool source_changed = false;
          auto* info = reinterpret_cast<FILE_NOTIFY_INFORMATION*>(buffer);
          while (true)
          {
            std::wstring filename(info->FileName, info->FileNameLength / sizeof(WCHAR));
            if (filename.ends_with(L".cpp") || filename.ends_with(L".h"))
            {
              source_changed = true;
              break;
            }
            if (info->NextEntryOffset == 0) break;
            info = reinterpret_cast<FILE_NOTIFY_INFORMATION*>(
              reinterpret_cast<char*>(info) + info->NextEntryOffset);
          }

          if (source_changed)
          {
            // Only auto-build when the setting is enabled
            const auto& local = Engine::FileIO::SettingsManager<Engine::FileIO::LocalSettings>::get();
            if (!local.auto_reload) continue;

            if (s_build_in_progress)
            {
              // Cancel current build and request restart
              Log::info("[HotReload] Source changed during build — restarting build...");
              request_build_restart();
            }
            else
            {
              // Small debounce — wait for file writes to finish
              Sleep(300);
              build_game_logic_async();
            }
          }
        }
      }

      CloseHandle(event);
      CloseHandle(dir_handle);
    }

    void start_source_watcher()
    {
      if (s_watcher_running) return;
      s_watcher_running = true;
      s_watcher_thread = std::thread(source_watcher_thread_fn);
      Log::info("[HotReload] Source file watcher started");
    }

    void stop_source_watcher()
    {
      if (!s_watcher_running) return;
      s_watcher_running = false;
      if (s_watcher_thread.joinable())
        s_watcher_thread.join();
      Log::info("[HotReload] Source file watcher stopped");
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

      // Use a unique filename each reload so we never conflict with a
      // still-locked copy from a previous load.
      int counter = s_copy_counter++;
      auto copy_path = get_dll_copy_path(counter);
      auto pdb_copy = get_pdb_copy_path(counter);

      // The hot-reload build writes to a uniquely-named GameLogic_build_*.pdb
      // to avoid conflicts with the debugger. Find the newest one.
      std::filesystem::path pdb_src = get_pdb_source_path(); // fallback
      {
        auto out_dir = source.parent_path();
        std::filesystem::file_time_type newest_time{};
        for (auto& entry : std::filesystem::directory_iterator(out_dir))
        {
          auto fname = entry.path().filename().string();
          if (fname.starts_with("GameLogic_build_") && fname.ends_with(".pdb"))
          {
            auto wt = entry.last_write_time();
            if (wt > newest_time)
            {
              newest_time = wt;
              pdb_src = entry.path();
            }
          }
        }
      }

      // Clean up older copies that may now be unlocked
      cleanup_old_copies(counter);

      try
      {
        std::filesystem::copy_file(source, copy_path,
          std::filesystem::copy_options::overwrite_existing);
      }
      catch (const std::filesystem::filesystem_error& e)
      {
        Log::error("[HotReload] Failed to copy DLL: {}", e.what());
        return false;
      }

      try
      {
        if (std::filesystem::exists(pdb_src))
        {
          std::filesystem::copy_file(pdb_src, pdb_copy,
            std::filesystem::copy_options::overwrite_existing);
        }
      }
      catch (const std::filesystem::filesystem_error& e)
      {
        Log::warning("[HotReload] PDB copy failed (non-fatal): {}", e.what());
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

      // ── Purge stale component arrays ───────────────────────────────
      // Any ComponentArray<T> whose vtable lives inside the DLL must be
      // erased BEFORE FreeLibrary unmaps its pages.  This catches types
      // that were lazily registered by DLL code (via add_component /
      // get_component_array) but never listed in UnregisterGameLogic().
      {
        auto dll_base = reinterpret_cast<uintptr_t>(s_dll_handle);
        auto* dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(dll_base);
        auto* nt = reinterpret_cast<const IMAGE_NT_HEADERS*>(dll_base + dos->e_lfanew);
        uintptr_t dll_end = dll_base + nt->OptionalHeader.SizeOfImage;

        auto& arrays = ECS::Internal::get_containers().component_arrays;
        for (ECS::ComponentTypeID i = 0; i < ECS::MAX_COMPONENTS; ++i)
        {
          if (!arrays[i]) continue;
          // The vtable pointer is stored at the very start of the object.
          uintptr_t vptr = *reinterpret_cast<const uintptr_t*>(arrays[i].get());
          if (vptr >= dll_base && vptr < dll_end)
          {
            Log::warning("[HotReload] Removing stale component array (type id {}) — "
              "vtable was inside the unloaded DLL", i);
            arrays[i].reset();
          }
        }
      }

      FreeLibrary(s_dll_handle);
      s_dll_handle = nullptr;
      s_register_fn = nullptr;
      s_unregister_fn = nullptr;

      // Old live copy will be cleaned up by the next load_game_logic() call.
      // Trying to delete here is unreliable — Windows may still hold the lock.

      Log::info("[HotReload] GameLogic.dll unloaded");
    }

    // --------------------------------------------------------------------------

    void request_manual_reload()
    {
      s_manual_reload_requested = true;
      if (s_build_in_progress)
      {
        // Cancel the running build and restart, same as mid-build source change
        Log::info("[HotReload] Manual reload requested (F4) — restarting build...");
        request_build_restart();
      }
      else
      {
        Log::info("[HotReload] Manual reload requested (F4) — building...");
        build_game_logic_async();
      }
    }

    void request_build_cancellation()
    {
      if (!s_build_in_progress) return;

      Log::info("[HotReload] Build cancellation requested...");
      s_rebuild_requested = false;
      s_build_cancelled = true;

      HANDLE proc = static_cast<HANDLE>(s_build_process.load());
      if (proc) TerminateProcess(proc, 1);
    }

    void request_build_restart()
    {
      if (!s_build_in_progress) return;

      Log::info("[HotReload] Build restart requested...");
      s_rebuild_requested = true;
      s_build_cancelled = true;

      HANDLE proc = static_cast<HANDLE>(s_build_process.load());
      if (proc) TerminateProcess(proc, 1);
    }

    void check_and_reload()
    {
      // Throttle filesystem checks — no need to poll every frame
      static int frame_counter = 0;
      if (++frame_counter < 30) return;
      frame_counter = 0;

      // Don't attempt reload while msbuild is still writing the DLL
      if (s_build_in_progress) return;

      auto source = get_dll_source_path();

      if (!std::filesystem::exists(source)) return;

      auto current_write_time = std::filesystem::last_write_time(source);
      if (current_write_time <= s_last_write_time) return;

      // Skip auto-reload when disabled, unless a manual reload was requested
      const auto& local = Engine::FileIO::SettingsManager<Engine::FileIO::LocalSettings>::get();
      if (!local.auto_reload && !s_manual_reload_requested.exchange(false))
        return;

      Log::info("[HotReload] Change detected in GameLogic.dll — reloading...");

      // Wait until the DLL file is no longer locked (msbuild may have
      // just released it).  Try opening it exclusively; retry a few times.
      for (int attempt = 0; attempt < 20; ++attempt)
      {
        HANDLE h = CreateFileA(source.string().c_str(),
          GENERIC_READ, 0 /*no sharing*/, nullptr,
          OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (h != INVALID_HANDLE_VALUE)
        {
          CloseHandle(h);
          break;  // file is unlocked
        }
        Sleep(100);
      }

      // ── Save scene state ──────────────────────────────────────────
      // Remember which scene was active so we can reload it after the
      // DLL swap.  We must tear down the current scene while the OLD
      // DLL is still loaded because Scene subclasses and their vtables
      // live inside the DLL.
      std::string saved_scene = SceneManager::get_current_scene_name();

      // Unload the current scene properly (DLL vtables still valid)
      if (SceneManager::current_scene)
      {
        SceneManager::current_scene->free();
        SceneManager::current_scene->unload();
      }

      // Destroy all entities and flush immediately so component arrays
      // run their DLL-side destructors while the DLL is still loaded.
      ECS::destroy_all_entities();
      ECS::process_housekeeping();

      // ── Clear prefab cache ─────────────────────────────────────
      // Prefab component_storage holds IComponentArray instances whose
      // vtables live in the current DLL.  Must be destroyed BEFORE
      // the DLL is unloaded.
      Resources::prefab_cache.clear();
      std::erase_if(Resources::persistent_resources, [](const Resources::Resource& r) {
        return std::holds_alternative<ECS::Prefab>(r);
        });

      // ── DLL swap ──────────────────────────────────────────────────
      // Unload old DLL (UnregisterGameLogic clears scenes, functions,
      // component arrays, systems, collision callbacks).
      unload_game_logic();

      // Load new DLL (copies and loads; RegisterGameLogic re-registers
      // everything).
      if (load_game_logic())
      {
        // Reload prefabs so their component_storage uses the new DLL's vtables
        Resources::load_all_prefabs();

        // ── Restore scene ─────────────────────────────────────────
        // Reload the scene that was active before the swap.
        if (!saved_scene.empty() && saved_scene != "No current scene")
        {
          SceneManager::load_scene(saved_scene);
        }
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

    bool is_building()
    {
      return s_build_in_progress.load();
    }

    float get_build_elapsed_seconds()
    {
      if (!s_build_in_progress) return 0.0f;
      auto elapsed = std::chrono::duration<float>(Clock::now() - s_build_start_time);
      return elapsed.count();
    }

    float get_last_build_duration()
    {
      return s_last_build_duration.load();
    }

    bool last_build_succeeded()
    {
      return s_last_build_success.load();
    }

    bool consume_build_cancelled()
    {
      return s_build_cancelled.exchange(false);
    }

    std::string get_current_build_status()
    {
      std::lock_guard<std::mutex> lock(s_build_output_mutex);
      return s_current_build_file;
    }

    int get_build_error_count()
    {
      std::lock_guard<std::mutex> lock(s_build_output_mutex);
      return s_build_error_count;
    }

    std::deque<std::string> get_build_output()
    {
      std::lock_guard<std::mutex> lock(s_build_output_mutex);
      return s_build_output;
    }

    bool dll_exists()
    {
      return std::filesystem::exists(get_dll_source_path());
    }

    void build_game_logic()
    {
      rebuild_game_logic_dll();
    }

    void build_game_logic_async()
    {
      if (s_build_in_progress) return;
      // Join any previous build thread before starting a new one
      if (s_build_thread.joinable())
        s_build_thread.join();
      s_build_thread = std::thread(rebuild_game_logic_dll);
    }

    void stop_build()
    {
      // Signal shutdown so the build loop exits after current iteration
      s_shutdown_requested = true;

      request_build_cancellation();

      // Join the build thread so it doesn't outlive the process
      if (s_build_thread.joinable())
        s_build_thread.join();

      Log::info("[HotReload] Build thread stopped");
    }
  }
}


