#include "../ECS/ECS.h"

namespace Engine {
  namespace FileIO {
    std::string serialize_prefab_to_json(const Engine::ECS::Prefab& prefab) {
      if (!prefab) {
        return "{}";
      }
      ReflectionLibrary::GenericValue generic_value = Engine::ECS::prefab_to_generic_value(prefab);
      return ReflectionLibrary::generic_value_to_json(generic_value, true);
    }

    Engine::ECS::Prefab deserialize_prefab_from_json(const std::string& json_str) {
      std::optional<ReflectionLibrary::GenericValue> generic_value = ReflectionLibrary::generic_value_from_json(json_str);
      if (!generic_value) {
        return nullptr;
      }
      return Engine::ECS::prefab_from_generic_value(generic_value.value());
    }
  }
}