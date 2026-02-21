//-----------------------------------------------------------------------------------
// file:    EventManager.cpp
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   This file implements the EventTypeManager static member initialization
//          and event dispatching logic.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#include "EventManager.h"
#include "../ECS/System.h"

namespace Engine {
  namespace EventManager
  {
    EventTypeID EventTypeManager::next_type_id = 0;

    namespace Internal
    {
      namespace {
        std::vector<Event>* g_event_queue = nullptr;
        std::unordered_map<EventTypeID, std::vector<std::function<void(const Event&)>>>* g_event_handlers = nullptr;
      }

      std::vector<Event>& get_event_queue()
      {
        if (!g_event_queue) g_event_queue = new std::vector<Event>();
        return *g_event_queue;
      }

      std::unordered_map<EventTypeID, std::vector<std::function<void(const Event&)>>>& get_event_handlers()
      {
        if (!g_event_handlers) g_event_handlers = new std::unordered_map<EventTypeID, std::vector<std::function<void(const Event&)>>>();
        return *g_event_handlers;
      }
    }

    void remove_event_listener(EventTypeID eventType)
    {
      Internal::get_event_handlers().erase(eventType);
    }

    void clear_all_listeners()
    {
      Internal::get_event_handlers().clear();
      Internal::get_event_queue().clear();
    }

    void destroy_all()
    {
      delete Internal::g_event_handlers;
      Internal::g_event_handlers = nullptr;
      delete Internal::g_event_queue;
      Internal::g_event_queue = nullptr;
    }

    void update_events(const std::vector<std::unique_ptr<ECS::System>>& systems)
    {
      auto& queue = Internal::get_event_queue();
      auto& handlers = Internal::get_event_handlers();

      // Dispatch all queued events
      for (const auto& event : queue)
      {
        // Call global event handlers
        auto handlerIt = handlers.find(event.type_id);
        if (handlerIt != handlers.end()) {
          for (const auto& handler : handlerIt->second) {
            handler(event);
          }
        }

        // Dispatch to systems that listen to this event type
        for (auto& system : systems)
        {
          const auto& systemEventSignature = system->get_event_signature();
          if (systemEventSignature.test(event.type_id))
          {
            for (ECS::EntityID entity : system->get_entities()) {
              system->on_event(event, entity);
            }
          }
        }
      }

      // Clear the queue after processing
      queue.clear();
    }
  }
}