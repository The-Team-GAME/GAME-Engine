
#include "../../Engine/MonkeyEngine.h"

namespace OldManCallbacks
{
  void old_man_on_interact(Engine::ECS::Entity entity) {
    Log::info("Interacted with entity ID {}", entity.get_id());
  }
  void old_man_on_update(Engine::ECS::Entity entity) {
    Log::info("Updating interaction with entity ID {}", entity.get_id());
  }
} // namespace OldManCallbacks


REGISTER_FUNCTION(OldManCallbacks::old_man_on_interact, void(Engine::ECS::Entity), OldManCallbacks::old_man_on_interact);
REGISTER_FUNCTION(OldManCallbacks::old_man_on_update, void(Engine::ECS::Entity), OldManCallbacks::old_man_on_update);