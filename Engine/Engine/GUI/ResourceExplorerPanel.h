//-----------------------------------------------------------------------------------
// file:    ResourceExplorerPanel.h
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   This file defines the ResourceExplorerPanel ImGui interface for browsing
//          and managing files in the resource folder. Provides a file explorer
//          view with folder navigation, file previews, and resource management.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

/*=================================================================================
Function Brief:
Renders a file explorer panel for the resource folder, allowing users to browse
files and folders, view previews of resources (textures, etc.), and load resources
into the cache.

Features:
- Folder navigation with breadcrumb trail
- File and folder list with icons
- Preview for image files
- Load resources on double-click
- Create new folders
- Delete files/folders

Where to call it:
Called during the ImGui rendering phase, typically registered with ImGuiManager.
==================================================================================*/
void ResourceExplorerPanel();
