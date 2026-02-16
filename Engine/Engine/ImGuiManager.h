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
#include "Math/Vector2D.h"

#include "Events/EngineEvents.h"

namespace ImGuiManager {

	using ToastType = Engine::Events::ToastType;

	void init();
	void exit();

	void begin();
	void end();

	void update();

	// Register a panel function to be called every frame
	void register_panel(const std::string& panel_name, const std::function<void()>& panel_func);

	// Clear all registered panels
	void clear_panels();

	void set_style(const std::string& style_name);

	// Show a toast notification
	void show_toast(const std::string& message, ToastType type = ToastType::Info, float duration = 3.0f);

	// Check if viewport mode is enabled
	bool is_viewport_mode_enabled();

	// Get the top-left position of the viewport area (either panel or main window)
	Math::Vec2D get_viewport_area_min();

	// Get the top-left position of the central dock node (where the game is rendered when not in viewport mode)
	Math::Vec2D get_game_render_area_min();

	// Check if game input should be blocked (an ImGui panel other than Viewport has focus)
	bool is_game_input_blocked();
}

