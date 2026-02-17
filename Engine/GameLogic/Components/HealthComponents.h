//-----------------------------------------------------------------------------------
// file:    HealthComponents.h
// project: Monkey Stole My GPA
//
// author:  Ong Zi Ming (100%)
//          Primary author
//
// email:   o.ziming@digipen.edu
//
// brief:   This file defines the ECS component and config structs used by the health 
//					and HUD systems. Includes core Health/HealthConfig data, player HUD
//          layout (HealthBar, HeartHUDSprite), and enemy HP bar settings
//          (EnemyHealthBarRect) for runtime rendering and behaviour.
//
// Copyright � 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------

#pragma once

#include "pch.h"
#include <algorithm>

namespace Components {

	
	// Timed HP regen/drain settings
		
	struct HealthConfig {
		f32 drain_per_sec = 0.0f;
		f32 heal_while_moving_per_sec = 0.0f;
		f32 carry = 0.0f;              // fractional HP accumulator
	};

	struct Health {
		int value = 100;
		int max = 100;

		void damage(int amt) { 
			value = std::max(0, value - amt); 
		}
		void heal(int amt) { 
			value = std::min(max, value + amt); 
		}
		bool is_dead() const {
			return value <= 0;
		}
	};
		
	// HUD hookups + layout
		
	struct HealthBar {
		Engine::ECS::Entity bar_bg{};
		Engine::ECS::Entity bar_fill{};
		Engine::ECS::Entity text{};
		f32 left = 0.f, top = 0.f, width = 0.f, height = 0.f;
	};

	struct HeartHUDSprite {
		Math::Vec2D anchor_px{ 20.f, 18.f };

		// BarBack canvas (not the head)
		float width = 763.f, height = 145.f;

		// Layers
		std::string layer = "player";
		std::string overlay_layer = "ui";
		std::string head_layer = "ui";
		bool  screen_space = true;

		// Per-part offsets
		Math::Vec2D head_offset_px{ 0.f, 20.f };
		Math::Vec2D back_offset_px{ 90.f, 0.f };

		Math::Vec2D overlay_offset_px{ 5.f, 0.f };
		bool overlay_relative_to_back = true;

		// Textures
		Engine::Graphics::Texture	tex_back{Engine::Resources::load_texture("images/Heartratebar/BarBack.png")};
		Engine::Graphics::Texture tex_solid{Engine::Resources::load_texture("images/Heartratebar/WhiteSolidForFill.png")};
		Engine::Graphics::Texture tex_white{Engine::Resources::load_texture("images/Heartratebar/WhiteBar.png")};
		Engine::Graphics::Texture tex_head{Engine::Resources::load_texture("images/Heartratebar/HeadBar.png")};

		// BPM mapping
		float bpm_min = 40.f, bpm_max = 200.f;

		// Source PNG size for WhiteSolidForFill.png
		int   src_w_px = 763;
		int   src_h_px = 145;

		// crop the SOURCE horizontally so columns match the on-screen band
		int   src_left_px = 3;                 // where the white band starts in PNG
		int   src_right_px = 743;                // where the white band ends    in PNG

		// Number of columns to reveal (default = cropped width)
		int   slices = src_right_px - src_left_px;

		// Fill band inside BarBack (measured from BarBack�s LEFT)
		float fill_left_px = 4.f;
		float fill_right_px = 742.f;

		// Inset so outline stays visible at full
		Math::Vec2D fill_inset_px{ 8.f, 10.f };

		bool use_white_overlay = true;

		Math::Vec4D fill_color_low{ 1.00f, 0.22f, 0.55f, 1.0f }; // pink (left/low BPM)
		Math::Vec4D fill_color_high{ 1.00f, 0.12f, 0.38f, 1.0f }; // deeper magenta (right/high)
		bool        use_bpm_tint = true;

		// Runtime
		Engine::ECS::Entity back, head, white_overlay;
		std::vector<Engine::ECS::Entity> whiteSlices;
		bool initialized = false;
	};

	/*
	struct EnemyHealthBar {
		// Layout
		float radius = 30.0f;
		float start_angle_deg = 160.0f;   // arc left -> right above head
		float end_angle_deg = 20.0f;
		int   segments = 36;
		Math::Vec2D offset{ 0.f, -20.f };   // above head

		// Rendering/layering
		std::string layer = "enemy";

		bool  use_center_span = true;
		float center_angle_deg = 90.0f;
		float arc_span_deg = 100.0f;

		// Visuals
		Engine::Graphics::Texture tex{};
		struct RGBA { float r = 1, g = 1, b = 1, a = 1; };
		RGBA base_color{ 0.08f, 0.08f, 0.10f, 1.00f }; // dark remainder
		RGBA front_color{ 0.96f, 0.34f, 0.52f, 1.00f }; // pink
		RGBA after_color{ 0.95f, 0.76f, 0.25f, 0.95f }; // delayed
		RGBA cap_color{ 0.95f, 0.85f, 0.35f, 1.00f }; // gold

		float base_thickness = 8.5f;
		float after_thickness = 9.0f;
		float front_thickness = 7.0f;

		// End-caps (short perpendicular dashes)
		float cap_len_px = 5.0f;
		float cap_thickness_px = 1.0f;
		float cap_slant_deg = 14.0f;

		// Behavior
		float drain_speed = 1.5f;
		float show_duration = 1.25f;
		bool  persist_after_first_hit = true;
		bool  has_been_hit = false;

		// Runtime
		std::vector<Engine::ECS::Entity> base;   // dark full arc
		std::vector<Engine::ECS::Entity> front;  // instant pink
		std::vector<Engine::ECS::Entity> after;  // delayed
		Engine::ECS::Entity cap_left{};
		Engine::ECS::Entity cap_right{};

		int   last_hp = -1, last_max = -1;
		float front_ratio = 1.0f;
		float after_ratio = 1.0f;
		float visible_until = 0.f;
		bool  initialized = false;
	};
	*/

	struct EnemyHealthBarRect {
		// Placement
		Math::Vec2D offset{ 0.f, -48.f };   // above head
		std::string layer = "enemy";        // draw with enemy layer by default

		// Geometry (outer frame)
		float width = 46.f;
		float height = 10.f;
		float corner = 0.f;                  // use textures if you want rounded

		// Colors
		struct RGBA { float r = 1, g = 1, b = 1, a = 1; };
		RGBA base_color{ 0.08f, 0.08f, 0.10f, 1.00f }; // dark remainder (background)
		RGBA front_color{ 0.96f, 0.34f, 0.52f, 1.00f }; // instant HP (pink)
		RGBA after_color{ 0.95f, 0.76f, 0.25f, 0.95f }; // delayed drain stripe (gold)

		// Thickness inside (top/bottom inset so base outline peeks)
		float inset_y_px = 1.f;

		// Behavior
		float drain_speed = 1.5f;      // how fast the delayed bar catches up
		float show_duration = 1.25f;   // if not persistent
		bool  persist_after_first_hit = true;

		// Runtime state
		int   last_hp = -1;
		int   last_max = -1;
		float front_ratio = 1.f;       // 0..1 (instant)
		float after_ratio = 1.f;       // 0..1 (delayed)
		float visible_until = 0.f;
		bool  has_been_hit = false;
		bool  initialized = false;

		// Sprites (rects)
		Engine::ECS::Entity base{};
		Engine::ECS::Entity after{};
		Engine::ECS::Entity front{};

		// Texture (white rect)
		Engine::Graphics::Texture tex{};
	};

}