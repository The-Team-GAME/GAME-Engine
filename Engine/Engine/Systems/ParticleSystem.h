//-----------------------------------------------------------------------------------
// file:    ParticleSystem.h
// project: Monkey Stole My GPA
//
// author:   (100%)
//          Primary author
//
// email:   @digipen.edu
//
// brief:   This file defines a particle system.
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
			struct Particle {
				bool alive{ false };
				Engine::ECS::Entity e{}; // pooled ECS quad entity


				float life{ 0.f }; // remaining life (s)
				float max_life{ 1.f };


				Math::Vec2D pos{ 0.f, 0.f };
				Math::Vec2D vel{ 0.f, 0.f };
				float rot{ 0.f };
				float ang_vel{ 0.f };


				float size0{ 8.f }; // size at birth
				float size1{ 1.f }; // size at death


				Components::Color start{ 1.f,1.f,1.f,1.f };
				Components::Color end{ 1.f,1.f,1.f,0.f };


				float gravity{ 280.f };
				float drag{ 0.8f };
			};


			struct EmitterDesc {
				Math::Vec2D position{ 480.f, 260.f };
				float rate{ 220.f }; // particles/sec
				int burst{ 0 }; // one-shot count
				bool playing{ false };


				// initial velocity cone
				float speed_min{ 80.f };
				float speed_max{ 180.f };
				float spread_deg{ 40.f };
				float angle_deg{ 90.f }; // 90 = up


				// lifetime/size
				float life_min{ 0.6f };
				float life_max{ 1.2f };
				float size0{ 8.f };
				float size1{ 1.f };


				// color over life
				Components::Color color0{ 1.0f, 0.9f, 0.4f, 1.0f };
				Components::Color color1{ 1.0f, 0.2f, 0.0f, 0.0f };


				// forces
				float gravity{ 280.f };
				float drag{ 0.8f };
			};


		private:
			// ------------------------- Helpers -------------------------
			void lazy_init();
			void spawn_one();
			void spawn_burst(int count);
			void simulate(float dt);


			static float rand01();
			static float rand_range(float a, float b);
			static inline float deg2rad(float d) { return d * 0.017453292519943295769f; }


			// ------------------------- Pool -------------------------
			std::vector<Particle> pool_{};
			std::vector<int> free_list_{}; // indices of free particles
			int pool_size_{ 512 };


			// ------------------------- Runtime -------------------------
			EmitterDesc emitter_{};
			float spawn_accum_{ 0.f };
			bool initialized_{ false };

			bool subscribed_{ false };

		};


	} // namespace Systems
} // namespace Engine