#include "AudioSystem.h"

#include <fmod_studio.hpp>
#include <fmod.hpp>
#include <fmod_errors.h>
#include <unordered_map>
#include <algorithm>
#include <iostream>
#include <string>


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

    // ------------------------------------------------------------
    // FMOD error checking helper
    // ------------------------------------------------------------
    static bool FMOD_Check(FMOD_RESULT result, const char* context = nullptr)
    {
      if (result != FMOD_OK)
      {
        if (context)
          ::std::cout << "[FMOD ERROR] " << context << ": ";
        else
          ::std::cout << "[FMOD ERROR] ";

        ::std::cout << FMOD_ErrorString(result) << "\n";
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
      fv.x = v.x;
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
        ::std::cout << "[AudioSystem] Already initialized\n";
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
      gListenerAttributes.position = { 0.0f, 0.0f, 0.0f };
      gListenerAttributes.velocity = { 0.0f, 0.0f, 0.0f };
      gListenerAttributes.forward = { 0.0f, 1.0f, 0.0f };
      gListenerAttributes.up = { 0.0f, 0.0f, 1.0f };

      if (gStudioSystem)
        gStudioSystem->setListenerAttributes(0, &gListenerAttributes);

      ::std::cout << "[AudioSystem] FMOD initialized\n";
      return true;
    }

    // ------------------------------------------------------------
    // Update FMOD (must be called every frame)
    // ------------------------------------------------------------
    void update()
    {
      if (gStudioSystem)
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

      ::std::cout << "[AudioSystem] FMOD shutdown\n";
    }

    // ------------------------------------------------------------
    // Bank Management
    // ------------------------------------------------------------
    bool loadBank(const ::std::string& path)
    {
      if (!gStudioSystem)
      {
        ::std::cout << "[AudioSystem] Not initialized\n";
        return false;
      }

      // Check if already loaded
      if (gBanks.find(path) != gBanks.end())
      {
        ::std::cout << "[AudioSystem] Bank already loaded: " << path << "\n";
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
      ::std::cout << "[AudioSystem] Loaded bank: " << path << "\n";
      return true;
    }

    bool unloadBank(const ::std::string& path)
    {
      auto it = gBanks.find(path);
      if (it == gBanks.end())
      {
        ::std::cout << "[AudioSystem] Bank not loaded: " << path << "\n";
        return false;
      }

      if (!FMOD_Check(it->second->unload(), "Bank::unload"))
        return false;

      gBanks.erase(it);
      ::std::cout << "[AudioSystem] Unloaded bank: " << path << "\n";
      return true;
    }

    void unloadAllBanks()
    {
      for (auto& pair : gBanks)
      {
        pair.second->unload();
      }
      gBanks.clear();
      ::std::cout << "[AudioSystem] Unloaded all banks\n";
    }

    bool isBankLoaded(const ::std::string& path)
    {
      return gBanks.find(path) != gBanks.end();
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

    bool playEventAtPosition(const ::std::string& eventPath, const Math::Vec2D& position)
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
        ::std::cout << "[AudioSystem] Event instance already exists: " << instanceName << "\n";
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
      ::std::cout << "[AudioSystem] Created event instance: " << instanceName << "\n";
      return true;
    }

    bool startEventInstance(const ::std::string& instanceName)
    {
      auto it = gEventInstances.find(instanceName);
      if (it == gEventInstances.end())
      {
        ::std::cout << "[AudioSystem] Event instance not found: " << instanceName << "\n";
        return false;
      }

      return FMOD_Check(it->second->start(), "start");
    }

    bool stopEventInstance(const ::std::string& instanceName, bool immediate)
    {
      auto it = gEventInstances.find(instanceName);
      if (it == gEventInstances.end())
      {
        ::std::cout << "[AudioSystem] Event instance not found: " << instanceName << "\n";
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
        ::std::cout << "[AudioSystem] Event instance not found: " << instanceName << "\n";
        return false;
      }

      return FMOD_Check(it->second->setPaused(pause), "setPaused");
    }

    bool releaseEventInstance(const ::std::string& instanceName)
    {
      auto it = gEventInstances.find(instanceName);
      if (it == gEventInstances.end())
      {
        ::std::cout << "[AudioSystem] Event instance not found: " << instanceName << "\n";
        return false;
      }

      FMOD_Check(it->second->release(), "release");
      gEventInstances.erase(it);
      ::std::cout << "[AudioSystem] Released event instance: " << instanceName << "\n";
      return true;
    }

    void releaseAllEventInstances()
    {
      for (auto& pair : gEventInstances)
      {
        pair.second->release();
      }
      gEventInstances.clear();
      ::std::cout << "[AudioSystem] Released all event instances\n";
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
        ::std::cout << "[AudioSystem] Event instance not found: " << instanceName << "\n";
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
        ::std::cout << "[AudioSystem] Event instance not found: " << instanceName << "\n";
        return false;
      }

      return FMOD_Check(
        it->second->getParameterByName(paramName.c_str(), &outValue),
        "getParameterByName");
    }

    // ------------------------------------------------------------
    // 3D Audio Positioning
    // ------------------------------------------------------------
    void setListenerPosition(const Math::Vec2D& position)
    {
      gListenerAttributes.position = ToFMODVector(position);

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
        ::std::cout << "[AudioSystem] Event instance not found: " << instanceName << "\n";
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
        ::std::cout << "[AudioSystem] Event instance not found: " << instanceName << "\n";
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
        ::std::cout << "[AudioSystem] Event instance not found: " << instanceName << "\n";
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
