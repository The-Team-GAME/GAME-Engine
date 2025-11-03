#pragma once

#include "../../Engine/MonkeyEngine.h"

class PrefabsState : public Engine::GameState<PrefabsState> {
private:
  Engine::ECS::Entity prefab_instance;

public:
  void load() {
    Engine::Resources::set_resource_path("Assets/");

    // Create a banana prefab directly with components
    auto banana_prefab = Engine::ECS::create_prefab();

    // Load banana texture
    auto banana_texture = Engine::Resources::load_texture("images/banana.png");

    // Add components to the prefab (like an entity)
    // Can also configure layers using add_to_layer()
    banana_prefab.add_component<Components::Transform2D>(1200.0f, 675.0f, 0.0f, 20.0f * 3.0f, 12.0f * 3.0f)
      .add_component<Components::Texture>(banana_texture)
      .add_component<Components::Color>(1.0f, 1.0f, 1.0f, 1.0f)
      .add_component<Components::RigidBody>((1200.f, 675.f), false, false)
      .add_to_layer("gameplay");  // Instantiated entities will be on these layers

    // Set initial rotation and angular velocity on the prefab
    auto* rigid = banana_prefab.get_component<Components::RigidBody>();
    rigid->rotation = 0.0f;
    rigid->angular_velocity = 45.0f;

    // You can serialize the prefab to JSON
    std::string banana_json = Engine::FileIO::serialize_prefab_to_json(banana_prefab);
    std::cout << banana_json << "\n";

    // Save the banana prefab to a JSON file
    Engine::Resources::save_prefab("prefabs/banana_prefab.json", banana_prefab);

    // Later, you can load the prefab from the JSON file
    auto loaded_banana_prefab = Engine::Resources::load_prefab("prefabs/banana_prefab.json");

    // Instantiate the loaded prefab in the ECS world
    auto banana = Engine::ECS::instantiate_prefab(loaded_banana_prefab).add_component<Components::Tag<"PrefabsState">>();

    // instantiate_prefab returns an Entity, so we can inspect or modify it further if needed
    banana.get_component<Components::Transform2D>()->scale = { 120.0f, 72.0f };

    // Alternative: Create a prefab from an existing entity
    auto test_entity = Engine::ECS::create_entity();
    test_entity.add_component<Components::Transform2D>(0.0f, 0.0f, 0.0f, 50.0f, 50.0f)
      .add_component<Components::Color>(1.0f, 0.5f, 0.0f, 1.0f)
      .add_component<Components::Texture>(Engine::Resources::load_texture("images/monkey.png"));

    // Convert entity to prefab
    auto entity_prefab = Engine::ECS::entity_to_prefab(test_entity);

    // Can now instantiate multiple entities from this prefab
    auto copy1 = Engine::ECS::instantiate_prefab(entity_prefab);
    auto copy2 = Engine::ECS::instantiate_prefab(entity_prefab);

    // std::string test_entity_json = Engine::FileIO::serialize_prefab_to_json(entity_prefab);
    // std::cout << test_entity_json << "\n";

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