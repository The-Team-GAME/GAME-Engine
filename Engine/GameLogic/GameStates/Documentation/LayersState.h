#pragma once

#include "../../Engine/MonkeyEngine.h"

class LayersState : public Engine::GameState<LayersState>
{
private:
  Engine::ECS::Prefab layersStatePrefab = Engine::ECS::create_prefab()
    .add_component<Components::Tag<"LayersState">>();

public:
  void load() {
  };

  void init() {
    Engine::Resources::set_resource_path("Assets/");

    layersStatePrefab.instantiate()
      .add_component<Components::Color>(1.0f, 1.0f, 1.0f, 1.0f)
      .add_component<Components::Transform2D>(800.0f, 450.0f, 0.0f, 15.0f * 6.0f, 21.0f * 6.0f)
      .add_component<Components::Texture>(Engine::Resources::load_texture("images/monkey.png"));

    layersStatePrefab.instantiate()
      .add_component<Components::Color>(1.0f, 1.0f, 1.0f, 1.0f)
      .add_component<Components::Transform2D>(800.0f, 450.0f, 0.0f, 20.0f * 4.0f, 12.0f * 4.0f)
      .add_component<Components::Texture>(Engine::Resources::load_texture("images/banana.png"));
  };

  void update() {
  };

  void draw() {
    Engine::Graphics::clear_screen({ 0.1f, 0.1f, 0.1f, 1.0f });
  };

  void free() {
    Engine::ECS::destroy_entities_with<Components::Tag<"LayersState">>();
  };

  void unload() {
  };
};
