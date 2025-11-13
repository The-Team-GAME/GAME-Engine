#include "ProjectSaveManager.h"
#include "Logger.h"
#include "FileIO/Settings.h"

namespace {
  std::bitset<32> dirty_flags;
}

void Engine::ProjectSaveManager::set_dirty(Dirty type)
{
  dirty_flags.set(static_cast<size_t>(type));
}

void Engine::ProjectSaveManager::save_all_dirty()
{
  if (dirty_flags.test(static_cast<size_t>(Dirty::ProjectSettings)))
  {
    // Save project settings
    if (Engine::FileIO::SettingsManager<Engine::FileIO::ProjectSettings>::save("Assets/settings/project_settings.json"))
    {
      Log::info("Project settings saved.");
      dirty_flags.reset(static_cast<size_t>(Dirty::ProjectSettings));
    }
    else
    {
      Log::error("Failed to save project settings.");
    }
  }
}

bool Engine::ProjectSaveManager::get_is_any_dirty()
{
  return dirty_flags.any();
}
