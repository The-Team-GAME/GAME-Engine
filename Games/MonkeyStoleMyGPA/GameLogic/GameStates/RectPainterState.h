//-----------------------------------------------------------------------------------
// file:    RectPainterState.h
// project: Monkey Stole My GPA
//
// author:  Ong Zi Ming (100%)
//          Primary author
//
// email:   o.ziming@digipen.edu
//
// brief:   Header file for the ImGuistate that handles object creation and deletion.
//
// Copyright � 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include "pch.h"
#include <vector>
#include <glm/glm.hpp>

class RectPainterState : public Engine::Scene
{
public:
  void load();
  void init();
  void update();
  void draw();
  void free();
  void unload();

private:

  enum class DragMode { None, Drawing, Moving, Resizing };


  // Interaction state
  DragMode dragMode_ = DragMode::None;
  bool passedThreshold_ = false;
  bool globalSnap_ = false;
  glm::vec2 dragStart_{ 0.0f };     // for drawing/resizing
  glm::vec2 prevMouse_{ 0.0f };
  glm::vec2 moveGrabOffset_{ 0.0f };

  struct ResizeMask { bool l = false, r = false, t = false, b = false; } resize_;

  // Thresholds
  f32 startDragThresholdPx_ = 6.0f;
  f32 minRectSizePx_ = 8.0f;
  f32 handlePx_ = 8.0f;
  f32 handleHoverInflate_ = 4.0f;
  f32 snapGridPx_ = 14.0f;

  // UI
  glm::vec4 currentColor_{ 1.0f, 1.0f, 1.0f, 0.25f };
  s32 selectedIndex_ = -1;
  bool spawnWithCollider_ = false;

  // ECS rectangles
  std::vector<Engine::ECS::EntityID> rects_{};

  // Core
  void handleMouse();
  void drawImGui();
  void drawSelectionGizmo();
  void runCollisionPass();

  // Picking / geometry
  static void computeCenterAndSize(const glm::vec2& a, const glm::vec2& b,
    glm::vec2& outCenter, glm::vec2& outSize);
  static void clampMinSize(glm::vec2& size, f32 minSize = 1.0f);

  bool getBounds(Engine::ECS::EntityID id, glm::vec2& minP, glm::vec2& maxP) const;
  static bool pointInRect(const glm::vec2& p, const glm::vec2& minP, const glm::vec2& maxP);

  static bool aabbOverlap(const glm::vec2& aMin, const glm::vec2& aMax,
    const glm::vec2& bMin, const glm::vec2& bMax);
  struct HitInfo { bool colliding = false; };

  ResizeMask hitHandle(const glm::vec2& p, const glm::vec2& minP, const glm::vec2& maxP) const;
  s32 pickTopmostRect(const glm::vec2& p) const;

  // keyboard actions
  void handleShortcuts();

  // Snapping
  static glm::vec2 snapToGrid(glm::vec2 v, f32 grid);
  static f32 snapScalar(f32 v, f32 grid);

  // Serialization stubs
  bool SaveToFile(const char* path) const;
  bool LoadFromFile(const char* path);

  // Helpers for persistence
  void clearAllRects();
  void bringToFront(s32 index);
  void sendToBack(s32 index);
};