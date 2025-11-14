#pragma once
#include "../UndoManager.h"
#include "../../ECS/ECS.h"
#include "../../Components/Components.h"

namespace Engine {

  struct MoveEntityCommand : public IUndoCommand {
    Engine::ECS::EntityID entity_id;
    Math::Vec2D old_pos;
    Math::Vec2D new_pos;

    MoveEntityCommand(Engine::ECS::EntityID id, const Math::Vec2D& from, const Math::Vec2D& to)
      : entity_id(id), old_pos(from), new_pos(to) {
    }

    void do_action() override {
      auto entity = Engine::ECS::get_entity(entity_id);
      if (entity.has_component<Components::Transform2D>()) {
        entity.get_component<Components::Transform2D>()->position = new_pos;
      }
    }

    void undo_action() override {
      auto entity = Engine::ECS::get_entity(entity_id);
      if (entity.has_component<Components::Transform2D>()) {
        entity.get_component<Components::Transform2D>()->position = old_pos;
      }
    }
  };

  struct RotateEntityCommand : public IUndoCommand {
    Engine::ECS::EntityID entity_id;
    f32 old_rotation;
    f32 new_rotation;

    RotateEntityCommand(Engine::ECS::EntityID id, f32 from, f32 to)
      : entity_id(id), old_rotation(from), new_rotation(to) {
    }

    void do_action() override {
      auto entity = Engine::ECS::get_entity(entity_id);
      if (entity.has_component<Components::Transform2D>()) {
        entity.get_component<Components::Transform2D>()->rotation = new_rotation;
      }
    }

    void undo_action() override {
      auto entity = Engine::ECS::get_entity(entity_id);
      if (entity.has_component<Components::Transform2D>()) {
        entity.get_component<Components::Transform2D>()->rotation = old_rotation;
      }
    }
  };

  struct ScaleEntityCommand : public IUndoCommand {
    Engine::ECS::EntityID entity_id;
    Math::Vec2D old_scale;
    Math::Vec2D new_scale;

    ScaleEntityCommand(Engine::ECS::EntityID id, const Math::Vec2D& from, const Math::Vec2D& to)
      : entity_id(id), old_scale(from), new_scale(to) {
    }

    void do_action() override {
      auto entity = Engine::ECS::get_entity(entity_id);
      if (entity.has_component<Components::Transform2D>()) {
        entity.get_component<Components::Transform2D>()->scale = new_scale;
      }
    }

    void undo_action() override {
      auto entity = Engine::ECS::get_entity(entity_id);
      if (entity.has_component<Components::Transform2D>()) {
        entity.get_component<Components::Transform2D>()->scale = old_scale;
      }
    }
  };

  struct PlaceTileCommand : public Engine::IUndoCommand {
    Engine::ECS::EntityID tile_id;
    Math::Vec2D grid_pos;
    Components::AtlasEntry atlas_entry;
    f32 grid_snap;
    f32 selected_tile_index;
    std::string selected_layer;

    PlaceTileCommand(const Math::Vec2D& pos, const Components::AtlasEntry& entry, f32 snap, f32 tile_idx, const std::string& layer)
        : grid_pos(pos), atlas_entry(entry), grid_snap(snap), selected_tile_index(tile_idx), selected_layer(layer) {}

    void do_action() override {
        auto tile = Engine::ECS::create_entity()
            .add_component<Components::Tile>()
            .add_component<Components::Transform2D>(grid_pos, 0.0f, Math::Vec2D{ grid_snap, grid_snap })
            .add_component<Components::Texture>(atlas_entry.texture)
            .add_component<Components::TextureAtlas>(atlas_entry.atlas)
            .add_component<Components::Color>(1.0f, 1.0f, 1.0f, 1.0f);
        tile.get_component<Components::TextureAtlas>()->drawIndex = selected_tile_index;
        tile.clear_layers();
        tile.add_to_layer(selected_layer);
        tile_id = tile.get_id();
    }

    void undo_action() override {
        if (Engine::ECS::is_valid(tile_id)) {
            Engine::ECS::destroy_entity(tile_id);
        }
    }
};
}