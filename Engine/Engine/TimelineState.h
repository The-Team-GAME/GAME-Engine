//-----------------------------------------------------------------------------------
// file:    TimelineState.h
// project: Monkey Stole My GPA
//
// author:  Ryan Ang An Kiat (100%)
//          Primary author
//
// email:   a.ryanankiat@digipen.edu
//
// brief:   This file defines an alternative implementation of the TimelineState for
//          the game engine. It demonstrates timeline-based entity management with
//          frame-accurate visibility control and animation sequencing. Entities are
//          shown/hidden based on timeline clips, and sprite animations are controlled
//          by the current frame position within each clip's duration.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include "../../Engine/MonkeyEngine.h"
#include "../../Engine/GUI/Timeline.h"


// Define the class and inherit from the GameState template
class TimelineState : public Engine::GameState<TimelineState> {
private:
	// Declare member variable
	Timeline timeline;
	Engine::ECS::Entity bro;
	Engine::ECS::Entity monkey;
	std::unordered_map<s32, bool> entity_visibility;
	std::vector<Engine::ECS::Entity> timeline_entities;


public:
	// Declare public methods
	void load() {};
	void init() {
		//animating entity
		bro = Engine::ECS::create_entity()
			.add_component<Components::Transform2D>(400.0f, 400.0f, 0.0f, 1000.0f / 7.0f, 130.0f)
			.add_component<Components::Color>(1.0f, 1.0f, 1.0f, 0.0f)
			.add_component<Components::Texture>(Engine::Resources::load_texture("images/Bro_Idle_Spritesheet.png"))
			.add_component<Components::TextureAtlas>(448.0f / 7.0f, 64.0f, 1, 7, 7) //index is set default to 0 playable around with
			.add_component<Components::Animation>(7, 0.1);
		//.add_component<Components::TextureAtlas>(banana.get_component<Components::Transform2D>()->scale.x, banana.get_component<Components::Transform2D>()->scale.y, 7.0f, 0.0f, 7);
		/*.add_component<Components::RigidBody>();
		banana.get_component<Components::RigidBody>()->angular_velocity = 10.0f;*/

		bro.get_component<Components::Animation>()->isPlaying = false; //for when the timeline wasnt controlling the frames(unused)
		//ImTextureID bro_texture_handle = (ImTextureID)(intptr_t)bro.get_component<Components::Texture>()->texture_handle;


		//banana.get_component<Components::TextureAtlas>()->drawIndex = 1;
		s32 bro_entity_id = bro.get_id();
		timeline.set_selected_entity_id(bro_entity_id);
		entity_visibility[bro_entity_id] = false;
		timeline_entities.push_back(bro);


		monkey = Engine::ECS::create_entity()
			.add_component<Components::Transform2D>(600.0f, 400.0f, 0.0f, 120.0f, 150.0f)
			.add_component<Components::Color>(1.0f, 1.0f, 1.0f, 0.0f)
			.add_component<Components::Texture>(Engine::Resources::load_texture("images/monkey.png"))
			.add_component<Components::TextureAtlas>(600.0f, 400.0f, 1, 1, 1);

		s32 monkey_entity_id = monkey.get_id();
		timeline.set_selected_entity_id(monkey_entity_id);
		entity_visibility[monkey_entity_id] = false;
		timeline_entities.push_back(monkey);


		// Add the clip to the timeline
		timeline.add_clip(
			bro_entity_id,  // Texture ID (Placeholder)
			0,                  // Layer 0
			10,                 // Start Frame 10
			60                  // Duration 60 frames
		);

		timeline.add_clip(
			monkey_entity_id,  // Texture ID (Placeholder)
			1,                  // Layer 0
			5,                 // Start Frame 10
			40                  // Duration 60 frames
		);
	};





	void update() {};
	void draw() {
		Engine::Graphics::clear_screen({ 0.078f, 0.078f, 0.078f, 1.0f });
		timeline.draw();


		//banana.get_component<Components::Transform2D>()->rotation += 100.0f;


		s32 current_frame = timeline.get_current_frame();


		//reset visibility flags
		for (auto& pair : entity_visibility) {
			pair.second = false;
		}

		//checks which entities should be visible this frame
		for (const auto& clip : timeline.get_clips()) {
			s32 entity_id = clip.texture_id; // Using texture_id as Entity ID

			s32 end_frame = clip.start_frame + clip.duration_frames;

			if (current_frame >= clip.start_frame && current_frame < end_frame) {
				// The entity should be visible in this frame!
				// Set its visibility flag to true.
				if (entity_visibility.count(entity_id)) {
					entity_visibility[entity_id] = true;
				}

				Engine::ECS::Entity entity = Engine::ECS::get_entity(entity_id);

				if (entity.has_component<Components::TextureAtlas>()) {
					auto* atlas = entity.get_component<Components::TextureAtlas>();

					size_t num_frames = atlas->subTextures.size();

					if (num_frames > 0) {
						// Frames passed since clip started (0-indexed)
						s32 frames_in_clip = current_frame - clip.start_frame;

						// Duration of the clip
						s32 total_clip_duration = clip.duration_frames;

						// Use integer division for clean frame progression
						size_t frame_index = (size_t)std::floor((f32)frames_in_clip / total_clip_duration * num_frames);

						// Clamp to the valid range [0, num_frames - 1]
						atlas->drawIndex = std::min(frame_index, num_frames - 1);
					}
				}
			}
		}

		// draws visible entities
		if (entity_visibility[bro.get_id()]) {

			bro.get_component<Components::Color>()->a = 1.0f;
		}

		else {
			bro.get_component<Components::Color>()->a = 0.0f;
		}

		if (entity_visibility[monkey.get_id()]) {
			// Example (replace with your actual engine call):
			monkey.get_component<Components::Color>()->a = 1.0f;
		}

		else {
			monkey.get_component<Components::Color>()->a = 0.0f;
		}

	};


	Engine::ECS::Entity create_timeline_entity(Engine::Graphics::Texture texture) {
		Engine::ECS::Entity new_entity = Engine::ECS::create_entity();

		// Add the Texture component (This holds the shared_ptr to the raw texture data)
		new_entity.add_component<Components::Texture>(texture);

		//// Add the Transform component (Required for positioning and scaling in the world/view)
		//new_entity.add_component<Components::Transform2D>(
		//    // Position, Rotation, Scale
		//    Math::Vec3D(0.0f, 0.0f, 0.0f), 
		//    Math::Vec3D(0.0f, 0.0f, 0.0f), 
		//    Math::Vec3D(1.0f, 1.0f, 1.0f)
		//);

			//sets any non atlas texture to use a default 1x1 atlas
		new_entity.add_component<Components::TextureAtlas>(
			1,       // numRows
			1,       // numColumns
			1,       // totalSubTextures
			0,       // currentSubTextureIndex (start at 0)
			1        // frameDuration (can be ignored for static sprites)
		);

		return new_entity;
	}


	void free() {};
	void unload() {
		for (Engine::ECS::Entity& entity : timeline_entities) {
			if (entity.is_valid()) {
				Engine::ECS::destroy_entity(entity);
			}
		}
		timeline_entities.clear();
	};
};


