#include "pch.h"
#include "../Components/GameplayComponent.h"

namespace CheckpointCallbacks
{

  inline void spawn_checkpoint_prompt_animation()
  {
    Engine::ECS::create_entity()
      .add_component<Components::Transform2D>(640.0f, 120.0f, 0.0f, 300.0f, 100.0f)
      .add_component<Components::Color>(1.0f, 1.0f, 1.0f, 1.0f)
      .add_component<Components::Texture>(
        Engine::Resources::load_texture("images/UI/CheckpointPrompt.png"))
      .add_component<Components::TextureAtlas>(
      /* your atlas data here */)
      .add_component<Components::Animation>(
      /* your animation data here */)
      .add_component<Components::Lifetime>(1.0f)
      .add_to_layer("ui");
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

    //spawn_checkpoint_prompt_animation();
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