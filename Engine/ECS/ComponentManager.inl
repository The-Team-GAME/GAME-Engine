#pragma once
#include "ComponentManager.h"

namespace ECS {

  template<typename T>
  ComponentTypeID ComponentTypeManager::getTypeId() {
    static ComponentTypeID typeId = ++nextTypeId;
    return typeId;
  }

} // namespace ECS