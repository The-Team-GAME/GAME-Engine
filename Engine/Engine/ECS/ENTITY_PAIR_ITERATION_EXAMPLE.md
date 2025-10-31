# Entity Pair Iteration - Usage Examples

The `for_each_entity_pair` function allows you to iterate over pairs of entities from different layers, perfect for collision detection and other entity-pair operations.

## Basic Usage

```cpp
// Get layer IDs
LayerID player_layer = Engine::ECS::get_layer_id("player");
LayerID bullet_layer = Engine::ECS::get_layer_id("bullet");

// Option 1: Inline construction with initializer list
Engine::ECS::for_each_entity_pair(
    Engine::ECS::LayerSignature{player_layer},  // LHS: entities on player layer
    Engine::ECS::LayerSignature{bullet_layer},  // RHS: entities on bullet layer
    [](Engine::ECS::Entity lhs, Engine::ECS::Entity rhs) {
        // lhs is a player entity, rhs is a bullet entity
        // Check collision here
    }
);

// Option 2: Build masks explicitly
Engine::ECS::LayerSignature lhs_mask;
lhs_mask.set(player_layer);

Engine::ECS::LayerSignature rhs_mask;
rhs_mask.set(bullet_layer);

Engine::ECS::for_each_entity_pair(lhs_mask, rhs_mask, 
    [](Engine::ECS::Entity lhs, Engine::ECS::Entity rhs) {
        // Check collision
    }
);
```

## Multiple Layers

```cpp
LayerID player_layer = Engine::ECS::get_layer_id("player");
LayerID enemy_layer = Engine::ECS::get_layer_id("enemy");
LayerID object_layer = Engine::ECS::get_layer_id("object");
LayerID bullet_layer = Engine::ECS::get_layer_id("bullet");

// Option 1: Inline construction with initializer list (RECOMMENDED for multiple layers!)
Engine::ECS::for_each_entity_pair(
    Engine::ECS::LayerSignature{player_layer, enemy_layer},  // LHS: players and enemies
    Engine::ECS::LayerSignature{bullet_layer, object_layer}, // RHS: bullets and objects
    [](Engine::ECS::Entity lhs, Engine::ECS::Entity rhs) {
        // lhs is either a player or enemy
        // rhs is either a bullet or object
        // Check collision
    }
);

// Option 2: Build masks explicitly
Engine::ECS::LayerSignature lhs_mask;
lhs_mask.set(player_layer);
lhs_mask.set(enemy_layer);

Engine::ECS::LayerSignature rhs_mask;
rhs_mask.set(bullet_layer);
rhs_mask.set(object_layer);

Engine::ECS::for_each_entity_pair(lhs_mask, rhs_mask, 
    [](Engine::ECS::Entity lhs, Engine::ECS::Entity rhs) {
        // Check collision
    }
);
```

## Collision Detection Example

```cpp
void CollisionSystem::update() {
    LayerID bullet_layer = Engine::ECS::get_layer_id("bullet");
    LayerID enemy_layer = Engine::ECS::get_layer_id("enemy");
    
    // Player bullets hitting enemies - clean inline syntax!
    Engine::ECS::for_each_entity_pair(
        Engine::ECS::LayerSignature{bullet_layer},
        Engine::ECS::LayerSignature{enemy_layer},
        [](Engine::ECS::Entity bullet, Engine::ECS::Entity enemy) {
            // Get components
            auto* bullet_transform = bullet.get_component<Components::Transform>();
            auto* bullet_collider = bullet.get_component<Components::CircleCollider>();
            auto* enemy_transform = enemy.get_component<Components::Transform>();
            auto* enemy_collider = enemy.get_component<Components::CircleCollider>();
            
            if (!bullet_transform || !bullet_collider || !enemy_transform || !enemy_collider)
                return;
            
            // Check collision
            float dx = bullet_transform->position.x - enemy_transform->position.x;
            float dy = bullet_transform->position.y - enemy_transform->position.y;
            float distance_sq = dx * dx + dy * dy;
            float radius_sum = bullet_collider->radius + enemy_collider->radius;
            
            if (distance_sq < radius_sum * radius_sum) {
                // Collision detected!
                bullet.destroy();
                // Damage enemy, etc.
            }
        }
    );
}
```

## Same Layer Collision (e.g., Physics Objects)

```cpp
LayerID physics_layer = Engine::ECS::get_layer_id("physics");

// Check collisions between all physics objects
Engine::ECS::LayerSignature physics_mask{physics_layer};

Engine::ECS::for_each_entity_pair(
    physics_mask,
    physics_mask,  // Use the same mask for both sides
    [](Engine::ECS::Entity obj1, Engine::ECS::Entity obj2) {
        // Note: The function automatically skips self-pairs (obj1 == obj2)
        // Check collision between obj1 and obj2
    }
);
```

## Performance Notes

- The function automatically skips self-pairs (when `lhs.get_id() == rhs.get_id()`)
- Entities may appear multiple times if they're on multiple layers
- Consider filtering by components in your callback for better performance
- For N LHS entities and M RHS entities, this performs N×M iterations

## Integration with Systems

```cpp
class PhysicsSystem : public Engine::ECS::System {
private:
    Engine::ECS::LayerSignature player_mask;
    Engine::ECS::LayerSignature enemy_mask;
    
public:
    PhysicsSystem() {
        require_components<Components::Transform, Components::Rigidbody>();
        
        // Cache layer masks (do this once in constructor)
        player_mask.set(Engine::ECS::get_layer_id("player"));
        enemy_mask.set(Engine::ECS::get_layer_id("enemy"));
    }
    
    void update() override {
        // First, update physics for all entities (normal system update)
        Engine::ECS::System::update();
        
        // Then, check collisions between different layer groups
        Engine::ECS::for_each_entity_pair(
            player_mask,
            enemy_mask,
            [this](Engine::ECS::Entity player, Engine::ECS::Entity enemy) {
                this->check_collision(player, enemy);
            }
        );
    }
    
private:
    void check_collision(Engine::ECS::Entity a, Engine::ECS::Entity b) {
        // Collision logic here
    }
};
```
