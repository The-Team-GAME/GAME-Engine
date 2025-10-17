#pragma once

#include "../../Engine/MonkeyEngine.h"

class PrefabsState : public Engine::GameState<PrefabsState> {
private:
  Engine::ECS::Entity prefab_instance;

public:
  void load() {
    Engine::Resources::set_resource_path("Assets/");

    // Create banana prefab
    auto banana_prefabs = Engine::ECS::create_prefab([this](Engine::ECS::Entity& entity) {

      auto banana_texture = Engine::Resources::load_texture("images/banana.png");

      entity.add_component<Components::Tag<"ShowcaseState">>()
        .add_component<Components::Transform2D>(1200.0f, 675.0f, 0.0f, 20.0f * 3.0f, 12.0f * 3.0f)
        .add_component<Components::Texture>(banana_texture)
        .add_component<Components::Color>(1.0f, 1.0f, 1.0f, 1.0f)
        .add_component<Components::RigidBody>();

      auto* rigid = entity.get_component<Components::RigidBody>();
      rigid->rotation = 0.0f;
      rigid->angular_velocity = 45.0f;
      });

    Engine::Resources::save_prefab("prefabs/banana_prefab.json", banana_prefabs);


    auto loaded_banana_prefab = Engine::Resources::load_prefab("prefabs/banana_prefab.json");

    Engine::ECS::instantiate_prefab(loaded_banana_prefab).add_component<Components::Tag<"PrefabsState">>();

  };

  void init() {};

  void update() {};

  void draw() {
    Engine::Graphics::clear_screen({ 0.1f, 0.1f, 0.1f, 1.0f });
  };

  void free() {};

  void unload() {
    Engine::ECS::destroy_entities_with<Components::Tag<"PrefabsState">>();
  };
};