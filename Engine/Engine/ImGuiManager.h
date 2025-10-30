//-----------------------------------------------------------------------------------
// file:    ImGuiManager.h
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   This file defines the ImGuiManager which handles ImGui initialization,
//          frame management, and rendering. Provides debug UI capabilities with
//          toggle support (F1 key).
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include <functional>
#include <string>

namespace ImGuiManager {


	void init();
	void exit();

	void begin();
	void end();

	void update();

	// Register a panel function to be called every frame
	void register_panel(const std::function<void()>& panel_func);

	// Clear all registered panels
	void clear_panels();

	void set_style(const std::string& style_name);
}

