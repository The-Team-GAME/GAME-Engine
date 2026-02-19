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
      // Leaked singletons — intentionally never destroyed so that
      // destructors running during static cleanup can still use them.
      std::vector<Event>& get_event_queue()
      {
        static auto* queue = new std::vector<Event>();
        return *queue;
      }

      std::unordered_map<EventTypeID, std::vector<std::function<void(const Event&)>>>& get_event_handlers()
      {
        static auto* handlers = new std::unordered_map<EventTypeID, std::vector<std::function<void(const Event&)>>>();
        return *handlers;
      }
    }

    void remove_event_listener(EventTypeID eventType)
    {
      Internal::get_event_handlers().erase(eventType);
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