//-----------------------------------------------------------------------------------
// file:    HealthComponents.h
// project: Monkey Stole My GPA
//
// author:  Ong Zi Ming (100%)
//          Primary author
//
// email:   o.ziming@digipen.edu
//
// brief:   This file defines struts required for the health system.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------

#pragma once

#include "../../Engine/MonkeyEngine.h"
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

	struct HealthWaveHUD {
		// Placement
		Math::Vec2D anchor_px{ 16.f, 16.f };
		float width = 420.f;
		float height = 52.f;
		int   segments = 48;
		std::string layer = "ui";

		// Visuals
		Engine::Graphics::Texture tex{};
		struct RGBA { float r = 1, g = 1, b = 1, a = 1; };
		RGBA back_color{ 0.07f, 0.10f, 0.35f, 1.0f };
		RGBA after_color{ 1.00f, 0.28f, 0.48f, 1.0f };
		RGBA front_color{ 0.18f, 0.90f, 1.00f, 1.0f };
		float back_thickness = 10.f;
		float after_thickness = 8.0f;
		float front_thickness = 6.0f;

		// Behavior (drain/grow speed for the delayed layer)
		float drain_speed = 1.6f;

		// ECG styling
		float spike_width_norm = 0.03f;
		float spike_height_px = 22.f;
		float pulse_scale = 0.18f;

		// Runtime
		std::vector<Engine::ECS::Entity> back, after, front;
		float front_ratio = 0.f;   // 0..1 (instant)
		float after_ratio = 0.f;   // 0..1 (delayed)
		float beat_phase = 0.f;   // 0..1 within beat
		float spike_pos = 0.f;   // 0..1 sweep left->right
		bool  initialized = false;
	};

	struct HeartHUDSprite {
		Math::Vec2D anchor_px{ 20.f, 18.f };
		float width = 763.f;
		float height = 145.f;
		std::string layer = "ui";
		bool  screen_space = true;

		// Textures (fill uses WhiteSolidForFill.png)
		Engine::Graphics::Texture tex_back{};
		Engine::Graphics::Texture tex_white{};
		Engine::Graphics::Texture tex_solid{};  // WhiteSolidForFill.png
		Engine::Graphics::Texture tex_head{};

		// Map BPM -> fill
		float bpm_min = 40.f, bpm_max = 200.f;

		// Pixel-accurate band
		float fill_left_px = 118.f;
		float fill_right_px = 730.f;
		float fill_center_y = 76.f;
		float fill_height_px = 42.f;

		// actual source (PNG) dimensions for the fill image (pixels)
		int   src_w_px = 763;        // set to WhiteSolidForFill.png width
		int   src_h_px = 145;        // set to WhiteSolidForFill.png height
		int   slices = 140;        // number of vertical columns

		bool use_white_overlay = false;

		// Runtime
		Engine::ECS::Entity back;
		Engine::ECS::Entity head;
		Engine::ECS::Entity white_overlay;
		std::vector<Engine::ECS::Entity> whiteSlices;   // column reveal
		bool initialized = false;
	};

	struct EnemyHealthBar {
		// Layout
		float radius = 24.0f;
		float start_angle_deg = 160.0f;   // arc left -> right above head
		float end_angle_deg = 20.0f;
		int   segments = 28;
		Math::Vec2D offset{ 0.f, -44.f };   // above head

		// Rendering/layering
		std::string layer = "enemy";

		bool  use_center_span = true;
		float center_angle_deg = 90.0f;
		float arc_span_deg = 140.0f;

		// Visuals
		Engine::Graphics::Texture tex{};
		struct RGBA { float r = 1, g = 1, b = 1, a = 1; };
		RGBA front_color{ 0.2f, 1.0f, 0.3f, 1.0f };   // instant layer
		RGBA after_color{ 1.0f, 0.85f, 0.25f, 0.95f }; // delayed layer
		float front_thickness = 6.0f;
		float after_thickness = 8.5f; // slightly thicker, sits behind

		// Behavior
		float drain_speed = 1.5f;     // ratio per second for delayed bar
		float show_duration = 1.25f;  // visible window after last damage
		bool  persist_after_first_hit = true;
		bool  has_been_hit = false;

		// Runtime
		std::vector<Engine::ECS::Entity> front; // immediate
		std::vector<Engine::ECS::Entity> after; // delayed
		int   last_hp = -1;
		int   last_max = -1;
		float front_ratio = 1.0f; // 0..1
		float after_ratio = 1.0f; // 0..1
		float visible_until = 0.0f;
		bool  initialized = false;
	};

}