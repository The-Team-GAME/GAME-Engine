//-----------------------------------------------------------------------------------
// file:    UISystem.h
// project: Monkey Stole My GPA
//
// author:  Ong Zi Ming (40%)
//          Primary author
// 
//          Ryan Ang (60%)
//          Secondary author
//
// email:   o.ziming@digipen.edu
//
// brief:   This file defines a Event-driven Clickable System that checks for entities
//          that has the clickable component and gets the mouse position of when the
//          object was clicked. (Ryan) It also handles the dynamic scaling animation when the mouse
//          hovers over the clickable entity.
//
// Copyright � 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------


#pragma once

#include "../ECS/ECS.h"
#include "../Components/Components.h"
#include "../Audio/AudioSystem.h"
#include "../InputManager.h"
#include "../FunctionRegistry.h"
#include "../Time.h"
#include "../Graphics/Renderer.h"
#include <cmath>
#include <limits>
#include <unordered_set>
#include <vector>

namespace Engine
{
  namespace Systems
  {

    namespace {
      static constexpr const char* UI_BUTTON_HOVER_EVENT = "event:/SFXUI/ButtonHover";
      static constexpr const char* UI_BUTTON_CLICK_EVENT = "event:/SFXUI/ButtonClick";
    }

    class ClickableSystem : public Engine::ECS::System {
    public:
      ClickableSystem() {
        require_components<Components::Clickable, Components::Transform2D, Components::BoxCollider>();
      }

      void update() override {
        if (Engine::Time::get_paused()) {
          clear_hover_state();
          return;
        }

        const auto mouse_screen_opt = Engine::Input::get_mouse_position_if_in_viewport();
        const auto mouse_world_opt = Engine::Input::get_mouse_world_position_if_in_viewport();
        const auto raw_mouse = Engine::Input::get_mouse_position();

        if (last_mouse_position_.has_value()) {
          float dx = raw_mouse.x - last_mouse_position_->x;
          float dy = raw_mouse.y - last_mouse_position_->y;
          if ((dx * dx + dy * dy) > 0.25f) {
            navigation_focus_active_ = false;
          }
        }
        last_mouse_position_ = raw_mouse;

        std::vector<NavigationCandidate> candidates;
        Engine::ECS::EntityID mouse_hovered_id = Engine::ECS::INVALID_ENTITY;

        for (Engine::ECS::EntityID entity_id : entities) {
          Engine::ECS::Entity entity(entity_id);
          auto* clickable = entity.get_component<Components::Clickable>();
          auto* tr = entity.get_component<Components::Transform2D>();
          auto* box_collider = entity.get_component<Components::BoxCollider>();

          if (!clickable || !tr || !box_collider) {
            continue;
          }

          update_collider_position(*box_collider, *tr, entity);

          const auto& mouse_opt = entity.has_component<Components::UI>() ? mouse_screen_opt : mouse_world_opt;
          bool inside = mouse_opt.has_value() && is_point_inside_collider(mouse_opt.value(), *box_collider);
          if (inside) {
            mouse_hovered_id = entity_id;
          }

          if (!is_navigable(entity, *clickable, *box_collider)) {
            continue;
          }

          candidates.push_back({ entity, clickable, get_collider_center(*box_collider), inside });
        }

        if (!contains_candidate(candidates, focused_entity_id_)) {
          focused_entity_id_ = Engine::ECS::INVALID_ENTITY;
        }

        if (!navigation_focus_active_ && mouse_hovered_id != Engine::ECS::INVALID_ENTITY) {
          focused_entity_id_ = mouse_hovered_id;
        }

        if (handle_navigation_input(candidates)) {
          navigation_focus_active_ = true;
        }

        if (focused_entity_id_ == Engine::ECS::INVALID_ENTITY) {
          focused_entity_id_ = choose_default_candidate(candidates);
        }

        Engine::ECS::EntityID active_hovered_id = Engine::ECS::INVALID_ENTITY;
        if (navigation_focus_active_ && focused_entity_id_ != Engine::ECS::INVALID_ENTITY) {
          active_hovered_id = focused_entity_id_;
        }
        else if (mouse_hovered_id != Engine::ECS::INVALID_ENTITY) {
          active_hovered_id = mouse_hovered_id;
        }

        sync_hover_state(active_hovered_id);

        if (mouse_hovered_id != Engine::ECS::INVALID_ENTITY && Engine::Input::is_pressed(KeyCode::MouseLeft)) {
          navigation_focus_active_ = false;
          focused_entity_id_ = mouse_hovered_id;
          invoke_click(mouse_hovered_id);
          return;
        }

        const bool focus_activate = Engine::Input::is_pressed(KeyCode::Space) || Engine::Input::is_pressed(KeyCode::Enter);
        if (navigation_focus_active_ && focus_activate && focused_entity_id_ != Engine::ECS::INVALID_ENTITY) {
          invoke_click(focused_entity_id_);
        }
      }

    private:
      struct NavigationCandidate {
        Engine::ECS::Entity entity;
        Components::Clickable* clickable;
        Math::Vec2D center;
        bool mouse_inside;
      };

      enum class NavigationDirection {
        Left,
        Right,
        Up,
        Down
      };

      // Update collider position to match transform
      void update_collider_position(Components::BoxCollider& collider,
        const Components::Transform2D& transform,
        Engine::ECS::Entity entity) {
        // Calculate flipped offset
        Vec2D flipped_offset = collider.offset;
        if (auto* flip = entity.get_component<Components::Flip>()) {
          flipped_offset.x *= flip->value.x;
          flipped_offset.y *= flip->value.y;
        }

        if (collider.type == Components::ColliderType::AABB) {
          collider.aabb.pos = transform.position + flipped_offset;
        }
        else if (collider.type == Components::ColliderType::OBB ||
          collider.type == Components::ColliderType::POLYGON) {
          collider.polygon.position = transform.position + flipped_offset;
          collider.polygon.rotation = transform.rotation;
        }
      }
      // Point-in-polygon test for any collider type
      bool is_point_inside_collider(const Math::Vec2D& point, const Components::BoxCollider& collider) {

        if (collider.type == Components::ColliderType::AABB) {
          // Convert AABB to polygon and get transformed vertices
          Polygon poly(collider.aabb);
          const std::vector<Vec2D> vertices = poly.get_transformed_vertices();
          return point_in_polygon(point, vertices);
        }
        else {
          // Polygon collision (works for OBB and POLYGON types)
          // Use point-in-polygon ray casting algorithm
          const std::vector<Vec2D> vertices = collider.polygon.get_transformed_vertices();
          return point_in_polygon(point, vertices);
        }
      }

      // Ray casting algorithm for point-in-polygon test
      bool point_in_polygon(const Math::Vec2D& point, const std::vector<Vec2D>& vertices) {
        bool inside = false;
        size_t n = vertices.size();

        for (size_t i = 0, j = n - 1; i < n; j = i++) {
          const Vec2D& vi = vertices[i];
          const Vec2D& vj = vertices[j];

          // Check if point is on horizontal ray crossing
          if (((vi.y > point.y) != (vj.y > point.y)) &&
            (point.x < (vj.x - vi.x) * (point.y - vi.y) / (vj.y - vi.y) + vi.x)) {
            inside = !inside;
          }
        }

        return inside;
      }

      Math::Vec2D get_collider_center(const Components::BoxCollider& collider) const {
        std::vector<Vec2D> vertices;
        if (collider.type == Components::ColliderType::AABB) {
          Polygon poly(collider.aabb);
          vertices = poly.get_transformed_vertices();
        }
        else {
          vertices = collider.polygon.get_transformed_vertices();
        }

        Math::Vec2D center(0.0f, 0.0f);
        if (vertices.empty()) {
          return center;
        }

        for (const auto& vertex : vertices) {
          center += vertex;
        }
        const float inverse_count = 1.0f / static_cast<float>(vertices.size());
        center.x *= inverse_count;
        center.y *= inverse_count;
        return center;
      }

      bool is_navigable(Engine::ECS::Entity entity,
        const Components::Clickable& clickable,
        const Components::BoxCollider& collider) const {
        if (clickable.on_click.empty()) {
          return false;
        }

        if (auto* color = entity.get_component<Components::Color>()) {
          if (color->a <= 0.01f) {
            return false;
          }
        }

        const auto center = get_collider_center(collider);
        const auto vres = Engine::Graphics::get_virtual_resolution();
        constexpr float margin = 96.0f;
        return center.x >= -margin
          && center.x <= vres.x + margin
          && center.y >= -margin
          && center.y <= vres.y + margin;
      }

      bool contains_candidate(const std::vector<NavigationCandidate>& candidates, Engine::ECS::EntityID id) const {
        for (const auto& candidate : candidates) {
          if (candidate.entity.get_id() == id) {
            return true;
          }
        }
        return false;
      }

      Engine::ECS::EntityID choose_default_candidate(const std::vector<NavigationCandidate>& candidates) const {
        Engine::ECS::EntityID best_id = Engine::ECS::INVALID_ENTITY;
        float best_y = std::numeric_limits<float>::max();
        float best_x = std::numeric_limits<float>::max();

        for (const auto& candidate : candidates) {
          if (candidate.center.y < best_y ||
            (std::abs(candidate.center.y - best_y) < 0.5f && candidate.center.x < best_x)) {
            best_id = candidate.entity.get_id();
            best_y = candidate.center.y;
            best_x = candidate.center.x;
          }
        }

        return best_id;
      }

      Engine::ECS::EntityID find_navigation_target(const std::vector<NavigationCandidate>& candidates,
        Engine::ECS::EntityID current_id,
        NavigationDirection direction) const {
        const NavigationCandidate* current = nullptr;
        for (const auto& candidate : candidates) {
          if (candidate.entity.get_id() == current_id) {
            current = &candidate;
            break;
          }
        }

        if (!current) {
          return choose_default_candidate(candidates);
        }

        Engine::ECS::EntityID best_id = Engine::ECS::INVALID_ENTITY;
        float best_score = std::numeric_limits<float>::max();

        for (const auto& candidate : candidates) {
          if (candidate.entity.get_id() == current_id) {
            continue;
          }

          const float dx = candidate.center.x - current->center.x;
          const float dy = candidate.center.y - current->center.y;
          float primary = 0.0f;
          float secondary = 0.0f;

          switch (direction) {
          case NavigationDirection::Left:
            if (dx >= -0.01f) continue;
            primary = -dx;
            secondary = std::abs(dy);
            break;
          case NavigationDirection::Right:
            if (dx <= 0.01f) continue;
            primary = dx;
            secondary = std::abs(dy);
            break;
          case NavigationDirection::Up:
            if (dy >= -0.01f) continue;
            primary = -dy;
            secondary = std::abs(dx);
            break;
          case NavigationDirection::Down:
            if (dy <= 0.01f) continue;
            primary = dy;
            secondary = std::abs(dx);
            break;
          }

          const float score = primary + secondary * 2.5f;
          if (score < best_score) {
            best_score = score;
            best_id = candidate.entity.get_id();
          }
        }

        return best_id == Engine::ECS::INVALID_ENTITY ? current_id : best_id;
      }

      bool handle_navigation_input(const std::vector<NavigationCandidate>& candidates) {
        NavigationDirection direction = NavigationDirection::Left;
        bool has_direction = false;

        if (Engine::Input::is_pressed(KeyCode::Left)) {
          direction = NavigationDirection::Left;
          has_direction = true;
        }
        else if (Engine::Input::is_pressed(KeyCode::Right)) {
          direction = NavigationDirection::Right;
          has_direction = true;
        }
        else if (Engine::Input::is_pressed(KeyCode::Up)) {
          direction = NavigationDirection::Up;
          has_direction = true;
        }
        else if (Engine::Input::is_pressed(KeyCode::Down)) {
          direction = NavigationDirection::Down;
          has_direction = true;
        }

        if (!has_direction || candidates.empty()) {
          return false;
        }

        if (focused_entity_id_ == Engine::ECS::INVALID_ENTITY) {
          focused_entity_id_ = choose_default_candidate(candidates);
          return true;
        }

        focused_entity_id_ = find_navigation_target(candidates, focused_entity_id_, direction);
        return true;
      }

      void clear_hover_state() {
        for (Engine::ECS::EntityID id : hovered_entities_) {
          call_exit_hover(id);
        }
        hovered_entities_.clear();
        focused_entity_id_ = Engine::ECS::INVALID_ENTITY;
        navigation_focus_active_ = false;
      }

      void sync_hover_state(Engine::ECS::EntityID active_hovered_id) {
        std::unordered_set<Engine::ECS::EntityID> next_hovered;
        if (active_hovered_id != Engine::ECS::INVALID_ENTITY) {
          next_hovered.insert(active_hovered_id);
        }

        for (Engine::ECS::EntityID id : hovered_entities_) {
          if (!next_hovered.contains(id)) {
            call_exit_hover(id);
          }
        }

        if (active_hovered_id != Engine::ECS::INVALID_ENTITY) {
          if (!hovered_entities_.contains(active_hovered_id)) {
            auto entity = Engine::ECS::Entity(active_hovered_id);
            if (entity.is_valid()) {
              if (auto* clickable = entity.get_component<Components::Clickable>()) {
                if (!clickable->on_click.empty()) {
                  Engine::AudioSystem::playEvent(UI_BUTTON_HOVER_EVENT);
                }
              }
            }
          }
          call_hover(active_hovered_id);
        }

        hovered_entities_ = std::move(next_hovered);
      }

      void call_hover(Engine::ECS::EntityID id) {
        auto entity = Engine::ECS::Entity(id);
        if (!entity.is_valid()) {
          return;
        }

        auto* clickable = entity.get_component<Components::Clickable>();
        if (!clickable || clickable->on_hover.empty()) {
          return;
        }

        auto hover_func = clickable->on_hover.get<void(Engine::ECS::Entity)>();
        if (hover_func) {
          hover_func(entity);
        }
      }

      void call_exit_hover(Engine::ECS::EntityID id) {
        auto entity = Engine::ECS::Entity(id);
        if (!entity.is_valid()) {
          return;
        }

        auto* clickable = entity.get_component<Components::Clickable>();
        if (!clickable || clickable->exit_hover.empty()) {
          return;
        }

        auto exit_func = clickable->exit_hover.get<void(Engine::ECS::Entity)>();
        if (exit_func) {
          exit_func(entity);
        }
      }

      void invoke_click(Engine::ECS::EntityID id) {
        auto entity = Engine::ECS::Entity(id);
        if (!entity.is_valid()) {
          return;
        }

        auto* clickable = entity.get_component<Components::Clickable>();
        if (!clickable || clickable->on_click.empty()) {
          return;
        }

        auto click_func = clickable->on_click.get<void()>();
        if (!click_func) {
          return;
        }

        Engine::AudioSystem::playEvent(UI_BUTTON_CLICK_EVENT);
        click_func();
        Engine::ECS::process_housekeeping();
      }

      std::unordered_set<Engine::ECS::EntityID> hovered_entities_;
      Engine::ECS::EntityID focused_entity_id_ = Engine::ECS::INVALID_ENTITY;
      bool navigation_focus_active_ = false;
      std::optional<glm::vec2> last_mouse_position_;
    };

    // System to handle dynamic button collider updates
    class DynamicButtonSystem : public Engine::ECS::System {
    public:
      DynamicButtonSystem() {
        require_components<Components::DynamicButton, Components::Transform2D, Components::BoxCollider>();
      }

      void update_entity(Engine::ECS::Entity entity) override {
        auto* dynamic_button = entity.get_component<Components::DynamicButton>();
        auto* box_collider = entity.get_component<Components::BoxCollider>();
        auto* transform = entity.get_component<Components::Transform2D>();

        if (!dynamic_button || !box_collider || !transform) return;

        // TODO: very inefficient to initialize every frame, optimize later
        // Initialize DynamicButton with current collider values if not set
        bool needs_init = false;
        if (box_collider->type == Components::ColliderType::AABB) {
          needs_init = (dynamic_button->normal.width == 0.0f && dynamic_button->normal.height == 0.0f);
        }
        else {
          needs_init = dynamic_button->normal.vertices.empty();
        }

        if (needs_init) {
          if (box_collider->type == Components::ColliderType::AABB) {
            dynamic_button->normal.width = box_collider->width;
            dynamic_button->normal.height = box_collider->height;
            dynamic_button->hovered.width = box_collider->width;
            dynamic_button->hovered.height = box_collider->height;
            dynamic_button->clicked.width = box_collider->width;
            dynamic_button->clicked.height = box_collider->height;
          }
          else {
            dynamic_button->normal.vertices = box_collider->polygon.vertices;
            dynamic_button->hovered.vertices = box_collider->polygon.vertices;
            dynamic_button->clicked.vertices = box_collider->polygon.vertices;
          }

          // Initialize transform data from current transform
          dynamic_button->normal.transform = *transform;
          dynamic_button->hovered.transform = *transform;
          dynamic_button->clicked.transform = *transform;
        }        // UI entities use screen-space coordinates (not affected by camera)
        const auto mouse_opt = entity.has_component<Components::UI>()
          ? Engine::Input::get_mouse_position_if_in_viewport()
          : Engine::Input::get_mouse_world_position_if_in_viewport();

        if (!mouse_opt.has_value()) {
          // Mouse not in viewport - ensure button is in normal state
          if (dynamic_button->is_hovered) {
            apply_normal_state(*dynamic_button, *box_collider, *transform);
            dynamic_button->is_hovered = false;
          }
          return;
        }

        const Math::Vec2D& mouse = mouse_opt.value();

        // Update collider position
        auto* tr = entity.get_component<Components::Transform2D>();
        update_collider_position(*box_collider, *tr, entity);

        // Check if mouse is inside
        bool inside = is_point_inside_collider(mouse, *box_collider);
        bool is_mouse_down = Engine::Input::is_pressed(KeyCode::MouseLeft);

        // State priority: Clicked > Hovered > Normal
        if (inside && is_mouse_down) {
          // Mouse is down on button - apply clicked state
          if (!dynamic_button->is_clicked) {
            start_transition(*dynamic_button, dynamic_button->clicked, *transform);
            dynamic_button->is_clicked = true;
            dynamic_button->is_hovered = false;
          }
        }
        else if (inside && !is_mouse_down) {
          // Mouse is hovering but not clicked
          if (!dynamic_button->is_hovered || dynamic_button->is_clicked) {
            start_transition(*dynamic_button, dynamic_button->hovered, *transform);
            dynamic_button->is_hovered = true;
            dynamic_button->is_clicked = false;
          }
        }
        else {
          // Mouse is outside - return to normal state
          if (dynamic_button->is_hovered || dynamic_button->is_clicked) {
            start_transition(*dynamic_button, dynamic_button->normal, *transform);
            dynamic_button->is_hovered = false;
            dynamic_button->is_clicked = false;
          }
        }

        // Update transition animation
        if (dynamic_button->target_state) {
          update_transition(*dynamic_button, *box_collider, *transform);
        }
      }

    private:
      void start_transition(Components::DynamicButton& button, Components::DynamicButton::StateData& target, const Components::Transform2D& current_transform) {
        button.transition_start_time = Engine::Time::get_time();
        button.transition_start_transform = current_transform;
        button.target_state = &target;
      }

      void update_transition(Components::DynamicButton& button, Components::BoxCollider& collider, Components::Transform2D& transform) {
        if (!button.target_state) return;

        float elapsed = Engine::Time::get_time() - button.transition_start_time;
        float t = std::min(elapsed / button.transition_duration, 1.0f);

        const auto& target = *button.target_state;
        const auto& start = button.transition_start_transform;

        // Apply easing to position
        Math::Vec2D final_position = target.transform.position;
        if (!target.position_easing.empty()) {
          auto easing_func = target.position_easing.get<f32(f32, f32, f32, f32)>();
          if (easing_func) {
            float eased_x = easing_func(elapsed, start.position.x, target.transform.position.x - start.position.x, button.transition_duration);
            float eased_y = easing_func(elapsed, start.position.y, target.transform.position.y - start.position.y, button.transition_duration);
            final_position = { eased_x, eased_y };
          }
        }
        else {
          final_position = start.position + (target.transform.position - start.position) * t;
        }

        // Apply easing to rotation
        float final_rotation = target.transform.rotation;
        if (!target.rotation_easing.empty()) {
          auto easing_func = target.rotation_easing.get<f32(f32, f32, f32, f32)>();
          if (easing_func) {
            final_rotation = easing_func(elapsed, start.rotation, target.transform.rotation - start.rotation, button.transition_duration);
          }
        }
        else {
          final_rotation = start.rotation + (target.transform.rotation - start.rotation) * t;
        }

        // Apply easing to scale
        Math::Vec2D final_scale = target.transform.scale;
        if (!target.scale_easing.empty()) {
          auto easing_func = target.scale_easing.get<f32(f32, f32, f32, f32)>();
          if (easing_func) {
            float eased_x = easing_func(elapsed, start.scale.x, target.transform.scale.x - start.scale.x, button.transition_duration);
            float eased_y = easing_func(elapsed, start.scale.y, target.transform.scale.y - start.scale.y, button.transition_duration);
            final_scale = { eased_x, eased_y };
          }
        }
        else {
          final_scale = start.scale + (target.transform.scale - start.scale) * t;
        }

        // Apply interpolated transform
        transform.position = final_position;
        transform.rotation = final_rotation;
        transform.scale = final_scale;

        // Lerp collider dimensions
        if (collider.type == Components::ColliderType::AABB) {
          collider.width = target.width;
          collider.height = target.height;
          collider.aabb.half_width = target.width / 2.0f;
          collider.aabb.half_height = target.height / 2.0f;
        }
        else if (collider.type == Components::ColliderType::OBB ||
          collider.type == Components::ColliderType::POLYGON) {
          if (!target.vertices.empty()) {
            collider.polygon.vertices = target.vertices;
          }
        }

        // Clear target state when transition is complete
        if (t >= 1.0f) {
          button.target_state = nullptr;
        }
      }

      void apply_normal_state(const Components::DynamicButton& button, Components::BoxCollider& collider, Components::Transform2D& transform) {
        if (collider.type == Components::ColliderType::AABB) {
          // Update AABB dimensions
          collider.width = button.normal.width;
          collider.height = button.normal.height;
          collider.aabb.half_width = button.normal.width / 2.0f;
          collider.aabb.half_height = button.normal.height / 2.0f;
        }
        else if (collider.type == Components::ColliderType::OBB ||
          collider.type == Components::ColliderType::POLYGON) {
          // Update polygon vertices
          if (!button.normal.vertices.empty()) {
            collider.polygon.vertices = button.normal.vertices;
          }
        }

        // Apply transform
        transform = button.normal.transform;
      }

      void update_collider_position(Components::BoxCollider& collider,
        const Components::Transform2D& transform,
        Engine::ECS::Entity entity) {
        Vec2D flipped_offset = collider.offset;
        if (auto* flip = entity.get_component<Components::Flip>()) {
          flipped_offset.x *= flip->value.x;
          flipped_offset.y *= flip->value.y;
        }

        if (collider.type == Components::ColliderType::AABB) {
          collider.aabb.pos = transform.position + flipped_offset;
        }
        else if (collider.type == Components::ColliderType::OBB ||
          collider.type == Components::ColliderType::POLYGON) {
          collider.polygon.position = transform.position + flipped_offset;
          collider.polygon.rotation = transform.rotation;
        }
      }

      bool is_point_inside_collider(const Math::Vec2D& point, const Components::BoxCollider& collider) {
        if (collider.type == Components::ColliderType::AABB) {
          Polygon poly(collider.aabb);
          const std::vector<Vec2D> vertices = poly.get_transformed_vertices();
          return point_in_polygon(point, vertices);
        }
        else {
          const std::vector<Vec2D> vertices = collider.polygon.get_transformed_vertices();
          return point_in_polygon(point, vertices);
        }
      }

      bool point_in_polygon(const Math::Vec2D& point, const std::vector<Vec2D>& vertices) {
        bool inside = false;
        size_t n = vertices.size();

        for (size_t i = 0, j = n - 1; i < n; j = i++) {
          const Vec2D& vi = vertices[i];
          const Vec2D& vj = vertices[j];

          if (((vi.y > point.y) != (vj.y > point.y)) &&
            (point.x < (vj.x - vi.x) * (point.y - vi.y) / (vj.y - vi.y) + vi.x)) {
            inside = !inside;
          }
        }

        return inside;
      }
    };

  } // namespace Systems
} // namespace Engine