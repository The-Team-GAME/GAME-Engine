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
      std::vector<Event> event_queue;
      std::unordered_map<EventTypeID, std::vector<std::function<void(const Event&)>>> event_handlers;
    }

    void remove_event_listener(EventTypeID eventType)
    {
      Internal::event_handlers.erase(eventType);
    }

    void update_events(const std::vector<std::unique_ptr<ECS::System>>& systems)
    {
      // Dispatch all queued events
      for (const auto& event : Internal::event_queue)
      {
        // Call global event handlers
        auto handlerIt = Internal::event_handlers.find(event.type_id);
        if (handlerIt != Internal::event_handlers.end()) {
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
      Internal::event_queue.clear();
    }
  }
}