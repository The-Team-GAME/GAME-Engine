#include "EcsState.h"
#include "Graphics.h"
#include <iostream>
#include <glm/glm.hpp>
#include "Core.h"
#include "BasicComponents.h"
#include "MovementSystem.h"
#include "HealthSystem.h"
#include "RenderSystem.h"
#include "BackgroundSystem.h"
#include "InputManager.h"

int enemyPrefab;

// --- Event System Test ---
// 1. Define an event struct
struct DamageEvent {
  ECS::EntityID target;
  int amount;
};

// 2. Define a system that listens for the event
class DamageSystem : public ECS::System {
public:
  DamageSystem() {
    listenToEvents<DamageEvent>();
    requireComponents<Components::Health>();
  }

  void onEvent(const ECS::Event& event, ECS::EntityID entity) override {
    if (const auto* damageEvent = event.get<DamageEvent>()) {
      // The event is for a specific target, only process if it matches the entity this system is currently processing
      if (damageEvent->target == entity) {
        auto* health = world->getComponent<Components::Health>(entity);
        health->damage(damageEvent->amount);
      }
    }
  }
};
// --- End of Event System Test ---

void EcsState::Load()
{
  enemyPrefab = Engine::FACTORY.registerObject("Assets/objects/enemy.json");

}

ECS::Entity player;

void EcsState::Init()
{
  // Create player entity
  player = Engine::WORLD.createEntity()
    .addComponent<Components::Tag<"Player">>()
    .addComponent<Components::Tag<"GameObjectState">>()
    .addComponent<Components::Position>(10.0f, 20.0f)
    .addComponent<Components::Velocity>(100.0f, 0.0f) // Make player move
    .addComponent<Components::Health>(100, 100)
    .addComponent<Components::Size>(10.0f, 10.0f)
    .addComponent<Components::Color>(0.0f, 1.0f, 1.0f, 1.0f); // Make player cyan

  // Create an enemy entity to act as a template
  auto enemy = Engine::WORLD.createEntity()
    .addComponent<Components::Tag<"Enemy">>()
    .addComponent<Components::Tag<"GameObjectState">>()
    .addComponent<Components::Position>(50.0f, 30.0f)
    .addComponent<Components::Health>(100, 100)
    .addComponent<Components::Size>(10.0f, 10.0f)
    .addComponent<Components::Color>(0.0f, 1.0f, 0.0f, 1.0f);

  // --- Cloning Demonstration ---
  // Create a new enemy by cloning the first one
  auto clonedEnemy = Engine::WORLD.cloneEntity(enemy.getId());

  // Check if the clone is valid and then modify it to differentiate it
  if (clonedEnemy.isValid()) {
    // Get the position component of the clone and move it somewhere else
    auto* clonedPos = clonedEnemy.getComponent<Components::Position>();
    if (clonedPos) {
      clonedPos->x = 200.0f;
      clonedPos->y = 150.0f;
    }
  }
  // --- End of Demonstration ---

  // Entity to clear background
  auto background = Engine::WORLD.createEntity()
    .addComponent<Components::Tag<"GameObjectState">>()
    .addComponent<Components::Background>()
    .addComponent<Components::Color>(1.0f, 1.0f, 1.0f, 1.0f); // White background

  // Add systems to the world
  Engine::WORLD.addSystem<BackgroundSystem>(); // Background system should be first
  Engine::WORLD.addSystem<MovementSystem>();
  Engine::WORLD.addSystem<HealthSystem>();
  Engine::WORLD.addSystem<RectRenderSystem>();
  Engine::WORLD.addSystem<DamageSystem>(); // Add the new system
}

void EcsState::Update()
{
  if (Engine::Input::IsKeyPressed(GLFW_KEY_SPACE))
  {
    Engine::WORLD.sendEvent<DamageEvent>(player.getId(), 10);
    // Print the health of all entities with Health component  
    for (const auto& e : Engine::WORLD.getEntitiesWith<Components::Health>()) {
      auto* health = Engine::WORLD.getComponent<Components::Health>(e);
      std::cout << "Entity " << e << " has health: " << health->value << "\n";
    }
    std::cout << "\n";

    auto bg_color = Engine::WORLD.getComponent<Components::Color>(Engine::WORLD.getFirstEntityWith<Components::Background>());

    if (!player.hasComponent<Components::Health>())
      return;

    *bg_color = Components::Color{
      player.getComponent<Components::Health>()->value / 100.0f,
      player.getComponent<Components::Health>()->value / 100.0f,
      player.getComponent<Components::Health>()->value / 100.0f,
      1.0f
    };
  }
}

void EcsState::Draw()
{
}

void EcsState::Free()
{
  Engine::WORLD.destroyEntitiesWith<Components::Tag<"GameObjectState">>();

  // Remove systems
  // TODO: make a function to remove all systems at once
  Engine::WORLD.removeSystem<BackgroundSystem>();
  Engine::WORLD.removeSystem<MovementSystem>();
  Engine::WORLD.removeSystem<HealthSystem>();
  Engine::WORLD.removeSystem<RectRenderSystem>();
  Engine::WORLD.removeSystem<DamageSystem>();
}

void EcsState::Unload()
{
}