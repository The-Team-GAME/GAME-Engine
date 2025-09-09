#pragma once

#include "StateManager.h"
#include <vector>
#include <glm/glm.hpp>

class ImGuiState : public Engine::GameState<ImGuiState>
{
public:
  void Load();
  void Init();
  void Update();
  void Draw();
  void Free();
  void Unload();

private:
  struct Rect
  {
    glm::vec2 pos{ 0.0f };   // top-left
    glm::vec2 size{ 0.0f };  // width/height (>= 0)
    glm::vec4 color{ 1.0f };
  };

  std::vector<Rect> rects_{};

  // Drag state
  bool isDrawing_ = false;
  bool prevLeftHeld_ = false;
  glm::vec2 dragStart_ = { 0.0f, 0.0f };
  glm::vec2 dragEnd_ = { 0.0f, 0.0f };

  // UI state
  glm::vec4 currentColor_{ 1.0f, 1.0f, 1.0f, 0.25f };
  int selectedIndex_ = -1;

  // Helpers
  static Rect MakeRect(const glm::vec2& a, const glm::vec2& b, const glm::vec4& color);
  static void ClampRectMinSize(Rect& r, float minSize = 1.0f);

  void handleMouse();
  void drawImGui();

  // Serialization stubs (intentionally unimplemented for now)
  bool SaveToFile(const char* path) const;
  bool LoadFromFile(const char* path);
};