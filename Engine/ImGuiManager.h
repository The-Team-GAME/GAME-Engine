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
#include <vector>

namespace ImguiMgr {
	void imgui_init();
	void imgui_exit();

	void begin();
	void end();

	void update();
}

