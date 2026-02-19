//-----------------------------------------------------------------------------------
// file:    RegisterAllPanels.h
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   Central registration point for all ImGui panels in the engine.
//          Includes headers for all panel implementations and provides the
//          register_all_panels() function to initialize the editor interface
//          with all available panels (viewport, performance, resources, etc.).
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#include "ImGuiManager.h"

#include "PerformanceViewerPanel.h"
#include "ResourceManagerPanel.h"
#include "ResourceExplorerPanel.h"
#include "EntityEditorPanel.h"
#include "PrefabEditorPanel.h" 
#include "CameraPanel.h"
#include "TilemapPanel.h"
#include "TileColliderEditorPanel.h"
#include "ViewportPanel.h"
#include "SceneChooserPanel.h"
#include "LayerManagerPanel.h"
#include "ColliderEditorPanel.h"
#include "SelectiveLevelSavePanel.h"
#include "FrameAnimationEditorPanel.h"
#include "TextureAtlasEditorPanel.h"
#include "ConfigurationPanel.h"
#include "LogConsolePanel.h"

void register_all_panels()
{
  ImGuiManager::register_panel("Viewport", ViewportPanel::draw_panel);
  ImGuiManager::register_panel("Performance", PerformanceViewerPanel);
  //ImGuiManager::register_panel("Resource", ResourceManagerPanel);
  ImGuiManager::register_panel("Resource Explorer", ResourceExplorerPanel);
  ImGuiManager::register_panel("Entity", EntityEditorPanel);
  ImGuiManager::register_panel("Prefab", PrefabEditorPanel);
  ImGuiManager::register_panel("Camera", CameraPanel);
  ImGuiManager::register_panel("Tilemap", TilemapPanel::draw_panel);
  ImGuiManager::register_panel("Tile Collider Editor", TileColliderEditorPanel);
  ImGuiManager::register_panel("Scene Chooser", SceneChooserPanel);
  ImGuiManager::register_panel("Layer Manager", LayerManagerPanel);
  ImGuiManager::register_panel("Collider Editor", ColliderEditorPanel);
  ImGuiManager::register_panel("Selective Level Save", SelectiveLevelSavePanel);
  ImGuiManager::register_panel("Frame Animation Editor", FrameAnimationEditorPanel);
  ImGuiManager::register_panel("Texture Atlas Editor", TextureAtlasEditorPanel);
  ImGuiManager::register_panel("Configuration", ConfigurationPanel);
  ImGuiManager::register_panel("Log Console", LogConsolePanel);
}