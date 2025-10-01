#include "ImGuiState.h"
#include "core.h"
#include "InputManager.h"
#include "BasicComponents.h"
#include "BackgroundSystem.h"
#include "RenderSystem.h"

#include <imgui.h>
#include <GLFW/glfw3.h>
#include <algorithm>
#include <cstdio>
#include <unordered_map>

using ECS::EntityID;

namespace Components {
  struct Collider {
    bool enabled = true;
    bool isTrigger = false;
  };
}

void ImGuiState::Load() {}
void ImGuiState::Init()
{
  Engine::WORLD.create_entity()
    .add_component<Components::Tag<"RectPainter">>()
    .add_component<Components::Background>()
    .add_component<Components::Color>(0.08f, 0.08f, 0.10f, 1.0f)
    .add_component<Components::Collider>(false, false);

  Engine::WORLD.addSystem<BackgroundSystem>();
  Engine::WORLD.addSystem<RectRenderSystem>();

  rects_.clear();
  selectedIndex_ = -1;
  dragMode_ = DragMode::None;
  passedThreshold_ = false;

}

void ImGuiState::Free()
{
  // Destroy all entities from this state
  Engine::WORLD.destory_entities_with<Components::Tag<"RectPainter">>();
  rects_.clear();
  selectedIndex_ = -1;

  // Remove systems added in Init
  Engine::WORLD.removeSystem<BackgroundSystem>();
  Engine::WORLD.removeSystem<RectRenderSystem>();
}

void ImGuiState::Unload() {}

void ImGuiState::Update()
{
  handleMouse();
}

void ImGuiState::Draw()
{
  // Preview while drawing
  if (dragMode_ == DragMode::Drawing && passedThreshold_) {
    glm::vec2 center, size;
    computeCenterAndSize(dragStart_, prevMouse_, center, size);
    clampMinSize(size, minRectSizePx_);
    Engine::GRAPHICS.draw_rect(currentColor_, center.x, center.y, size.x, size.y, 0.0f, Graphics::Origin::Center);
  }

  runCollisionPass();
  drawSelectionGizmo();
  drawImGui();
}

void ImGuiState::handleMouse()
{
  ImGuiIO& io = ImGui::GetIO();
  if (io.WantCaptureMouse) {
    prevMouse_ = Engine::Input::GetMousePosition();
    return;
  }

  const glm::vec2 mouse = Engine::Input::GetMousePosition();
  const bool lDown = Engine::Input::IsMouseButtonPressed(GLFW_MOUSE_BUTTON_LEFT);
  const bool lHeld = Engine::Input::IsMouseButtonHeld(GLFW_MOUSE_BUTTON_LEFT);
  const bool lUp = Engine::Input::IsMouseButtonReleased(GLFW_MOUSE_BUTTON_LEFT);

  // Esc clears selection
  if (Engine::Input::IsKeyPressed(GLFW_KEY_ESCAPE)) { selectedIndex_ = -1; dragMode_ = DragMode::None; }

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
          auto* pos = Engine::WORLD.get_component<Components::Position>(rects_[selectedIndex_]);
          moveGrabOffset_ = mouse - glm::vec2{ pos->X(), pos->Y() };
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
        auto* pos = Engine::WORLD.get_component<Components::Position>(rects_[selectedIndex_]);
        moveGrabOffset_ = mouse - glm::vec2{ pos->X(), pos->Y() };
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
      if (auto* pos = Engine::WORLD.get_component<Components::Position>(id)) {
        glm::vec2 newCenter = mouse - moveGrabOffset_;
        pos->X() = newCenter.x;
        pos->Y() = newCenter.y;
      }
    }
    else if (dragMode_ == DragMode::Resizing && selectedIndex_ >= 0) {
      auto id = rects_[selectedIndex_];
      auto* pos = Engine::WORLD.get_component<Components::Position>(id);
      auto* size = Engine::WORLD.get_component<Components::Size>(id);
      if (pos && size) {
        glm::vec2 minP{ pos->X() - size->width * 0.5f,  pos->Y() - size->height * 0.5f };
        glm::vec2 maxP{ pos->X() + size->width * 0.5f,  pos->Y() + size->height * 0.5f };

        // Adjust sides
        if (resize_.l) minP.x = std::min(mouse.x, maxP.x - minRectSizePx_);
        if (resize_.r) maxP.x = std::max(mouse.x, minP.x + minRectSizePx_);
        if (resize_.t) minP.y = std::min(mouse.y, maxP.y - minRectSizePx_);
        if (resize_.b) maxP.y = std::max(mouse.y, minP.y + minRectSizePx_);

        // Recompute center/size
        pos->X() = (minP.x + maxP.x) * 0.5f;
        pos->Y() = (minP.y + maxP.y) * 0.5f;
        size->width = std::max(minRectSizePx_, maxP.x - minP.x);
        size->height = std::max(minRectSizePx_, maxP.y - minP.y);
      }
    }

    if (dragMode_ == DragMode::Drawing) {
      prevMouse_ = mouse;
    }
  }

  // Mouse up: finalize
  if (lUp) {
    if (dragMode_ == DragMode::Drawing && passedThreshold_) {
      glm::vec2 center, size;
      computeCenterAndSize(dragStart_, mouse, center, size);
      if (size.x >= minRectSizePx_ && size.y >= minRectSizePx_) {
        auto entity = Engine::WORLD.create_entity()
          .add_component<Components::Tag<"RectPainter">>()
          .add_component<Components::Position>(center.x, center.y)
          .add_component<Components::Size>(size.x, size.y)
          .add_component<Components::Color>(currentColor_.x, currentColor_.y, currentColor_.z, currentColor_.w)
          .add_component<Components::Collider>(true, false);
        rects_.push_back(entity.getId());
        selectedIndex_ = (int)rects_.size() - 1;
      }
    }
    dragMode_ = DragMode::None;
    passedThreshold_ = false;
  }

  prevMouse_ = mouse;
}

void ImGuiState::runCollisionPass()
{
  // transient per-frame collision flags
  static std::unordered_map<EntityID, HitInfo> hits;
  hits.clear();

  // Brute-force AABB among collider-enabled rects
  for (size_t i = 0; i < rects_.size(); ++i) {
    auto idA = rects_[i];
    auto* colA = Engine::WORLD.get_component<Components::Collider>(idA);
    if (!colA || !colA->enabled) continue;

    glm::vec2 aMin, aMax;
    if (!getBounds(idA, aMin, aMax)) continue;

    for (size_t j = i + 1; j < rects_.size(); ++j) {
      auto idB = rects_[j];
      auto* colB = Engine::WORLD.get_component<Components::Collider>(idB);
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
    Engine::GRAPHICS.draw_rect(glm::vec4{ 1,0,0,0.20f }, c.x, c.y, s.x + 6.0f, s.y + 6.0f, 0.0f, Graphics::Origin::Center);
  }
}

/* Overlay */
void ImGuiState::drawSelectionGizmo()
{
  if (selectedIndex_ < 0 || selectedIndex_ >= (int)rects_.size()) return;

  glm::vec2 minP, maxP;
  if (!getBounds(rects_[selectedIndex_], minP, maxP)) return;

  // Slight highlight box
  glm::vec2 center{ (minP.x + maxP.x) * 0.5f, (minP.y + maxP.y) * 0.5f };
  glm::vec2 size{ std::max(1.0f, maxP.x - minP.x), std::max(1.0f, maxP.y - minP.y) };
  glm::vec4 sel{ 1.f, 1.f, 0.f, 0.15f };
  Engine::GRAPHICS.draw_rect(sel, center.x, center.y, size.x + 4.0f, size.y + 4.0f, 0.0f, Graphics::Origin::Center);

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
    Engine::GRAPHICS.draw_rect(glm::vec4{ 1,1,0,0.9f }, p.x, p.y, hs, hs, 0.0f, Graphics::Origin::Center);
  }
}

/* Picking helpers */
bool ImGuiState::getBounds(EntityID id, glm::vec2& minP, glm::vec2& maxP) const
{
  auto* pos = Engine::WORLD.get_component<Components::Position>(id);
  auto* size = Engine::WORLD.get_component<Components::Size>(id);
  if (!pos || !size) return false;
  minP = { pos->X() - size->width * 0.5f,  pos->Y() - size->height * 0.5f };
  maxP = { pos->X() + size->width * 0.5f,  pos->Y() + size->height * 0.5f };
  return true;
}

bool ImGuiState::pointInRect(const glm::vec2& p, const glm::vec2& minP, const glm::vec2& maxP)
{
  return (p.x >= minP.x && p.x <= maxP.x && p.y >= minP.y && p.y <= maxP.y);
}

ImGuiState::ResizeMask ImGuiState::hitHandle(const glm::vec2& p, const glm::vec2& minP, const glm::vec2& maxP) const
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

int ImGuiState::pickTopmostRect(const glm::vec2& p) const
{
  for (int i = (int)rects_.size() - 1; i >= 0; --i) { // topmost = last created
    glm::vec2 minP, maxP;
    if (getBounds(rects_[i], minP, maxP) && pointInRect(p, minP, maxP)) return i;
  }
  return -1;
}

/* ImGui */
void ImGuiState::drawImGui()
{
  if (ImGui::Begin("Rect Painter (ECS)", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
    ImGui::TextUnformatted("Click rect to select. Drag inside to move. Drag handles to resize.");
    ImGui::Separator();

    ImGui::ColorEdit4("Default Color", &currentColor_.x, ImGuiColorEditFlags_AlphaBar);

    if (ImGui::Button("Clear All")) {
      for (EntityID id : rects_) Engine::WORLD.destory_entity(id);
      rects_.clear();
      selectedIndex_ = -1;
      dragMode_ = DragMode::None;
    }

    ImGui::SameLine();
    const bool canDelete = (selectedIndex_ >= 0 && selectedIndex_ < (int)rects_.size());
    if (ImGui::Button("Delete Selected") && canDelete) {
      Engine::WORLD.destory_entity(rects_[selectedIndex_]);
      rects_.erase(rects_.begin() + selectedIndex_);
      selectedIndex_ = -1;
    }

    if (ImGui::BeginListBox("Rectangles", ImVec2(420, 200))) {
      for (int i = 0; i < (int)rects_.size(); ++i) {
        const EntityID id = rects_[i];
        auto* pos = Engine::WORLD.get_component<Components::Position>(id);
        auto* size = Engine::WORLD.get_component<Components::Size>(id);
        if (!pos || !size) continue;
        char label[160];
        std::snprintf(label, sizeof(label), "#%d  pos(%.0f,%.0f) size(%.0f,%.0f)",
          i, pos->X(), pos->Y(), size->width, size->height);
        bool selected = (selectedIndex_ == i);
        if (ImGui::Selectable(label, selected)) selectedIndex_ = i;
      }
      ImGui::EndListBox();
    }

    if (selectedIndex_ >= 0 && selectedIndex_ < (int)rects_.size()) {
      ImGui::Separator();
      ImGui::Text("Selected #%d", selectedIndex_);
      const EntityID id = rects_[selectedIndex_];
      auto* pos = Engine::WORLD.get_component<Components::Position>(id);
      auto* size = Engine::WORLD.get_component<Components::Size>(id);
      auto* color = Engine::WORLD.get_component<Components::Color>(id);
      auto* col = Engine::WORLD.get_component<Components::Collider>(id);

      if (pos && size && color) {
        ImGui::DragFloat2("Position (center)", &pos->X(), 1.0f);
        ImGui::DragFloat2("Size", &size->width, 1.0f, minRectSizePx_, 10000.0f);
        ImGui::ColorEdit4("Color", &color->X(), ImGuiColorEditFlags_AlphaBar);

        // Keep size invariant even if user drags below min
        glm::vec2 s{ size->width, size->height };
        clampMinSize(s, minRectSizePx_);
        size->width = s.x;
        size->height = s.y;

        // Collider controls
        bool hasCollider = (col != nullptr);
        if (!hasCollider) {
          if (ImGui::Button("Add Collider")) {
            Engine::WORLD.add_component<Components::Collider>(id, Components::Collider{ true,false });
            col = Engine::WORLD.get_component<Components::Collider>(id);
          }
        }
        else {
          ImGui::Checkbox("Collider Enabled", &col->enabled);
          ImGui::Checkbox("Is Trigger", &col->isTrigger);
          if (ImGui::Button("Remove Collider")) {
            col->enabled = false; col->isTrigger = false;
          }
        }

        ImGui::SameLine();
        if (ImGui::Button("Apply Default Color")) {
          color->X() = currentColor_.x;
          color->Y() = currentColor_.y;
          color->Z() = currentColor_.z;
          color->W() = currentColor_.w;
        }
      }
    }

  }
  ImGui::End();
}

/* Helpers */
void ImGuiState::computeCenterAndSize(const glm::vec2& a, const glm::vec2& b,
  glm::vec2& outCenter, glm::vec2& outSize)
{
  const f32 minX = std::min(a.x, b.x);
  const f32 maxX = std::max(a.x, b.x);
  const f32 minY = std::min(a.y, b.y);
  const f32 maxY = std::max(a.y, b.y);
  outSize = { std::max(0.0f, maxX - minX), std::max(0.0f, maxY - minY) };
  outCenter = { minX + outSize.x * 0.5f, minY + outSize.y * 0.5f };
}

void ImGuiState::clampMinSize(glm::vec2& size, f32 minSize)
{
  size.x = std::max(size.x, minSize);
  size.y = std::max(size.y, minSize);
}

bool ImGuiState::aabbOverlap(const glm::vec2& aMin, const glm::vec2& aMax,
  const glm::vec2& bMin, const glm::vec2& bMax)
{
  return !(aMax.x < bMin.x || bMax.x < aMin.x || aMax.y < bMin.y || bMax.y < aMin.y);
}

/* Serialization stubs */
bool ImGuiState::SaveToFile(const char* /*path*/) const { return false; }
bool ImGuiState::LoadFromFile(const char* /*path*/) { return false; }