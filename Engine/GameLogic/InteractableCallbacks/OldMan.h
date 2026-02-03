
#include "../../Engine/MonkeyEngine.h"

namespace OldManCallbacks
{
  void old_man_on_enter(Engine::ECS::Entity interactable, Engine::ECS::Entity player_entity) {
    Log::info("Player {} entered interaction range of entity {}", player_entity.get_id(), interactable.get_id());
  }

  void old_man_on_update(Engine::ECS::Entity interactable, Engine::ECS::Entity player_entity) {
    Log::info("Player {} is within interaction range of entity {}", player_entity.get_id(), interactable.get_id());
  }

  void old_man_on_exit(Engine::ECS::Entity interactable, Engine::ECS::Entity player_entity) {
    Log::info("Player {} exited interaction range of entity {}", player_entity.get_id(), interactable.get_id());
  }
} // namespace OldManCallbacks


REGISTER_FUNCTION(old_man_on_enter, void(Engine::ECS::Entity, Engine::ECS::Entity), OldManCallbacks::old_man_on_enter);
REGISTER_FUNCTION(old_man_on_update, void(Engine::ECS::Entity, Engine::ECS::Entity), OldManCallbacks::old_man_on_update);
REGISTER_FUNCTION(old_man_on_exit, void(Engine::ECS::Entity, Engine::ECS::Entity), OldManCallbacks::old_man_on_exit);