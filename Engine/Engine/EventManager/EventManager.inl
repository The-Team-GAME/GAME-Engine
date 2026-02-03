//-----------------------------------------------------------------------------------
// file:    EventManager.inl
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   This file implements the template methods for EventTypeManager and Event.
//          Provides automatic event type ID generation and type-safe event data
//          storage and retrieval.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once
#include "EventManager.h"

namespace Engine {
  namespace EventManager
  {

    template <typename T>
    EventTypeID EventTypeManager::get_type_id()
    {
      static EventTypeID type_id = ++next_type_id;
      return type_id;
    }

    template <typename T>
    Event::Event(T&& eventData) : type_id(EventTypeManager::get_type_id<T>()), data(std::forward<T>(eventData)) {}

    template <typename T>
    const T* Event::get() const
    {
      if (EventTypeManager::get_type_id<T>() == type_id)
      {
        return std::any_cast<T>(&data);
      }
      return nullptr;
    }

    template <typename T, typename... Args>
    void send_event(Args&&... args)
    {
      T event(std::forward<Args>(args)...);
      Internal::event_queue.emplace_back(std::move(event));
    }

    template <typename T>
    void add_event_listener(std::function<void(const T&)> handler)
    {
      EventTypeID eventType = EventTypeManager::get_type_id<T>();
      Internal::event_handlers[eventType].emplace_back([handler](const Event& event) {
        if (const T* eventData = event.get<T>()) {
          handler(*eventData);
        }
        });
    }

    template <typename T>
    void add_event_listener(std::function<void(const Event&)> handler)
    {
      EventTypeID eventType = EventTypeManager::get_type_id<T>();
      Internal::event_handlers[eventType].emplace_back(handler);
    }

    template <typename T>
    void remove_event_listener()
    {
      EventTypeID eventType = EventTypeManager::get_type_id<T>();
      remove_event_listener(eventType);
    }

    template <typename SystemsContainer>
    void update_events(const SystemsContainer& systems)
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
        for (size_t i = 0; i < systems.size(); ++i)
        {
          auto& system = systems[i];
          const auto& systemEventSignature = system->get_event_signature();
          if (systemEventSignature.test(event.type_id))
          {
            for (auto entity : system->get_entities()) {
              system->on_event(event, entity);
            }
          }
        }
      }

      // Clear the queue after processing
      Internal::event_queue.clear();
    }

  } // namespace EventManager
} // namespace Engine