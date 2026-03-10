#include "AudioSystem.h"

#include <fmod_studio.hpp>
#include <fmod.hpp>
#include <fmod_errors.h>
#include <unordered_map>
#include <algorithm>
#include "../Logger.h"
#include <filesystem>
#include <chrono>
#include <vector>


namespace Engine {
  namespace AudioSystem {

    // ------------------------------------------------------------
    // Internal FMOD state (private to this translation unit)
    // ------------------------------------------------------------
    static FMOD::Studio::System* gStudioSystem = nullptr;
    static FMOD::System* gCoreSystem = nullptr;

    // Resource tracking
    static ::std::unordered_map<::std::string, FMOD::Studio::Bank*> gBanks;
    static ::std::unordered_map<::std::string, FMOD::Studio::EventInstance*> gEventInstances;
    static ::std::unordered_map<::std::string, FMOD::Studio::EventDescription*> gEventDescriptions;
    static ::std::unordered_map<::std::string, FMOD::Studio::Bus*> gBuses;
    static ::std::unordered_map<::std::string, FMOD::Studio::VCA*> gVCAs;

    // 3D audio state
    static FMOD_3D_ATTRIBUTES gListenerAttributes = {};
    static float gMasterVolume = 1.0f;

    // Hot-reload state
    static ::std::unordered_map<::std::string, ::std::filesystem::file_time_type> gBankTimestamps;
    static ::std::unordered_map<::std::string, ::std::string> gBankAbsolutePaths; // original path -> absolute path
    static auto gLastHotReloadCheck = ::std::chrono::steady_clock::now();
    static constexpr float HOT_RELOAD_INTERVAL = 1.0f;  // seconds between polls
    static constexpr float HOT_RELOAD_STABLE_WAIT = 1.0f; // seconds timestamps must be stable before reload
    static ::std::chrono::steady_clock::time_point gChangeDetectedAt = {};
    static bool gPendingReload = false;
    static bool gHotReloadDiagnosticLogged = false;
    static constexpr const char* SCENE_BGM_INSTANCE = "scene_bgm";

    // Forward declarations
    static void checkBankHotReload();
    static bool FMOD_Check(FMOD_RESULT result, const char* context = nullptr);

    static bool stopAndReleaseInstanceIfExists(const ::std::string& instanceName, bool immediate, const char* context)
    {
      auto it = gEventInstances.find(instanceName);
      if (it == gEventInstances.end())
        return true;

      FMOD_STUDIO_STOP_MODE mode = immediate ? FMOD_STUDIO_STOP_IMMEDIATE : FMOD_STUDIO_STOP_ALLOWFADEOUT;
      bool stopOk = FMOD_Check(it->second->stop(mode), context);
      bool releaseOk = FMOD_Check(it->second->release(), context);
      gEventInstances.erase(it);
      return stopOk && releaseOk;
    }

    // ------------------------------------------------------------
    // FMOD error checking helper
    // ------------------------------------------------------------
    static bool FMOD_Check(FMOD_RESULT result, const char* context)
    {
      if (result != FMOD_OK)
      {
        if (context)
          Log::error("[AudioSystem] FMOD {}: {}", context, FMOD_ErrorString(result));
        else
          Log::error("[AudioSystem] FMOD error: {}", FMOD_ErrorString(result));
        return false;
      }
      return true;
    }

    // ------------------------------------------------------------
    // Convert Math::Vec2D to FMOD_VECTOR (2D in 3D space, z=0)
    // ------------------------------------------------------------
    static FMOD_VECTOR ToFMODVector(const Math::Vec2D& v)
    {
      FMOD_VECTOR fv;
      fv.x = -v.x;  // Negate X to match FMOD left/right orientation with world space
      fv.y = v.y;
      fv.z = 0.0f;
      return fv;
    }

    // ------------------------------------------------------------
    // Initialize FMOD Studio
    // ------------------------------------------------------------
    bool init()
    {
      if (gStudioSystem)
      {
        Log::info("[AudioSystem] Already initialized");
        return true;
      }

      if (!FMOD_Check(FMOD::Studio::System::create(&gStudioSystem), "System::create"))
        return false;

      if (!FMOD_Check(
        gStudioSystem->initialize(
          1024,
          FMOD_STUDIO_INIT_NORMAL,
          FMOD_INIT_NORMAL,
          nullptr), "System::initialize"))
        return false;

      FMOD_Check(gStudioSystem->getCoreSystem(&gCoreSystem), "getCoreSystem");

      // Initialize listener attributes
      // For a 2D side-view game: forward=(0,0,1) into the screen, up=(0,1,0).
      // This makes FMOD's computed right = +X, matching the game world.
      gListenerAttributes.position = { 0.0f, 0.0f, 0.0f };
      gListenerAttributes.velocity = { 0.0f, 0.0f, 0.0f };
      gListenerAttributes.forward = { 0.0f, 1.0f, 0.0f };
      gListenerAttributes.up = { 0.0f, 0.0f, 1.0f };

      if (gStudioSystem)
        gStudioSystem->setListenerAttributes(0, &gListenerAttributes);

      Log::info("[AudioSystem] FMOD initialized");
      return true;
    }

    // ------------------------------------------------------------
    // Update FMOD (must be called every frame)
    // ------------------------------------------------------------
    void update()
    {
      if (!gStudioSystem) return;

      // Periodically check for bank file changes
      auto now = ::std::chrono::steady_clock::now();
      float elapsed = ::std::chrono::duration<float>(now - gLastHotReloadCheck).count();
      if (elapsed >= HOT_RELOAD_INTERVAL) {
        gLastHotReloadCheck = now;
        checkBankHotReload();
      }

      gStudioSystem->update();
    }

    // ------------------------------------------------------------
    // Shutdown FMOD cleanly
    // ------------------------------------------------------------
    void shutdown()
    {
      if (!gStudioSystem)
        return;

      // Release all event instances
      releaseAllEventInstances();

      // Unload all banks
      unloadAllBanks();

      // Clear cached event descriptions
      gEventDescriptions.clear();

      // Clear buses and VCAs
      gBuses.clear();
      gVCAs.clear();

      // Release FMOD system
      gStudioSystem->release();
      gStudioSystem = nullptr;
      gCoreSystem = nullptr;
      gHotReloadDiagnosticLogged = false;
      gPendingReload = false;

      Log::info("[AudioSystem] FMOD shutdown");
    }

    // ------------------------------------------------------------
    // FMOD Core access (for subsystems that need raw PCM playback)
    // ------------------------------------------------------------
    void* getCoreSystem()
    {
      return static_cast<void*>(gCoreSystem);
    }

    // ------------------------------------------------------------
    // Bank Management
    // ------------------------------------------------------------
    bool loadBank(const ::std::string& path)
    {
      if (!gStudioSystem)
      {
        Log::warning("[AudioSystem] Not initialized");
        return false;
      }

      // Check if already loaded
      if (gBanks.find(path) != gBanks.end())
      {
        Log::info("[AudioSystem] Bank already loaded: {}", path);
        return true;
      }

      FMOD::Studio::Bank* bank = nullptr;

      if (!FMOD_Check(
        gStudioSystem->loadBankFile(
          path.c_str(),
          FMOD_STUDIO_LOAD_BANK_NORMAL,
          &bank), "loadBankFile"))
        return false;

      gBanks[path] = bank;

      // Record absolute path and timestamp for hot-reload detection
      try {
        auto abs = ::std::filesystem::absolute(path);
        gBankAbsolutePaths[path] = abs.string();
        gBankTimestamps[path] = ::std::filesystem::last_write_time(abs);
      }
      catch (const ::std::exception& e) {
        Log::warning("[AudioSystem] Could not record bank timestamp for '{}': {}", path, e.what());
      }

      Log::info("[AudioSystem] Loaded bank: {}", path);
      return true;
    }

    bool unloadBank(const ::std::string& path)
    {
      auto it = gBanks.find(path);
      if (it == gBanks.end())
      {
        Log::warning("[AudioSystem] Bank not loaded: {}", path);
        return false;
      }

      if (!FMOD_Check(it->second->unload(), "Bank::unload"))
        return false;

      gBanks.erase(it);
      gBankTimestamps.erase(path);
      gBankAbsolutePaths.erase(path);
      Log::info("[AudioSystem] Unloaded bank: {}", path);
      return true;
    }

    void unloadAllBanks()
    {
      for (auto& pair : gBanks)
      {
        pair.second->unload();
      }
      gBanks.clear();
      gBankTimestamps.clear();
      gBankAbsolutePaths.clear();
      Log::info("[AudioSystem] Unloaded all banks");
    }

    bool isBankLoaded(const ::std::string& path)
    {
      return gBanks.find(path) != gBanks.end();
    }

    // ------------------------------------------------------------
    // Bank Hot-Reload
    // ------------------------------------------------------------
    static void checkBankHotReload()
    {
      if (!gStudioSystem || gBankTimestamps.empty()) return;

      // One-time diagnostic: show which paths are being watched
      if (!gHotReloadDiagnosticLogged)
      {
        gHotReloadDiagnosticLogged = true;
        Log::info("[AudioSystem] Hot-reload watching {} bank(s):", gBankAbsolutePaths.size());
        for (auto& [orig, abs] : gBankAbsolutePaths)
          Log::info("  {}", abs);
      }

      // Snapshot current on-disk timestamps using stored absolute paths
      bool any_changed = false;
      ::std::unordered_map<::std::string, ::std::filesystem::file_time_type> currentTimes;
      for (auto& [path, recorded] : gBankTimestamps)
      {
        auto absIt = gBankAbsolutePaths.find(path);
        const ::std::string& absPath = (absIt != gBankAbsolutePaths.end()) ? absIt->second : path;
        try {
          auto t = ::std::filesystem::last_write_time(absPath);
          currentTimes[path] = t;
          if (t != recorded)
          {
            Log::info("[AudioSystem] Bank changed: {}", absPath);
            any_changed = true;
          }
        }
        catch (const ::std::exception& e) {
          // File temporarily inaccessible (being written) — treat as changed
          Log::warning("[AudioSystem] Cannot read timestamp for '{}': {} — assuming changed", absPath, e.what());
          any_changed = true;
        }
      }

      if (any_changed)
      {
        // Update our snapshot to the latest times
        for (auto& [path, t] : currentTimes)
          gBankTimestamps[path] = t;

        if (!gPendingReload)
        {
          // First detection — start the stability timer
          gPendingReload = true;
          gChangeDetectedAt = ::std::chrono::steady_clock::now();
          Log::info("[AudioSystem] Bank change detected, waiting for writes to finish...");
        }
        else
        {
          // Still changing — reset the stability timer
          gChangeDetectedAt = ::std::chrono::steady_clock::now();
        }
        return;
      }

      // No change detected this poll. If a reload is pending, check if stable long enough.
      if (!gPendingReload) return;

      float stable_secs = ::std::chrono::duration<float>(
        ::std::chrono::steady_clock::now() - gChangeDetectedAt).count();
      if (stable_secs < HOT_RELOAD_STABLE_WAIT) return;

      // Timestamps have been stable — proceed with reload
      gPendingReload = false;
      Log::info("[AudioSystem] Bank files stable, hot-reloading...");

      // Collect all loaded bank paths
      ::std::vector<::std::string> paths;
      paths.reserve(gBanks.size());
      for (auto& [path, _] : gBanks)
        paths.push_back(path);

      // Strings banks must load first so event name lookups work
      ::std::stable_sort(paths.begin(), paths.end(), [](const ::std::string& a, const ::std::string& b) {
        bool a_str = a.find(".strings.") != ::std::string::npos;
        bool b_str = b.find(".strings.") != ::std::string::npos;
        return a_str > b_str;
      });

      // Release all instances and caches (they reference the old banks)
      releaseAllEventInstances();
      gEventDescriptions.clear();
      gBuses.clear();
      gVCAs.clear();

      // Unload all banks
      for (auto& [_, bank] : gBanks)
        bank->unload();
      gBanks.clear();
      gBankTimestamps.clear();

      // Let FMOD process the unloads before loading new banks
      gStudioSystem->update();

      // Reload all banks in order
      for (auto& path : paths)
      {
        FMOD::Studio::Bank* bank = nullptr;
        if (FMOD_Check(
          gStudioSystem->loadBankFile(
            path.c_str(),
            FMOD_STUDIO_LOAD_BANK_NORMAL,
            &bank), "loadBankFile (hot reload)"))
        {
          gBanks[path] = bank;
          try {
            gBankTimestamps[path] = ::std::filesystem::last_write_time(path);
          }
          catch (...) {}
          Log::info("[AudioSystem] Reloaded bank: {}", path);
        }
        else
        {
          Log::error("[AudioSystem] Failed to reload bank: {}", path);
        }
      }

      Log::info("[AudioSystem] Hot-reload complete");
    }

    // ------------------------------------------------------------
    // Event Playback (One-shot)
    // ------------------------------------------------------------
    bool playEvent(const ::std::string& eventPath)
    {
      if (!gStudioSystem)
        return false;

      FMOD::Studio::EventDescription* desc = nullptr;

      // Check cache first
      auto cacheIt = gEventDescriptions.find(eventPath);
      if (cacheIt != gEventDescriptions.end())
      {
        desc = cacheIt->second;
      }
      else
      {
        if (!FMOD_Check(
          gStudioSystem->getEvent(eventPath.c_str(), &desc), "getEvent"))
          return false;

        gEventDescriptions[eventPath] = desc;
      }

      FMOD::Studio::EventInstance* instance = nullptr;
      if (!FMOD_Check(desc->createInstance(&instance), "createInstance"))
        return false;

      FMOD_Check(instance->start(), "start");
      FMOD_Check(instance->release(), "release");

      return true;
    }

    bool playEventAtPosition(const ::std::string& eventPath, const Math::Vec2D& position, float minDistance, float maxDistance)
    {
      if (!gStudioSystem)
        return false;

      FMOD::Studio::EventDescription* desc = nullptr;

      auto cacheIt = gEventDescriptions.find(eventPath);
      if (cacheIt != gEventDescriptions.end())
      {
        desc = cacheIt->second;
      }
      else
      {
        if (!FMOD_Check(
          gStudioSystem->getEvent(eventPath.c_str(), &desc), "getEvent"))
          return false;

        gEventDescriptions[eventPath] = desc;
      }

      FMOD::Studio::EventInstance* instance = nullptr;
      if (!FMOD_Check(desc->createInstance(&instance), "createInstance"))
        return false;

      // Set 3D attributes
      FMOD_3D_ATTRIBUTES attributes = {};
      attributes.position = ToFMODVector(position);
      attributes.velocity = { 0.0f, 0.0f, 0.0f };
      attributes.forward = { 0.0f, 1.0f, 0.0f };
      attributes.up = { 0.0f, 0.0f, 1.0f };

      FMOD_Check(instance->set3DAttributes(&attributes), "set3DAttributes");
      
      // Set min/max distance for 3D spatialization control
      // Larger minDistance = more center channel presence (less hard-panned)
      FMOD_Check(instance->setProperty(FMOD_STUDIO_EVENT_PROPERTY_MINIMUM_DISTANCE, minDistance), "setMinDistance");
      FMOD_Check(instance->setProperty(FMOD_STUDIO_EVENT_PROPERTY_MAXIMUM_DISTANCE, maxDistance), "setMaxDistance");
      
      FMOD_Check(instance->start(), "start");
      FMOD_Check(instance->release(), "release");

      return true;
    }

    // ------------------------------------------------------------
    // Event Instance Management (Persistent)
    // ------------------------------------------------------------
    bool createEventInstance(const ::std::string& instanceName, const ::std::string& eventPath)
    {
      if (!gStudioSystem)
        return false;

      // Check if instance already exists
      if (gEventInstances.find(instanceName) != gEventInstances.end())
      {
        Log::warning("[AudioSystem] Event instance already exists: {}", instanceName);
        return false;
      }

      FMOD::Studio::EventDescription* desc = nullptr;

      auto cacheIt = gEventDescriptions.find(eventPath);
      if (cacheIt != gEventDescriptions.end())
      {
        desc = cacheIt->second;
      }
      else
      {
        if (!FMOD_Check(
          gStudioSystem->getEvent(eventPath.c_str(), &desc), "getEvent"))
          return false;

        gEventDescriptions[eventPath] = desc;
      }

      FMOD::Studio::EventInstance* instance = nullptr;
      if (!FMOD_Check(desc->createInstance(&instance), "createInstance"))
        return false;

      gEventInstances[instanceName] = instance;
      Log::info("[AudioSystem] Created event instance: {}", instanceName);
      return true;
    }

    bool startEventInstance(const ::std::string& instanceName)
    {
      auto it = gEventInstances.find(instanceName);
      if (it == gEventInstances.end())
      {
        Log::warning("[AudioSystem] Event instance not found: {}", instanceName);
        return false;
      }

      return FMOD_Check(it->second->start(), "start");
    }

    bool stopEventInstance(const ::std::string& instanceName, bool immediate)
    {
      auto it = gEventInstances.find(instanceName);
      if (it == gEventInstances.end())
      {
        Log::warning("[AudioSystem] Event instance not found: {}", instanceName);
        return false;
      }

      FMOD_STUDIO_STOP_MODE mode = immediate ?
        FMOD_STUDIO_STOP_IMMEDIATE : FMOD_STUDIO_STOP_ALLOWFADEOUT;

      return FMOD_Check(it->second->stop(mode), "stop");
    }

    bool pauseEventInstance(const ::std::string& instanceName, bool pause)
    {
      auto it = gEventInstances.find(instanceName);
      if (it == gEventInstances.end())
      {
        Log::warning("[AudioSystem] Event instance not found: {}", instanceName);
        return false;
      }

      return FMOD_Check(it->second->setPaused(pause), "setPaused");
    }

    bool releaseEventInstance(const ::std::string& instanceName)
    {
      auto it = gEventInstances.find(instanceName);
      if (it == gEventInstances.end())
      {
        Log::warning("[AudioSystem] Event instance not found: {}", instanceName);
        return false;
      }

      FMOD_Check(it->second->release(), "release");
      gEventInstances.erase(it);
      Log::info("[AudioSystem] Released event instance: {}", instanceName);
      return true;
    }

    void releaseAllEventInstances()
    {
      for (auto& pair : gEventInstances)
      {
        pair.second->release();
      }
      gEventInstances.clear();
      Log::info("[AudioSystem] Released all event instances");
    }

    bool isEventInstancePlaying(const ::std::string& instanceName)
    {
      auto it = gEventInstances.find(instanceName);
      if (it == gEventInstances.end())
        return false;

      FMOD_STUDIO_PLAYBACK_STATE state;
      if (FMOD_Check(it->second->getPlaybackState(&state), "getPlaybackState"))
      {
        return state == FMOD_STUDIO_PLAYBACK_PLAYING ||
          state == FMOD_STUDIO_PLAYBACK_STARTING;
      }
      return false;
    }

    // ------------------------------------------------------------
    // Event Parameters
    // ------------------------------------------------------------
    bool setEventParameter(const ::std::string& instanceName, const ::std::string& paramName, float value)
    {
      auto it = gEventInstances.find(instanceName);
      if (it == gEventInstances.end())
      {
        Log::warning("[AudioSystem] Event instance not found: {}", instanceName);
        return false;
      }

      return FMOD_Check(
        it->second->setParameterByName(paramName.c_str(), value),
        "setParameterByName");
    }

    bool getEventParameter(const ::std::string& instanceName, const ::std::string& paramName, float& outValue)
    {
      auto it = gEventInstances.find(instanceName);
      if (it == gEventInstances.end())
      {
        Log::warning("[AudioSystem] Event instance not found: {}", instanceName);
        return false;
      }

      return FMOD_Check(
        it->second->getParameterByName(paramName.c_str(), &outValue),
        "getParameterByName");
    }

    // ------------------------------------------------------------
    // 3D Audio Positioning
    // ------------------------------------------------------------
    void setListenerPosition(const Math::Vec2D& position, float depth)
    {
      gListenerAttributes.position = ToFMODVector(position);
      gListenerAttributes.position.z = depth;

      if (gStudioSystem)
        gStudioSystem->setListenerAttributes(0, &gListenerAttributes);
    }

    void setListenerAttributes(const Math::Vec2D& position, const Math::Vec2D& velocity,
      const Math::Vec2D& forward, const Math::Vec2D& up)
    {
      gListenerAttributes.position = ToFMODVector(position);
      gListenerAttributes.velocity = ToFMODVector(velocity);
      gListenerAttributes.forward = ToFMODVector(forward);
      gListenerAttributes.up = ToFMODVector(up);

      if (gStudioSystem)
        gStudioSystem->setListenerAttributes(0, &gListenerAttributes);
    }

    bool setEventInstancePosition(const ::std::string& instanceName, const Math::Vec2D& position)
    {
      auto it = gEventInstances.find(instanceName);
      if (it == gEventInstances.end())
      {
        Log::warning("[AudioSystem] Event instance not found: {}", instanceName);
        return false;
      }

      FMOD_3D_ATTRIBUTES attributes;
      it->second->get3DAttributes(&attributes);
      attributes.position = ToFMODVector(position);

      return FMOD_Check(it->second->set3DAttributes(&attributes), "set3DAttributes");
    }

    bool setEventInstanceAttributes(const ::std::string& instanceName,
      const Math::Vec2D& position, const Math::Vec2D& velocity)
    {
      auto it = gEventInstances.find(instanceName);
      if (it == gEventInstances.end())
      {
        Log::warning("[AudioSystem] Event instance not found: {}", instanceName);
        return false;
      }

      FMOD_3D_ATTRIBUTES attributes;
      it->second->get3DAttributes(&attributes);
      attributes.position = ToFMODVector(position);
      attributes.velocity = ToFMODVector(velocity);

      return FMOD_Check(it->second->set3DAttributes(&attributes), "set3DAttributes");
    }

    // ------------------------------------------------------------
    // Volume Controls
    // ------------------------------------------------------------
    bool setEventInstanceVolume(const ::std::string& instanceName, float volume)
    {
      auto it = gEventInstances.find(instanceName);
      if (it == gEventInstances.end())
      {
        Log::warning("[AudioSystem] Event instance not found: {}", instanceName);
        return false;
      }

      return FMOD_Check(it->second->setVolume(volume), "setVolume");
    }

    bool setBusVolume(const ::std::string& busPath, float volume)
    {
      if (!gStudioSystem)
        return false;

      FMOD::Studio::Bus* bus = nullptr;

      // Check cache
      auto cacheIt = gBuses.find(busPath);
      if (cacheIt != gBuses.end())
      {
        bus = cacheIt->second;
      }
      else
      {
        if (!FMOD_Check(gStudioSystem->getBus(busPath.c_str(), &bus), "getBus"))
          return false;

        gBuses[busPath] = bus;
      }

      return FMOD_Check(bus->setVolume(volume), "Bus::setVolume");
    }

    bool setVCAVolume(const ::std::string& vcaPath, float volume)
    {
      if (!gStudioSystem)
        return false;

      FMOD::Studio::VCA* vca = nullptr;

      // Check cache
      auto cacheIt = gVCAs.find(vcaPath);
      if (cacheIt != gVCAs.end())
      {
        vca = cacheIt->second;
      }
      else
      {
        if (!FMOD_Check(gStudioSystem->getVCA(vcaPath.c_str(), &vca), "getVCA"))
          return false;

        gVCAs[vcaPath] = vca;
      }

      return FMOD_Check(vca->setVolume(volume), "VCA::setVolume");
    }

    bool getBusVolume(const ::std::string& busPath, float& outVolume)
    {
      if (!gStudioSystem)
        return false;

      FMOD::Studio::Bus* bus = nullptr;

      auto cacheIt = gBuses.find(busPath);
      if (cacheIt != gBuses.end())
      {
        bus = cacheIt->second;
      }
      else
      {
        if (!FMOD_Check(gStudioSystem->getBus(busPath.c_str(), &bus), "getBus"))
          return false;

        gBuses[busPath] = bus;
      }

      return FMOD_Check(bus->getVolume(&outVolume), "Bus::getVolume");
    }

    bool getVCAVolume(const ::std::string& vcaPath, float& outVolume)
    {
      if (!gStudioSystem)
        return false;

      FMOD::Studio::VCA* vca = nullptr;

      auto cacheIt = gVCAs.find(vcaPath);
      if (cacheIt != gVCAs.end())
      {
        vca = cacheIt->second;
      }
      else
      {
        if (!FMOD_Check(gStudioSystem->getVCA(vcaPath.c_str(), &vca), "getVCA"))
          return false;

        gVCAs[vcaPath] = vca;
      }

      return FMOD_Check(vca->getVolume(&outVolume), "VCA::getVolume");
    }

    bool playSceneMusic(const ::std::string& eventPath, float volume, bool immediateStop)
    {
      if (!gStudioSystem)
      {
        Log::warning("[AudioSystem] Not initialized");
        return false;
      }

      if (!stopAndReleaseInstanceIfExists(SCENE_BGM_INSTANCE, immediateStop, "scene_bgm stop/release"))
      {
        Log::warning("[AudioSystem] Continuing after scene BGM cleanup failure");
      }

      if (!createEventInstance(SCENE_BGM_INSTANCE, eventPath))
        return false;

      if (!setEventInstanceVolume(SCENE_BGM_INSTANCE, volume))
      {
        stopAndReleaseInstanceIfExists(SCENE_BGM_INSTANCE, true, "scene_bgm cleanup after set volume failure");
        return false;
      }

      if (!startEventInstance(SCENE_BGM_INSTANCE))
      {
        stopAndReleaseInstanceIfExists(SCENE_BGM_INSTANCE, true, "scene_bgm cleanup after start failure");
        return false;
      }

      return true;
    }

    bool stopSceneMusic(bool immediate)
    {
      return stopAndReleaseInstanceIfExists(SCENE_BGM_INSTANCE, immediate, "scene_bgm stop/release");
    }

    bool isSceneMusicPlaying()
    {
      return isEventInstancePlaying(SCENE_BGM_INSTANCE);
    }

    // ------------------------------------------------------------
    // Global Controls
    // ------------------------------------------------------------
    void pauseAll(bool pause)
    {
      for (auto& pair : gEventInstances)
      {
        pair.second->setPaused(pause);
      }
    }

    void stopAll(bool immediate)
    {
      FMOD_STUDIO_STOP_MODE mode = immediate ?
        FMOD_STUDIO_STOP_IMMEDIATE : FMOD_STUDIO_STOP_ALLOWFADEOUT;

      for (auto& pair : gEventInstances)
      {
        pair.second->stop(mode);
        pair.second->release();
      }
      gEventInstances.clear();
    }

    void setMasterVolume(float volume)
    {
      gMasterVolume = volume;

      if (gStudioSystem)
      {
        FMOD::Studio::Bus* masterBus = nullptr;
        if (FMOD_Check(gStudioSystem->getBus("bus:/", &masterBus), "getMasterBus"))
        {
          masterBus->setVolume(volume);
        }
      }
    }

    float getMasterVolume()
    {
      return gMasterVolume;
    }

    // Bus-based volume controls
    void setBGMVolume(float volume)
    {
      setBusVolume("bus:/BGM", volume);
    }

    void setSFXVolume(float volume)
    {
      setBusVolume("bus:/SFX", volume);
    }

    float getBGMVolume()
    {
      float vol = 1.0f;
      getBusVolume("bus:/BGM", vol);
      return vol;
    }

    float getSFXVolume()
    {
      float vol = 1.0f;
      getBusVolume("bus:/SFX", vol);
      return vol;
    }

    // ------------------------------------------------------------
    // Event Enumeration (for editor dropdowns)
    // ------------------------------------------------------------
    ::std::vector<::std::string> getAllEventPaths()
    {
      ::std::vector<::std::string> paths;
      if (!gStudioSystem)
        return paths;

      for (auto& [bankPath, bank] : gBanks)
      {
        int eventCount = 0;
        bank->getEventCount(&eventCount);
        if (eventCount <= 0)
          continue;

        ::std::vector<FMOD::Studio::EventDescription*> events(eventCount);
        int retrieved = 0;
        if (FMOD_Check(bank->getEventList(events.data(), eventCount, &retrieved), "getEventList"))
        {
          for (int i = 0; i < retrieved; ++i)
          {
            char pathBuf[512];
            int pathLen = 0;
            if (events[i]->getPath(pathBuf, sizeof(pathBuf), &pathLen) == FMOD_OK)
            {
              paths.emplace_back(pathBuf);
            }
          }
        }
      }

      ::std::sort(paths.begin(), paths.end());
      return paths;
    }

    ::std::vector<::std::string> getAllBusPaths()
    {
      ::std::vector<::std::string> paths;
      if (!gStudioSystem)
        return paths;

      for (auto& [bankPath, bank] : gBanks)
      {
        int busCount = 0;
        bank->getBusCount(&busCount);
        if (busCount <= 0)
          continue;

        ::std::vector<FMOD::Studio::Bus*> buses(busCount);
        int retrieved = 0;
        if (FMOD_Check(bank->getBusList(buses.data(), busCount, &retrieved), "getBusList"))
        {
          for (int i = 0; i < retrieved; ++i)
          {
            char pathBuf[512];
            int pathLen = 0;
            if (buses[i]->getPath(pathBuf, sizeof(pathBuf), &pathLen) == FMOD_OK)
            {
              paths.emplace_back(pathBuf);
            }
          }
        }
      }

      ::std::sort(paths.begin(), paths.end());
      return paths;
    }

    // ------------------------------------------------------------
    // Utility
    // ------------------------------------------------------------
    bool isInitialized()
    {
      return gStudioSystem != nullptr;
    }

    void flushCommandQueue()
    {
      if (gStudioSystem)
        gStudioSystem->flushCommands();
    }

  } // namespace AudioSystem
} // namespace Engine
