#include "ImGuiManager.h"

#include "PerformanceViewerPanel.h"
#include "ResourceManagerPanel.h"
#include "EntityEditorPanel.h"
#include "CameraPanel.h"
#include "TilemapPanel.h"

void register_all_panels()
{
  // Initialize TilemapPanel to load saved tilemaps from project settings
  TilemapPanel::init();

  ImGuiManager::register_panel(PerformanceViewerPanel);
  ImGuiManager::register_panel(ResourceManagerPanel);
  ImGuiManager::register_panel(EntityEditorPanel);
  ImGuiManager::register_panel(CameraPanel);
  ImGuiManager::register_panel(TilemapPanel::draw_panel);
}