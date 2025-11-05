#include "ImGuiManager.h"

#include "PerformanceViewerPanel.h"
#include "ResourceManagerPanel.h"
#include "EntityEditorPanel.h"
#include "PrefabEditorPanel.h"
#include "LevelPanel.h"
#include "CameraPanel.h"
#include "TilemapPanel.h"
#include "ViewportPanel.h"
#include "SceneChooserPanel.h"

void register_all_panels()
{
  ImGuiManager::register_panel("Viewport", ViewportPanel::draw_panel);
  ImGuiManager::register_panel("Performance", PerformanceViewerPanel);
  ImGuiManager::register_panel("Resource", ResourceManagerPanel);
  ImGuiManager::register_panel("Entity", EntityEditorPanel);
  ImGuiManager::register_panel("Prefab", PrefabEditorPanel);
  ImGuiManager::register_panel("Level", LevelPanel);
  ImGuiManager::register_panel("Camera", CameraPanel);
  ImGuiManager::register_panel("Tilemap", TilemapPanel::draw_panel);
  ImGuiManager::register_panel("Scene Chooser", SceneChooserPanel);
}