#include "ImGuiState.h"
#include "core.h"
#include "InputManager.h"

#include <imgui.h>
#include <GLFW/glfw3.h>
#include <algorithm>
#include <cstdio>
#include <cmath>

void ImGuiState::Load() {}
void ImGuiState::Init() {}
void ImGuiState::Free() {}
void ImGuiState::Unload() {}

void ImGuiState::Update()
{
  handleMouse();
}

void ImGuiState::Draw()
{
  // Background
  Engine::GRAPHICS.clearScreen(0.08f, 0.08f, 0.10f, 1.0f);

  // Draw committed rects
  for (const Rect& r : rects_)
    Engine::GRAPHICS.drawRect(r.color, r.pos.x, r.pos.y, r.size.x, r.size.y);

  // Draw preview while dragging
  if (isDrawing_ && passedThreshold_)
  {
    Rect preview = MakeRect(dragStart_, dragEnd_, currentColor_);
    ClampRectMinSize(preview);
    Engine::GRAPHICS.drawRect(preview.color, preview.pos.x, preview.pos.y, preview.size.x, preview.size.y);
  }

  drawImGui();
}

void ImGuiState::handleMouse()
{
  ImGuiIO& io = ImGui::GetIO();
  if (io.WantCaptureMouse)
  {
    prevLeftHeld_ = false;
    isDrawing_ = false;
    passedThreshold_ = false;
    return;
  }

  const glm::vec2 mousePos = Engine::Input::GetMousePosition();
  const bool leftHeld = Engine::Input::IsMouseButtonHeld(GLFW_MOUSE_BUTTON_LEFT);

  // Begin drag
  if (!prevLeftHeld_ && leftHeld && !isDrawing_)
  {
    isDrawing_ = true;
    passedThreshold_ = false;
    dragStart_ = dragEnd_ = mousePos;
  }

  // Update drag
  if (isDrawing_ && leftHeld)
  {
    dragEnd_ = mousePos;
    if (!passedThreshold_)
    {
      const float moved = glm::length(dragEnd_ - dragStart_);
      if (moved >= startDragThresholdPx_)
        passedThreshold_ = true;
    }
  }

  // End drag -> commit
  if (isDrawing_ && prevLeftHeld_ && !leftHeld)
  {
    if (passedThreshold_)
    {
      Rect r = MakeRect(dragStart_, dragEnd_, currentColor_);
      if (r.size.x >= minRectSizePx_ && r.size.y >= minRectSizePx_)
        rects_.push_back(r);
    }
    isDrawing_ = false;
    passedThreshold_ = false;
  }

  prevLeftHeld_ = leftHeld;
}


/* ImGui */
void ImGuiState::drawImGui()
{
  if (ImGui::Begin("Rect Painter", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
  {
    ImGui::TextUnformatted("Drag LMB on empty space to create rectangles.");
    ImGui::TextUnformatted("Mouse is ignored when hovering an ImGui window.");
    ImGui::Separator();

    ImGui::ColorEdit4("Default Color", &currentColor_.x, ImGuiColorEditFlags_AlphaBar);

    if (ImGui::Button("Clear All"))
    {
      rects_.clear();
      selectedIndex_ = -1;
    }

    ImGui::SameLine();
    const bool canDelete = (selectedIndex_ >= 0 && selectedIndex_ < (int)rects_.size());
    if (ImGui::Button("Delete Selected") && canDelete)
    {
      rects_.erase(rects_.begin() + selectedIndex_);
      selectedIndex_ = -1;
    }

    ImGui::Separator();

    if (ImGui::BeginListBox("Rectangles", ImVec2(420, 200)))
    {
      for (int i = 0; i < (int)rects_.size(); ++i)
      {
        const Rect& r = rects_[i];
        char label[128];
        std::snprintf(label, sizeof(label), "#%d  pos(%.0f,%.0f) size(%.0f,%.0f)", i, r.pos.x, r.pos.y, r.size.x, r.size.y);
        bool selected = (selectedIndex_ == i);
        if (ImGui::Selectable(label, selected))
          selectedIndex_ = i;
      }
      ImGui::EndListBox();
    }

    if (selectedIndex_ >= 0 && selectedIndex_ < (int)rects_.size())
    {
      Rect& r = rects_[selectedIndex_];
      ImGui::Separator();
      ImGui::Text("Selected #%d", selectedIndex_);
      ImGui::DragFloat2("Position", &r.pos.x, 1.0f);
      ImGui::DragFloat2("Size", &r.size.x, 1.0f, 1.0f, 10000.0f);
      ImGui::ColorEdit4("Color", &r.color.x, ImGuiColorEditFlags_AlphaBar);
      ClampRectMinSize(r);
    }

    ImGui::Separator();
    static char pathBuf[260] = "rects.json";
    ImGui::InputText("File", pathBuf, IM_ARRAYSIZE(pathBuf));

    ImGui::BeginDisabled(true); // enable when serialization implemented
    if (ImGui::Button("Save")) { (void)SaveToFile(pathBuf); }
    ImGui::SameLine();
    if (ImGui::Button("Load")) { (void)LoadFromFile(pathBuf); }
    ImGui::EndDisabled();
  }
  ImGui::End();

  if (ImGui::Begin("Help##RectPainter", nullptr,
    ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings))
  {
    ImGui::Text("Rects: %d", (int)rects_.size());
    if (isDrawing_)
    {
      Rect r = MakeRect(dragStart_, dragEnd_, currentColor_);
      ImGui::Text("Drawing: %.0fx%.0f", r.size.x, r.size.y);
    }
  }
  ImGui::End();
}


/* Helpers */
ImGuiState::Rect ImGuiState::MakeRect(const glm::vec2& a, const glm::vec2& b, const glm::vec4& color)
{
  Rect r{};
  r.pos.x = std::min(a.x, b.x);
  r.pos.y = std::min(a.y, b.y);
  r.size.x = std::abs(b.x - a.x);
  r.size.y = std::abs(b.y - a.y);
  r.color = color;
  return r;
}

void ImGuiState::ClampRectMinSize(Rect& r, float minSize)
{
  r.size.x = std::max(r.size.x, minSize);
  r.size.y = std::max(r.size.y, minSize);
}


/* Serialization stubs */
bool ImGuiState::SaveToFile(const char* /*path*/) const { return false; }
bool ImGuiState::LoadFromFile(const char* /*path*/) { return false; }
