//-----------------------------------------------------------------------------------
// file:    ParticleSystem.h
// project: Monkey Stole My GPA
//
// author:  Ong Zi Ming (100%)
//          Primary author
//
// email:   o.ziming@digipen.edu
//
// brief:   This file defines a Event-driven particle system (no input). Emits quads on demand and
//          destroys them when life run out.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once


#include "../ECS/System.h"
#include "../ECS/ECS.h"
#include "../Components/Components.h"
#include "../Resources.h"
#include "../InputManager.h"
#include "../Time.h"
#include "../EventManager/EventManager.h"


namespace Engine {
	namespace Systems {

		// Event for decoupled particle spawning.
		struct SpawnParticlesEvent {
			Math::Vec2D pos{ 0.f,0.f };
			int count{ 0 };
		};

		class ParticleSystem final : public Engine::ECS::System {
		public:
			ParticleSystem();
			~ParticleSystem() override;


			// Runs the particle simulation, spawns new particles, and writes back to ECS.
			void update() override;


		private:
			// ------------------------- Internal data -------------------------
      struct ParticleSlot {
        Engine::ECS::Entity entity{}; // invalid when free
        Math::Vec2D velocity{ 0.f, 0.f };
        float life{ 0.f };              // seconds left
      };

      struct EmitterDesc {
        Math::Vec2D position{ 0.f, 0.f };
        bool playing{ false };
        float rate{ 0.f };

        float life_min{ 1.5f };
        float life_max{ 10.0f };

        float speed_min{ 160.f };
        float speed_max{ 300.f };
        float angle_deg{ 0.f };
        float spread_deg{ 80.f };

        float gravity{ 520.f };
        float drag{ 0.15f };
        float fade_rate{ 0.9f };
        float shrink_rate{ 5.f };

      };

      // lifecycle
      void lazy_init();
      bool ensure_entity(ParticleSlot& s);

      // spawning/sim
      void spawn_burst(int n);
      void spawn_one(ParticleSlot& s);
      void simulate(float dt);

      // helpers
      static inline float deg2rad(float d) { return d * 0.01745329251994329577f; }

      // state
      int pool_size_{ 512 };
      std::vector<ParticleSlot> pool_;
      EmitterDesc emitter_{};
      float spawn_accum_{ 0.f };
      bool initialized_{ false };
      bool subscribed_{ false };
    };


	} // namespace Systems
} // namespace Engine