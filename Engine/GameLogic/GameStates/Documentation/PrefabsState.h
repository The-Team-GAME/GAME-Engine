#pragma once

#include "../../Engine/MonkeyEngine.h"

class PrefabsState : public Engine::GameState<PrefabsState> {
private:
  Engine::ECS::Entity prefab_instance;

public:
  void load() {
    Engine::Resources::set_resource_path("Assets/");

    //// Create a banana prefab
    //// Engine::ECS::create_prefab takes a lambda function that sets up the entity
    //auto banana_prefabs = Engine::ECS::create_prefab([this]() {
    //  // Inside this lambda, we define the components for the banana prefab

    //  auto entity = Engine::ECS::create_entity();

    //  // Load banana texture
    //  auto banana_texture = Engine::Resources::load_texture("images/banana.png");

    //  // Add components to the entity
    //  entity.add_component<Components::Tag<"PrefabsState">>()
    //    .add_component<Components::Transform2D>(1200.0f, 675.0f, 0.0f, 20.0f * 3.0f, 12.0f * 3.0f)
    //    .add_component<Components::Texture>(banana_texture)
    //    .add_component<Components::Color>(1.0f, 1.0f, 1.0f, 1.0f)
    //    .add_component<Components::RigidBody>();

    //  // Set initial rotation and angular velocity
    //  auto* rigid = entity.get_component<Components::RigidBody>();
    //  rigid->rotation = 0.0f;
    //  rigid->angular_velocity = 45.0f;

    //  return entity;
    //  }
    //);

    //// You serialize the prefab to JSON
    //std::string banana_json = Engine::ECS::serialize_prefab_to_json(banana_prefabs);
    //std::cout << banana_json << "\n";

    //// Save the banana prefab to a JSON file
    //Engine::Resources::save_prefab("prefabs/banana_prefab.json", banana_prefabs);

    // Later, you can load the prefab from the JSON file
    auto loaded_banana_prefab = Engine::Resources::load_prefab("prefabs/banana_prefab.json");

    // Instantiate the loaded prefab in the ECS world
    auto banana = Engine::ECS::instantiate_prefab(loaded_banana_prefab).add_component<Components::Tag<"PrefabsState">>();

    // print rigidbody angular velocity to verify
    std::cout << banana.get_component<Components::RigidBody>()->angular_velocity << "\n";

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