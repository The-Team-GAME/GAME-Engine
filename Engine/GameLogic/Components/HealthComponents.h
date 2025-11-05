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

}