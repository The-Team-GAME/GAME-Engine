#pragma once
#include "../EventManager/EventManager.h"
#include "WindowEvents.h"
#include "../Graphics/Renderer.h"
#include "../Resources.h"


namespace Engine {
  namespace Events {
    void register_all_events() {
      // Register the graphics resize system 
      Engine::EventManager::add_event_listener<Engine::Events::WindowResizeEvent>([](const Engine::Events::WindowResizeEvent& event)
        {
          Engine::Graphics::set_viewport({ 0, 0, event.width, event.height });
        }
      );

      // Register file drop event to load resources
      Engine::EventManager::add_event_listener<Engine::Events::FileDropEvent>([](const Engine::Events::FileDropEvent& event)
        {
          for (const auto& filepath : event.filepaths)
          {
            Log::info("File dropped: " + filepath);
            Engine::Resources::load(filepath, false, true);
          }
        }
      );
    }
  }
}
