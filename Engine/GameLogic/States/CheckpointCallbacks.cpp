#include "pch.h"
#include "../Components/GameplayComponent.h"

namespace CheckpointCallbacks
{

  inline void spawn_checkpoint_prompt(Engine::ECS::Entity checkpoint)
  {
    auto* checkpoint_transform = checkpoint.get_component<Components::Transform2D>();
    if (!checkpoint_transform) return;

    auto prefab = Engine::Resources::load_prefab("prefabs/checkpoint_prompt.json");
    Engine::ECS::Entity prompt = Engine::ECS::instantiate_prefab(prefab);

    if (auto* prompt_transform = prompt.get_component<Components::Transform2D>())
    {
      prompt_transform->position = checkpoint_transform->position;
      prompt_transform->position.y -= 50.0f; // above checkpoint
    }

    prompt.add_component<Components::Lifetime>(1.0f);

  }

  void checkpoint_on_enter(Engine::ECS::Entity checkpoint, Engine::ECS::Entity player_entity)
  {
    auto* checkpoint_transform = checkpoint.get_component<Components::Transform2D>();
    auto* player = player_entity.get_component<Components::Player>();

    if (!checkpoint_transform || !player) return;

    player->respawn_point = checkpoint_transform->position;
    player->respawn_point.y -= 50.0f; //ground offset
    player->has_checkpoint = true;

    Log::info("Checkpoint reached at ({}, {})",
      checkpoint_transform->position.x,
      checkpoint_transform->position.y);

    spawn_checkpoint_prompt(checkpoint);
  }

  void checkpoint_on_update([[maybe_unused]] Engine::ECS::Entity checkpoint,
    [[maybe_unused]] Engine::ECS::Entity player_entity)
  {
  }

  void checkpoint_on_exit([[maybe_unused]] Engine::ECS::Entity checkpoint,
    [[maybe_unused]] Engine::ECS::Entity player_entity)
  {
  }
}

REGISTER_FUNCTION(CheckpointCallbacks::checkpoint_on_enter)
REGISTER_FUNCTION(CheckpointCallbacks::checkpoint_on_update)
REGISTER_FUNCTION(CheckpointCallbacks::checkpoint_on_exit)