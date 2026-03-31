//-----------------------------------------------------------------------------------
// file:    BuildSizeAnalyzerPanel.h
// project: Monkey Stole My GPA
//
// brief:   Declares the Build Size Analyzer ImGui panel. This panel lets users
//          review project assets, auto-detect level dependencies, select assets
//          for export, and monitor + export the total selected build size.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

/*=================================================================================
Function Brief:
Creates and renders the Build Size Analyzer ImGui panel. Scans the Assets folder,
displays all files with checkboxes and sizes, supports level-based auto-detection
of referenced assets, and allows exporting selected assets to a destination folder.

Where to call it:
Register with ImGuiManager::register_panel() in RegisterAllPanels.h.
==================================================================================*/
void BuildSizeAnalyzerPanel();
