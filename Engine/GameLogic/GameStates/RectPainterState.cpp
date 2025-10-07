//-----------------------------------------------------------------------------------
// file:    RectPainterState.cpp
// project: Monkey Stole My GPA
//
// author:  Ong Zi Ming (100%)
//          Primary author
//
// email:   o.ziming@digipen.edu
//
// brief:   Defines functions that creates and deletes rectangle objects that can be 
// edited to change position, size and color. Multiple objects can be created or 
// deleted and collision detection is present. Object collision can also be turned off.
// Objects can also be duplicated.
//
// Copyright � 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------

#include "RectPainterState.h"
#include "../../Engine/MonkeyEngine.h"
#include <imgui.h>
#include <algorithm>
#include <cstdio>
#include <unordered_map>

using ECS::EntityID;

namespace Components {
  struct BasicCollider {
    bool enabled = true;
    bool isTrigger = false;
    BasicCollider(bool e = true, bool t = false) : enabled(e), isTrigger(t) {}
  };
}

void RectPainterState::load() {}
void RectPainterState::init()
{
  ECS::World::add_system<Systems::StaticSpriteRenderSystem>();

  rects_.clear();
  selectedIndex_ = -1;
  dragMode_ = DragMode::None;
  passedThreshold_ = false;
  globalSnap_ = false;

  if (startDragThresholdPx_ <= 0.f) startDragThresholdPx_ = 6.f;
  if (minRectSizePx_ <= 0.f) minRectSizePx_ = 8.f;
  if (handlePx_ <= 0.f) handlePx_ = 8.f;
  if (handleHoverInflate_ < 0.f) handleHoverInflate_ = 4.f;
  if (snapGridPx_ <= 0.f) snapGridPx_ = 16.f;

}

void RectPainterState::free()
{
  // Destroy all entities from this state
  ECS::World::destroy_entities_with<Components::Tag<"RectPainter">>();
  rects_.clear();
  selectedIndex_ = -1;
  dragMode_ = DragMode::None;

  // Remove systems added in Init
  ECS::World::remove_system<Systems::StaticSpriteRenderSystem>();
}

void RectPainterState::unload() {}

void RectPainterState::update()
{
  handleShortcuts();
  handleMouse();
}

void RectPainterState::draw()
{
  Engine::Graphics::clear_screen({ 0.08f, 0.08f, 0.10f, 1.0f });

  // Preview while drawing
  if (dragMode_ == DragMode::Drawing && passedThreshold_) {
    glm::vec2 center, size;
    computeCenterAndSize(dragStart_, prevMouse_, center, size);
    clampMinSize(size, minRectSizePx_);
    // preview respects snap when Shift or global snap active
    const bool shift = Engine::Input::is_key_held(GLFW_KEY_LEFT_SHIFT) || Engine::Input::is_key_held(GLFW_KEY_RIGHT_SHIFT);
    if (globalSnap_ || shift) {
      center = snapToGrid(center, snapGridPx_);
      size.x = snapScalar(size.x, snapGridPx_);
      size.y = snapScalar(size.y, snapGridPx_);
    }

    Engine::Graphics::draw_rect(
      { center.x, center.y },
      { size.x, size.y },
      0.0f,
      { currentColor_.x  , currentColor_.y  , currentColor_.z  , currentColor_.w }
    );
  }

  runCollisionPass();
  drawSelectionGizmo();
  drawImGui();
}

void RectPainterState::handleMouse()
{
  ImGuiIO& io = ImGui::GetIO();
  if (io.WantCaptureMouse) {
    prevMouse_ = Engine::Input::GetMouseWorldPosition();
    return;
  }

  const glm::vec2 mouse = Engine::Input::GetMouseWorldPosition();
  const bool lDown = Engine::Input::is_mouse_button_pressed(GLFW_MOUSE_BUTTON_LEFT);
  const bool lHeld = Engine::Input::is_mouse_button_held(GLFW_MOUSE_BUTTON_LEFT);
  const bool lUp = Engine::Input::is_mouse_button_released(GLFW_MOUSE_BUTTON_LEFT);
  const bool shift = Engine::Input::is_key_held(GLFW_KEY_LEFT_SHIFT) || Engine::Input::is_key_held(GLFW_KEY_RIGHT_SHIFT);
  const bool snap = (globalSnap_ || shift);

  // Esc clears selection
  if (Engine::Input::is_key_pressed(GLFW_KEY_ESCAPE)) { selectedIndex_ = -1; dragMode_ = DragMode::None; }

  // Mouse down: decide mode
  if (lDown) {
    passedThreshold_ = false;

    // If we already have a selection, check handles first
    if (selectedIndex_ >= 0 && selectedIndex_ < (int)rects_.size()) {
      glm::vec2 minP, maxP;
      if (getBounds(rects_[selectedIndex_], minP, maxP)) {
        ResizeMask mask = hitHandle(mouse, minP, maxP);
        if (mask.l || mask.r || mask.t || mask.b) {
          dragMode_ = DragMode::Resizing;
          resize_ = mask;
          dragStart_ = mouse;
          prevMouse_ = mouse;
          return;
        }
        if (pointInRect(mouse, minP, maxP)) {
          dragMode_ = DragMode::Moving;
          auto* transform = ECS::World::get_component<Components::Transform2D>(rects_[selectedIndex_]);
          moveGrabOffset_ = mouse - glm::vec2{ transform->position.x, transform->position.y };
          prevMouse_ = mouse;
          return;
        }
      }
    }

    // Try to pick another rect
    s32 pick = pickTopmostRect(mouse);
    if (pick >= 0) {
      selectedIndex_ = pick;
      // Second click starts move immediately if inside body
      glm::vec2 minP, maxP;
      if (getBounds(rects_[selectedIndex_], minP, maxP) && pointInRect(mouse, minP, maxP)) {
        dragMode_ = DragMode::Moving;
        auto* transform = ECS::World::get_component<Components::Transform2D>(rects_[selectedIndex_]);
        moveGrabOffset_ = mouse - glm::vec2{ transform->position.x, transform->position.y };
      }
      else {
        dragMode_ = DragMode::None;
      }
      prevMouse_ = mouse;
      return;
    }

    // Else start drawing new rect
    dragStart_ = prevMouse_ = mouse;
    dragMode_ = DragMode::Drawing;
  }

  // Mouse move updates
  if (lHeld) {
    if (!passedThreshold_) {
      if (glm::length(mouse - dragStart_) >= startDragThresholdPx_) passedThreshold_ = true;
    }

    if (dragMode_ == DragMode::Moving && selectedIndex_ >= 0) {
      auto id = rects_[selectedIndex_];
      if (auto* transform = ECS::World::get_component<Components::Transform2D>(id)) {
        glm::vec2 newCenter = mouse - moveGrabOffset_;
        if (snap) newCenter = snapToGrid(newCenter, snapGridPx_);
        transform->position.x = newCenter.x;
        transform->position.y = newCenter.y;
      }
    }
    else if (dragMode_ == DragMode::Resizing && selectedIndex_ >= 0) {
      auto id = rects_[selectedIndex_];
      auto* transform = ECS::World::get_component<Components::Transform2D>(id);

      if (transform) {
        Math::Vec2D& pos = transform->position;
        Math::Vec2D& size = transform->scale;

        glm::vec2 minP{ pos.x - size.x * 0.5f,  pos.y - size.y * 0.5f };
        glm::vec2 maxP{ pos.x + size.x * 0.5f,  pos.y + size.y * 0.5f };

        glm::vec2 m = mouse;
        if (snap) m = snapToGrid(m, snapGridPx_);

        // Adjust sides
        if (resize_.l) minP.x = std::min(mouse.x, maxP.x - minRectSizePx_);
        if (resize_.r) maxP.x = std::max(mouse.x, minP.x + minRectSizePx_);
        if (resize_.t) minP.y = std::min(mouse.y, maxP.y - minRectSizePx_);
        if (resize_.b) maxP.y = std::max(mouse.y, minP.y + minRectSizePx_);

        // Recompute center/size
        pos.x = (minP.x + maxP.x) * 0.5f;
        pos.y = (minP.y + maxP.y) * 0.5f;
        size.x = std::max(minRectSizePx_, maxP.x - minP.x);
        size.y = std::max(minRectSizePx_, maxP.y - minP.y);

        if (snap) {
          // snap edges to keep alignment tidy
          glm::vec2 newMin{ pos.x - size.x * 0.5f,  pos.y - size.y * 0.5f };
          glm::vec2 newMax{ pos.x + size.x * 0.5f,  pos.y + size.y * 0.5f };
          newMin = snapToGrid(newMin, snapGridPx_);
          newMax = snapToGrid(newMax, snapGridPx_);
          pos.x = (newMin.x + newMax.x) * 0.5f;
          pos.y = (newMin.y + newMax.y) * 0.5f;
          size.x = std::max(minRectSizePx_, newMax.x - newMin.x);
          size.y = std::max(minRectSizePx_, newMax.y - newMin.y);
        }

      }
    }

    if (dragMode_ == DragMode::Drawing) {
      prevMouse_ = snap ? snapToGrid(mouse, snapGridPx_) : mouse;
    }
  }

  // Mouse up: finalize
  if (lUp) {
    if (dragMode_ == DragMode::Drawing && passedThreshold_) {
      glm::vec2 end = snap ? snapToGrid(mouse, snapGridPx_) : mouse;
      glm::vec2 center, size;
      computeCenterAndSize(dragStart_, mouse, center, size);
      clampMinSize(size, minRectSizePx_);
      if (size.x >= minRectSizePx_ && size.y >= minRectSizePx_) {
        auto entity = ECS::World::create_entity()
          .add_component<Components::Tag<"RectPainter">>()
          .add_component<Components::Transform2D>(center.x, center.y, 0.0f, size.x, size.y)
          .add_component<Components::Color>(currentColor_.x, currentColor_.y, currentColor_.z, currentColor_.w)
          .add_component<Components::BasicCollider>(spawnWithCollider_, false);
        rects_.push_back(entity.get_id());
        selectedIndex_ = (int)rects_.size() - 1;
      }
    }
    dragMode_ = DragMode::None;
    passedThreshold_ = false;
  }

  prevMouse_ = mouse;
}

void RectPainterState::handleShortcuts()
{
  ImGuiIO& io = ImGui::GetIO();
  if (io.WantTextInput) return; // don't steal while typing
  const bool ctrl = Engine::Input::is_key_held(GLFW_KEY_LEFT_CONTROL) || Engine::Input::is_key_held(GLFW_KEY_RIGHT_CONTROL);
  const bool hasSel = (selectedIndex_ >= 0 && selectedIndex_ < (int)rects_.size());

  if (hasSel && Engine::Input::is_key_pressed(GLFW_KEY_DELETE)) {
    ECS::World::destroy_entity(rects_[selectedIndex_]);
    rects_.erase(rects_.begin() + selectedIndex_);
    selectedIndex_ = -1;
  }

  if (hasSel && ctrl && Engine::Input::is_key_pressed(GLFW_KEY_D)) {
    const EntityID src = rects_[selectedIndex_];

    auto* translate = ECS::World::get_component<Components::Transform2D>(src);

    auto* c = ECS::World::get_component<Components::Color>(src);
    auto* col = ECS::World::get_component<Components::BasicCollider>(src);
    if (translate && c) {
      auto e = ECS::World::create_entity()
        .add_component<Components::Tag<"RectPainter">>()
        .add_component<Components::Transform2D>(translate->position.x + 10.f, translate->position.y + 10.f, 0.0f, translate->scale.x, translate->scale.y)
        .add_component<Components::Color>(c->r(), c->g(), c->b(), c->a());
      if (col) e.add_component<Components::BasicCollider>(col->enabled, col->isTrigger);
      rects_.push_back(e.get_id());
      selectedIndex_ = (int)rects_.size() - 1;
    }
  }

  if (hasSel && Engine::Input::is_key_pressed(GLFW_KEY_LEFT_BRACKET))  sendToBack(selectedIndex_);
  if (hasSel && Engine::Input::is_key_pressed(GLFW_KEY_RIGHT_BRACKET)) bringToFront(selectedIndex_);

  if (Engine::Input::is_key_pressed(GLFW_KEY_G)) globalSnap_ = !globalSnap_;

  //if (ctrl && Engine::Input::is_key_pressed(GLFW_KEY_S)) ;
  //if (ctrl && Engine::Input::is_key_pressed(GLFW_KEY_O)) ;
}

void RectPainterState::runCollisionPass()
{
  // transient per-frame collision flags
  static std::unordered_map<EntityID, HitInfo> hits;
  hits.clear();

  // Brute-force AABB among collider-enabled rects
  for (size_t i = 0; i < rects_.size(); ++i) {
    auto idA = rects_[i];
    auto* colA = ECS::World::get_component<Components::BasicCollider>(idA);
    if (!colA || !colA->enabled) continue;

    glm::vec2 aMin, aMax;
    if (!getBounds(idA, aMin, aMax)) continue;

    for (size_t j = i + 1; j < rects_.size(); ++j) {
      auto idB = rects_[j];
      auto* colB = ECS::World::get_component<Components::BasicCollider>(idB);
      if (!colB || !colB->enabled) continue;

      glm::vec2 bMin, bMax;
      if (!getBounds(idB, bMin, bMax)) continue;

      if (aabbOverlap(aMin, aMax, bMin, bMax)) {
        hits[idA].colliding = true;
        hits[idB].colliding = true;
      }
    }
  }

  // Debug overlay (red)
  for (auto id : rects_) {
    if (!hits[id].colliding) continue;
    glm::vec2 minP, maxP;
    if (!getBounds(id, minP, maxP)) continue;
    glm::vec2 c{ (minP.x + maxP.x) * 0.5f, (minP.y + maxP.y) * 0.5f };
    glm::vec2 s{ std::max(1.0f, maxP.x - minP.x), std::max(1.0f, maxP.y - minP.y) };
    Engine::Graphics::draw_rect(
      { c.x, c.y },
      { s.x + 6.0f, s.y + 6.0f },
      0.0f,
      { 1.0f, 0.0f, 0.0f, 0.5f }
    );
  }
}

/* Overlay */
void RectPainterState::drawSelectionGizmo()
{
  if (selectedIndex_ < 0 || selectedIndex_ >= (int)rects_.size()) return;

  glm::vec2 minP, maxP;
  if (!getBounds(rects_[selectedIndex_], minP, maxP)) return;

  // Slight highlight box
  glm::vec2 center{ (minP.x + maxP.x) * 0.5f, (minP.y + maxP.y) * 0.5f };
  glm::vec2 size{ std::max(1.0f, maxP.x - minP.x), std::max(1.0f, maxP.y - minP.y) };
  Engine::Graphics::draw_rect(
    { center.x, center.y },
    { size.x + 4.0f, size.y + 4.0f },
    0.0f,
    { 1.0f, 1.0f, 0.0f, 0.15f }
  );

  // 8 handles
  const f32 hs = handlePx_;
  const glm::vec2 c = center;
  const glm::vec2 s = size * 0.5f;
  const glm::vec2 pts[8] = {
    {c.x - s.x, c.y - s.y}, {c.x, c.y - s.y}, {c.x + s.x, c.y - s.y},
    {c.x - s.x, c.y},                           {c.x + s.x, c.y},
    {c.x - s.x, c.y + s.y}, {c.x, c.y + s.y}, {c.x + s.x, c.y + s.y},
  };
  for (auto& p : pts) {
    Engine::Graphics::draw_rect(
      { p.x, p.y },
      { hs, hs },
      0.0f,
      { 1.0f, 1.0f, 0.0f, 0.5f }
    );
  }
}

/* Picking helpers */
bool RectPainterState::getBounds(EntityID id, glm::vec2& minP, glm::vec2& maxP) const
{
  auto* transform = ECS::World::get_component<Components::Transform2D>(id);


  if (!transform) return false;
  minP = { transform->position.x - transform->scale.x * 0.5f,  transform->position.y - transform->scale.y * 0.5f };
  maxP = { transform->position.x + transform->scale.x * 0.5f,  transform->position.y + transform->scale.y * 0.5f };
  return true;
}

bool RectPainterState::pointInRect(const glm::vec2& p, const glm::vec2& minP, const glm::vec2& maxP)
{
  return (p.x >= minP.x && p.x <= maxP.x && p.y >= minP.y && p.y <= maxP.y);
}

RectPainterState::ResizeMask RectPainterState::hitHandle(const glm::vec2& p, const glm::vec2& minP, const glm::vec2& maxP) const
{
  ResizeMask m{};
  const f32 hs = handlePx_ * 0.5f + handleHoverInflate_;
  const glm::vec2 c{ (minP.x + maxP.x) * 0.5f, (minP.y + maxP.y) * 0.5f };
  const glm::vec2 s{ (maxP.x - minP.x) * 0.5f, (maxP.y - minP.y) * 0.5f };

  auto inBox = [&](glm::vec2 pc) { return std::abs(p.x - pc.x) <= hs && std::abs(p.y - pc.y) <= hs; };

  // corners
  if (inBox({ c.x - s.x, c.y - s.y })) { m.l = m.t = true; return m; }
  if (inBox({ c.x + s.x, c.y - s.y })) { m.r = m.t = true; return m; }
  if (inBox({ c.x - s.x, c.y + s.y })) { m.l = m.b = true; return m; }
  if (inBox({ c.x + s.x, c.y + s.y })) { m.r = m.b = true; return m; }

  // edges
  if (inBox({ c.x, c.y - s.y })) { m.t = true; return m; }
  if (inBox({ c.x, c.y + s.y })) { m.b = true; return m; }
  if (inBox({ c.x - s.x, c.y })) { m.l = true; return m; }
  if (inBox({ c.x + s.x, c.y })) { m.r = true; return m; }

  return m;
}

s32 RectPainterState::pickTopmostRect(const glm::vec2& p) const
{
  for (s32 i = (s32)rects_.size() - 1; i >= 0; --i) { // topmost = last created
    glm::vec2 minP, maxP;
    if (getBounds(rects_[i], minP, maxP) && pointInRect(p, minP, maxP)) return i;
  }
  return -1;
}

/* ImGui */
void RectPainterState::drawImGui()
{
  if (ImGui::Begin("Rect Painter (ECS)", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
    ImGui::TextUnformatted("Click rect to select. Drag inside to move. Drag handles to resize.");
    ImGui::Separator();

    ImGui::ColorEdit4("Default Color", &currentColor_.x, ImGuiColorEditFlags_AlphaBar);
    ImGui::Checkbox("Global Snap (G)", &globalSnap_);
    ImGui::SameLine();
    ImGui::Text("Grid: %.0f px", snapGridPx_);

    if (ImGui::Button("Clear All")) {
      clearAllRects();
      /*
      for (EntityID id : rects_) ECS::World::destroy_entity(id);
      rects_.clear();
      selectedIndex_ = -1;
      dragMode_ = DragMode::None;
      */
    }

    ImGui::SameLine();
    const bool canDelete = (selectedIndex_ >= 0 && selectedIndex_ < (int)rects_.size());
    if (ImGui::Button("Delete Selected") && canDelete) {
      ECS::World::destroy_entity(rects_[selectedIndex_]);
      rects_.erase(rects_.begin() + selectedIndex_);
      selectedIndex_ = -1;
    }

    ImGui::SameLine();
    if (ImGui::Button("Duplicate") && canDelete) {
      const EntityID src = rects_[selectedIndex_];
      auto* translate = ECS::World::get_component<Components::Transform2D>(src);
      Math::Vec2D* p = &translate->position;
      Math::Vec2D* s = &translate->scale;
      auto* c = ECS::World::get_component<Components::Color>(src);
      auto* col = ECS::World::get_component<Components::BasicCollider>(src);
      if (p && s && c) {
        auto e = ECS::World::create_entity()
          .add_component<Components::Tag<"RectPainter">>()
          .add_component<Components::Transform2D>(p->x + 12.f, p->y + 12.f, 0.0f, s->x, s->y)
          .add_component<Components::Color>(c->r(), c->g(), c->b(), c->a());
        if (col) e.add_component<Components::BasicCollider>(col->enabled, col->isTrigger);
        rects_.push_back(e.get_id());
        selectedIndex_ = (int)rects_.size() - 1;
      }
    }

    if (ImGui::BeginListBox("Rectangles", ImVec2(420, 200))) {
      for (s32 i = 0; i < (s32)rects_.size(); ++i) {
        const EntityID id = rects_[i];

        auto* transform = ECS::World::get_component<Components::Transform2D>(id);

        if (!transform) continue;
        char label[160];
        std::snprintf(label, sizeof(label), "#%d  pos(%.0f,%.0f) size(%.0f,%.0f)",
          i, transform->position.x, transform->position.y, transform->scale.x, transform->scale.y);
        bool selected = (selectedIndex_ == i);
        if (ImGui::Selectable(label, selected)) selectedIndex_ = i;
      }
      ImGui::EndListBox();
    }

    if (selectedIndex_ >= 0 && selectedIndex_ < (int)rects_.size()) {
      ImGui::Separator();
      ImGui::Text("Selected #%d", selectedIndex_);
      const EntityID id = rects_[selectedIndex_];
      auto* transform = ECS::World::get_component<Components::Transform2D>(id);
      auto* color = ECS::World::get_component<Components::Color>(id);
      //auto* col = ECS::World::get_component<Components::BasicCollider>(id);

      if (transform && color) {
        ImGui::DragFloat2("Position (center)", &transform->position.x, 1.0f);
        ImGui::DragFloat2("Size", &transform->scale.x, 1.0f, minRectSizePx_, 10000.0f);
        ImGui::ColorEdit4("Color", &color->r(), ImGuiColorEditFlags_AlphaBar);

        // Keep size invariant even if user drags below min
        glm::vec2 s{ transform->scale.x, transform->scale.y };
        clampMinSize(s, minRectSizePx_);
        transform->scale.x = s.x;
        transform->scale.y = s.y;

        // BasicCollider controls
        //bool hasBasicCollider = (col != nullptr);
       /* if (!hasBasicCollider) {
          if (ImGui::Button("Add BasicCollider")) {
            ECS::World::add_component<Components::BasicCollider>(id, Components::BasicCollider{ true,false });
            col = ECS::World::get_component<Components::BasicCollider>(id);
          }
        }
        else {
          ImGui::Checkbox("Is Trigger", &col->isTrigger);
          if (ImGui::Button("Remove BasicCollider")) {
            col->isTrigger = false;
          }
        }*/

        ImGui::SameLine();
        if (ImGui::Button("Apply Default Color")) {
          color->r() = currentColor_.x;
          color->g() = currentColor_.y;
          color->b() = currentColor_.z;
          color->a() = currentColor_.w;
        }

        if (ImGui::Button("Send to Back [")) {
          sendToBack(selectedIndex_);
        }
        ImGui::SameLine();
        if (ImGui::Button("Bring to Front ]")) {
          bringToFront(selectedIndex_);
        }

      }
    }

  }
  ImGui::End();
}

/* Helpers */
void RectPainterState::computeCenterAndSize(const glm::vec2& a, const glm::vec2& b,
  glm::vec2& outCenter, glm::vec2& outSize)
{
  const f32 minX = std::min(a.x, b.x);
  const f32 maxX = std::max(a.x, b.x);
  const f32 minY = std::min(a.y, b.y);
  const f32 maxY = std::max(a.y, b.y);
  outSize = { std::max(0.0f, maxX - minX), std::max(0.0f, maxY - minY) };
  outCenter = { minX + outSize.x * 0.5f, minY + outSize.y * 0.5f };
}

void RectPainterState::clampMinSize(glm::vec2& size, f32 minSize)
{
  size.x = std::max(size.x, minSize);
  size.y = std::max(size.y, minSize);
}

glm::vec2 RectPainterState::snapToGrid(glm::vec2 v, f32 grid)
{
  if (grid <= 0.0f) return v;
  return { std::round(v.x / grid) * grid, std::round(v.y / grid) * grid };
}

f32 RectPainterState::snapScalar(f32 v, f32 grid)
{
  if (grid <= 0.0f) return v;
  return std::round(v / grid) * grid;
}

void RectPainterState::bringToFront(s32 index)
{
  if (index < 0 || index >= (s32)rects_.size()) return;
  auto id = rects_[index];
  rects_.erase(rects_.begin() + index);
  rects_.push_back(id);
  selectedIndex_ = (s32)rects_.size() - 1;
}

void RectPainterState::sendToBack(s32 index)
{
  if (index < 0 || index >= (s32)rects_.size()) return;
  auto id = rects_[index];
  rects_.erase(rects_.begin() + index);
  rects_.insert(rects_.begin(), id);
  selectedIndex_ = 0;
}

void RectPainterState::clearAllRects()
{
  for (EntityID id : rects_) ECS::World::destroy_entity(id);
  rects_.clear();
  selectedIndex_ = -1;
  dragMode_ = DragMode::None;
}

bool RectPainterState::aabbOverlap(const glm::vec2& aMin, const glm::vec2& aMax,
  const glm::vec2& bMin, const glm::vec2& bMax)
{
  return !(aMax.x < bMin.x || bMax.x < aMin.x || aMax.y < bMin.y || bMax.y < aMin.y);
}

/* Serialization stubs */
bool RectPainterState::SaveToFile(const char* /*path*/) const { return false; }
bool RectPainterState::LoadFromFile(const char* /*path*/) { return false; }