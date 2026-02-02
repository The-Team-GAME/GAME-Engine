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

    // Note: update_events is now a template function defined in EventManager.inl
  }
}