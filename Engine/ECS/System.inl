#pragma once
#include "System.h"
#include "ComponentManager.h"
#include "EventManager.h"

namespace ECS {

  template<typename... Components>
  void System::requireComponents() {
    requiredSignature.reset();
    ((requiredSignature.set(ComponentTypeManager::getTypeId<Components>())), ...);
  }

  template<typename... Events>
  void System::listenToEvents() {
    eventSignature.reset();
    ((eventSignature.set(EventTypeManager::getTypeId<Events>())), ...);
  }

} // namespace ECS