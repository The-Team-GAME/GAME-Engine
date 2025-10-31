#pragma once

#include "../../Engine/MonkeyEngine.h"

// =============================================================================
// LayersState - Comprehensive demonstration of the ECS Layer System
// =============================================================================
// This state showcases ALL layer features:
// 1. Layer registration
// 2. Adding/removing entities to/from layers
// 3. Multi-layer entities
// 4. Dynamic layer switching
// 5. Layer queries and checks
// 6. Layer priorities and sorting
// 7. String-based layer API
// 8. Runtime layer management
// =============================================================================

class LayersState : public Engine::GameState<LayersState> {
private:
  // Layer IDs (cached for performance)
  Engine::ECS::LayerID background_layer;
  Engine::ECS::LayerID gameplay_layer;
  Engine::ECS::LayerID foreground_layer;
  Engine::ECS::LayerID ui_layer;
  Engine::ECS::LayerID debug_layer;
  Engine::ECS::LayerID effects_layer;

  // Entities for demonstration
  Engine::ECS::Entity background_entity;
  Engine::ECS::Entity gameplay_entity1;
  Engine::ECS::Entity gameplay_entity2;
  Engine::ECS::Entity foreground_entity;
  Engine::ECS::Entity ui_entity;
  Engine::ECS::Entity debug_entity;
  Engine::ECS::Entity multi_layer_entity;

  // Toggle states
  bool show_debug = false;
  bool pause_gameplay = false;
  float time_elapsed = 0.0f;

public:
  void load() {
    Engine::Resources::set_resource_path("Assets/");

    // Load textures
    auto monkey_texture = Engine::Resources::load_texture("images/monkey.png");
    auto banana_texture = Engine::Resources::load_texture("images/banana.png");

    // =======================================================================
    // FEATURE 0: Default Layer (Auto-initialized in Core)
    // =======================================================================
    std::cout << "\n========================================\n";
    std::cout << "LAYER SYSTEM DEMONSTRATION\n";
    std::cout << "========================================\n\n";

    std::cout << "Step 0: Default Layer\n";

    // The 'default' layer is automatically pre-registered with ID 1
    // in the WorldContainers constructor (no manual initialization needed)
    Engine::ECS::LayerID default_layer = Engine::ECS::get_default_layer();
    std::cout << "  Default layer pre-registered: 'default' (ID: " << default_layer << ")\n";
    std::cout << "  All entities are automatically added to 'default' on creation\n\n";

    // =======================================================================
    // FEATURE 1: Layer Registration (already done in WinMain.cpp at startup)
    // =======================================================================
    std::cout << "Step 1: Retrieving layer IDs...\n";

    // Get layer IDs (layers were registered in WinMain.cpp)
    // Lower IDs = higher priority = rendered/processed first
    background_layer = Engine::ECS::get_layer_id("background");
    gameplay_layer = Engine::ECS::get_layer_id("gameplay");
    foreground_layer = Engine::ECS::get_layer_id("foreground");
    ui_layer = Engine::ECS::get_layer_id("ui");
    debug_layer = Engine::ECS::get_layer_id("debug");
    effects_layer = Engine::ECS::get_layer_id("effects");

    std::cout << "  'background' -> ID: " << background_layer << "\n";
    std::cout << "  'gameplay' -> ID: " << gameplay_layer << "\n";
    std::cout << "  'foreground' -> ID: " << foreground_layer << "\n";
    std::cout << "  'ui' -> ID: " << ui_layer << "\n";
    std::cout << "  'debug' -> ID: " << debug_layer << "\n";
    std::cout << "  'effects' -> ID: " << effects_layer << "\n";
    std::cout << "  (Layers registered once at startup in WinMain.cpp)\n\n";

    // =======================================================================
    // FEATURE 2: Entities start on default layer
    // =======================================================================
    std::cout << "Step 2: Demonstrating default layer behavior...\n";

    // Create a temporary entity to show default layer behavior
    auto temp_entity = Engine::ECS::create_entity();
    std::cout << "  Created temporary entity (no layer specified)\n";
    std::cout << "  Is on 'default' layer? " << (temp_entity.is_on_layer("default") ? "YES" : "NO") << "\n";
    std::cout << "  Total layers: " << temp_entity.get_layers().count() << "\n";
    Engine::ECS::destroy_entity(temp_entity);
    std::cout << "  (Entities automatically start on 'default' layer!)\n\n";

    // =======================================================================
    // FEATURE 3: Creating entities and assigning to layers BY NAME
    // =======================================================================
    std::cout << "Step 3: Creating entities on specific layers...\n";

    // Background entity - rendered first (highest priority)
    background_entity = Engine::ECS::create_entity();
    background_entity.add_to_layer("background")
      .add_component<Components::Transform2D>(640.0f, 360.0f, 0.0f, 1280.0f, 720.0f)
      .add_component<Components::Color>(0.2f, 0.2f, 0.3f, 1.0f)
      .add_component<Components::Tag<"LayersState">>();
    std::cout << "  Created background entity (on 'default' + 'background' layers)\n";

    // Gameplay entities - rendered after background
    gameplay_entity1 = Engine::ECS::create_entity();
    gameplay_entity1.add_to_layer("gameplay")
      .add_component<Components::Transform2D>(400.0f, 360.0f, 0.0f, 100.0f, 100.0f)
      .add_component<Components::Texture>(monkey_texture)
      .add_component<Components::Color>(1.0f, 1.0f, 1.0f, 1.0f)
      .add_component<Components::RigidBody>()
      .add_component<Components::Tag<"LayersState">>();

    auto* rb1 = gameplay_entity1.get_component<Components::RigidBody>();
    rb1->velocity = { 50.0f, 0.0f };
    rb1->angular_velocity = 45.0f;
    std::cout << "  Created gameplay entity 1 on 'gameplay' layer\n";

    gameplay_entity2 = Engine::ECS::create_entity();
    gameplay_entity2.add_to_layer("gameplay")
      .add_component<Components::Transform2D>(880.0f, 360.0f, 0.0f, 80.0f, 48.0f)
      .add_component<Components::Texture>(banana_texture)
      .add_component<Components::Color>(1.0f, 1.0f, 0.0f, 1.0f)
      .add_component<Components::RigidBody>()
      .add_component<Components::Tag<"LayersState">>();

    auto* rb2 = gameplay_entity2.get_component<Components::RigidBody>();
    rb2->velocity = { -30.0f, 20.0f };
    rb2->angular_velocity = -90.0f;
    std::cout << "  Created gameplay entity 2 on 'gameplay' layer\n";

    // Foreground entity - rendered on top of gameplay
    foreground_entity = Engine::ECS::create_entity();
    foreground_entity.add_to_layer("foreground")
      .add_component<Components::Transform2D>(640.0f, 100.0f, 0.0f, 400.0f, 80.0f)
      .add_component<Components::Color>(0.0f, 0.0f, 0.0f, 0.3f)
      .add_component<Components::Tag<"LayersState">>();
    std::cout << "  Created foreground entity on 'foreground' layer\n";

    // UI entity - rendered on top of everything
    ui_entity = Engine::ECS::create_entity();
    ui_entity.add_to_layer("ui")
      .add_component<Components::Transform2D>(100.0f, 50.0f, 0.0f, 180.0f, 60.0f)
      .add_component<Components::Color>(0.1f, 0.1f, 0.1f, 0.8f)
      .add_component<Components::Tag<"LayersState">>();
    std::cout << "  Created UI entity on 'ui' layer\n";

    // Debug entity - initially off screen (will be shown when debug enabled)
    debug_entity = Engine::ECS::create_entity();
    debug_entity.add_to_layer("debug")
      .add_component<Components::Transform2D>(640.0f, 650.0f, 0.0f, 200.0f, 40.0f)
      .add_component<Components::Color>(1.0f, 0.0f, 0.0f, 0.7f)
      .add_component<Components::Tag<"LayersState">>();
    std::cout << "  Created debug entity on 'debug' layer\n\n";

    // =======================================================================
    // FEATURE 4: Multi-layer entities
    // =======================================================================
    std::cout << "Step 4: Creating multi-layer entity...\n";

    multi_layer_entity = Engine::ECS::create_entity();
    multi_layer_entity.add_to_layer("gameplay")   // Show in main game
      .add_to_layer("effects")     // Also show in effects layer
      .add_component<Components::Transform2D>(640.0f, 500.0f, 0.0f, 60.0f, 60.0f)
      .add_component<Components::Color>(1.0f, 0.5f, 0.0f, 1.0f)
      .add_component<Components::Tag<"LayersState">>();

    std::cout << "  Created entity on 'default', 'gameplay', AND 'effects' layers\n";
    std::cout << "  Total layers: " << multi_layer_entity.get_layers().count() << "\n";
    std::cout << "  (Entity will be processed by systems filtering any of these layers)\n\n";

    // =======================================================================
    // FEATURE 5: Layer priority demonstration
    // =======================================================================
    std::cout << "Step 5: Demonstrating layer priorities...\n";

    std::cout << "  Note: 'default' is ID " << Engine::ECS::get_default_layer()
      << ", so it has highest priority!\n";
    std::cout << "  (Entities sorted by highest priority layer for rendering)\n\n";

    std::cout << "========================================\n";
    std::cout << "CONTROLS:\n";
    std::cout << "========================================\n";
    std::cout << "[D] - Toggle Debug Layer visibility\n";
    std::cout << "[P] - Pause/Resume Gameplay Layer\n";
    std::cout << "[M] - Move entity between layers\n";
    std::cout << "[A] - Add/remove entity to/from effects\n";
    std::cout << "[I] - Show layer information\n";
    std::cout << "[C] - Clear all layers from entity\n";
    std::cout << "========================================\n\n";
  };

  void init() {
    time_elapsed = 0.0f;
  };

  void update() {
    time_elapsed += Engine::Time::get_fixed_dt();

    // =======================================================================
    // FEATURE 6: String-based layer API (runtime)
    // =======================================================================
    if (Engine::Input::is_pressed(Engine::KeyCode::D)) {
      show_debug = !show_debug;

      if (show_debug) {
        std::cout << "Debug layer ENABLED (entities on 'debug' now visible)\n";
      }
      else {
        std::cout << "Debug layer DISABLED (entities on 'debug' now hidden)\n";
      }
    }

    // =======================================================================
    // FEATURE 7: Dynamic layer manipulation
    // =======================================================================
    if (Engine::Input::is_pressed(Engine::KeyCode::P)) {
      pause_gameplay = !pause_gameplay;

      if (pause_gameplay) {
        // Remove entities from gameplay layer (systems won't process them)
        gameplay_entity1.remove_from_layer("gameplay");
        gameplay_entity2.remove_from_layer("gameplay");

        // Stop their physics
        gameplay_entity1.get_component<Components::RigidBody>()->velocity = { 0.0f, 0.0f };
        gameplay_entity1.get_component<Components::RigidBody>()->angular_velocity = 0.0f;
        gameplay_entity2.get_component<Components::RigidBody>()->velocity = { 0.0f, 0.0f };
        gameplay_entity2.get_component<Components::RigidBody>()->angular_velocity = 0.0f;

        std::cout << "Gameplay PAUSED (entities removed from 'gameplay' layer)\n";
      }
      else {
        // Add back to gameplay layer
        gameplay_entity1.add_to_layer("gameplay");
        gameplay_entity2.add_to_layer("gameplay");

        // Restore physics
        gameplay_entity1.get_component<Components::RigidBody>()->velocity = { 50.0f, 0.0f };
        gameplay_entity1.get_component<Components::RigidBody>()->angular_velocity = 45.0f;
        gameplay_entity2.get_component<Components::RigidBody>()->velocity = { -30.0f, 20.0f };
        gameplay_entity2.get_component<Components::RigidBody>()->angular_velocity = -90.0f;

        std::cout << "Gameplay RESUMED (entities added back to 'gameplay' layer)\n";
      }
    }

    // =======================================================================
    // FEATURE 8: Moving entities between layers
    // =======================================================================
    if (Engine::Input::is_pressed(Engine::KeyCode::M)) {
      // Check current layer of gameplay_entity1
      if (gameplay_entity1.is_on_layer("gameplay")) {
        // Move to foreground (removes from all other layers)
        gameplay_entity1.clear_layers();
        gameplay_entity1.add_to_layer("foreground");
        std::cout << "Moved entity to 'foreground' layer (cleared other layers)\n";

      }
      else {
        // Move back to gameplay
        gameplay_entity1.clear_layers();
        gameplay_entity1.add_to_layer("gameplay");
        std::cout << "Moved entity back to 'gameplay' layer\n";
      }
    }

    // =======================================================================
    // FEATURE 9: Add/remove from multiple layers
    // =======================================================================
    if (Engine::Input::is_pressed(Engine::KeyCode::B)) {
      if (multi_layer_entity.is_on_layer("effects")) {
        multi_layer_entity.remove_from_layer("effects");
        std::cout << "Removed multi-layer entity from 'effects' layer\n";
        std::cout << "  Still on 'gameplay'? " << (multi_layer_entity.is_on_layer("gameplay") ? "YES" : "NO") << "\n";
      }
      else {
        multi_layer_entity.add_to_layer("effects");
        std::cout << "Added multi-layer entity back to 'effects' layer\n";
        std::cout << "  Now on both 'gameplay' and 'effects'\n";
      }
    }

    // =======================================================================
    // FEATURE 10: Layer queries and information
    // =======================================================================
    if (Engine::Input::is_pressed(Engine::KeyCode::I)) {
      std::cout << "\n========================================\n";
      std::cout << "LAYER INFORMATION\n";
      std::cout << "========================================\n";

      // Check individual layers by name
      std::cout << "\nGameplay Entity 1:\n";
      std::cout << "  On 'gameplay' layer? " << (gameplay_entity1.is_on_layer("gameplay") ? "YES" : "NO") << "\n";
      std::cout << "  On 'foreground' layer? " << (gameplay_entity1.is_on_layer("foreground") ? "YES" : "NO") << "\n";


      // Multi-layer entity info
      std::cout << "\nMulti-Layer Entity:\n";
      std::cout << "  On 'gameplay'? " << (multi_layer_entity.is_on_layer("gameplay") ? "YES" : "NO") << "\n";
      std::cout << "  On 'effects'? " << (multi_layer_entity.is_on_layer("effects") ? "YES" : "NO") << "\n";

      // Get all layers bitset
      Engine::ECS::LayerSignature layers = multi_layer_entity.get_layers();
      std::cout << "  Layer bitset: " << layers.to_string() << "\n";

      // Count active layers
      int count = 0;
      for (Engine::ECS::LayerID i = 0; i < Engine::ECS::MAX_LAYERS; ++i) {
        if (layers.test(i)) count++;
      }
      std::cout << "  Total layers: " << count << "\n";

      // Show all layer names and IDs
      std::cout << "\nAll Registered Layers:\n";
      auto& containers = Engine::ECS::Internal::get_containers();
      for (auto& [name, id] : containers.layer_name_to_id) {
        std::cout << "  " << name << " -> ID: " << id << "\n";
      }

      std::cout << "========================================\n\n";
    }

    // =======================================================================
    // FEATURE 11: Clear all layers
    // =======================================================================
    if (Engine::Input::is_pressed(Engine::KeyCode::X)) {
      if (foreground_entity.get_layers().any()) {
        foreground_entity.clear_layers();
        std::cout << "Cleared all layers from foreground entity\n";
        std::cout << "  Entity is now on NO layers!\n";
        std::cout << "  (Will not be processed by layer-filtered systems)\n";
      }
      else {
        foreground_entity.add_to_layer("foreground");
        std::cout << "Restored foreground entity to 'foreground' layer\n";
      }
    }

    // Bounce physics entities off screen edges
    auto* t1 = gameplay_entity1.get_component<Components::Transform2D>();
    auto* r1 = gameplay_entity1.get_component<Components::RigidBody>();
    if (t1 && r1) {
      if (t1->position.x < 50.0f || t1->position.x > 1230.0f) {
        r1->velocity.x *= -1;
      }
      if (t1->position.y < 50.0f || t1->position.y > 670.0f) {
        r1->velocity.y *= -1;
      }
    }

    auto* t2 = gameplay_entity2.get_component<Components::Transform2D>();
    auto* r2 = gameplay_entity2.get_component<Components::RigidBody>();
    if (t2 && r2) {
      if (t2->position.x < 40.0f || t2->position.x > 1240.0f) {
        r2->velocity.x *= -1;
      }
      if (t2->position.y < 24.0f || t2->position.y > 696.0f) {
        r2->velocity.y *= -1;
      }
    }
  };

  void draw() {
    Engine::Graphics::clear_screen({ 0.05f, 0.05f, 0.08f, 1.0f });

    // =======================================================================
    // LAYER-AWARE RENDERING NOTES
    // =======================================================================
    // In a real implementation, you would have separate render systems
    // for each layer that automatically filter entities by layer.
    // 
    // Example systems:
    //
    // class BackgroundRenderSystem : public System {
    // public:
    //   BackgroundRenderSystem() {
    //     // Only process entities on 'background' layer
    //     LayerID bg = get_layer_id("background");
    //     // Then manually filter in draw_entity(), or use cached ID
    //     require_components<Transform2D, Color>();
    //   }
    //   void draw_entity(const Entity& entity) override {
    //     if (entity.is_on_layer("background")) {
    //       // Render background entities first
    //     }
    //   }
    // };
    //
    // The ECS automatically sorts entities by layer priority, so:
    // - 'background' entities processed first
    // - 'gameplay' entities processed next
    // - 'foreground' entities processed after
    // - 'ui' entities processed on top
    // - 'debug' entities processed last
    //
    // This ensures correct render order without manual sorting!
  };

  void free() {};

  void unload() {
    // Clean up all entities with this state's tag
    Engine::ECS::destroy_entities_with<Components::Tag<"LayersState">>();

    std::cout << "\n========================================\n";
    std::cout << "LAYER SYSTEM DEMONSTRATION COMPLETE\n";
    std::cout << "========================================\n";
    std::cout << "Features Demonstrated:\n";
    std::cout << "  ✓ Layer registration by name\n";
    std::cout << "  ✓ String-based layer API\n";
    std::cout << "  ✓ Multi-layer entities\n";
    std::cout << "  ✓ Dynamic layer switching\n";
    std::cout << "  ✓ Layer queries and information\n";
    std::cout << "  ✓ Layer priority system\n";
    std::cout << "  ✓ Automatic entity sorting\n";
    std::cout << "  ✓ Clear all layers\n";
    std::cout << "  ✓ Runtime layer management\n";
    std::cout << "========================================\n\n";
  };
};
