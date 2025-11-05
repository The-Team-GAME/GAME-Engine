#pragma once

#include "../../Engine/MonkeyEngine.h"
#include "../../Engine/GUI/Timeline.h"
#include "../../Engine/GUI/ResourcePicker.h"


// Define the class and inherit from the GameState template
class TimelineState : public Engine::Scene {
private:
	// Declare member variable
	Timeline timeline;
	Engine::ECS::Entity bro;
	Engine::ECS::Entity monkey;
	std::unordered_map<s32, bool> entity_visibility;
	std::vector<Engine::ECS::Entity> timeline_entities;

	std::string pending_texture_path; // Path selected by the picker
	bool is_texture_picker_open = false; // Flag to open the picker
	std::string current_resource_path = "";

	// The desired size/columns for the picker grid
	static constexpr int RESOURCE_GRID_COLUMNS = 5;
	static constexpr float RESOURCE_PREVIEW_SIZE = 96.0f;

public:
	// Declare public methods
	void load() {};
	void init() {
	
		Engine::Resources::set_resource_path("Assets/"); //default resource path

		Engine::ECS::Entity create_timeline_entity(const std::string & texture_path);

		// Function to draw the timeline ImGui panel (to be implemented in Timeline.cpp)
		void draw_timeline_panel();

		//animating entity
		bro = Engine::ECS::create_entity()
			.add_component<Components::Transform2D>(400.0f, 400.0f, 0.0f, 1000.0f/ 7.0f, 130.0f)
			.add_component<Components::Color>(1.0f, 1.0f, 1.0f, 0.0f)
			.add_component<Components::Texture>(Engine::Resources::load_texture("images/Bro_Idle_Spritesheet.png"))
			.add_component<Components::TextureAtlas>(448.0f/ 7.0f, 64.0f, 1, 7, 7) //index is set default to 0 playable around with
		  .add_component<Components::Animation>(7, 0.1);
		//.add_component<Components::TextureAtlas>(banana.get_component<Components::Transform2D>()->scale.x, banana.get_component<Components::Transform2D>()->scale.y, 7.0f, 0.0f, 7);
		/*.add_component<Components::RigidBody>();
		banana.get_component<Components::RigidBody>()->angular_velocity = 10.0f;*/

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
			s32 entity_id = clip.entity_id; // Using texture_id as Entity ID

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
	

	Engine::ECS::Entity create_timeline_entity(const std::string& texture_path) {
		// 1. Load the texture handle (Engine::Graphics::Texture is the value type)
		Engine::Graphics::Texture texture_value = Engine::Resources::load_texture(texture_path);

		// Correct validity check: Assuming Engine::Graphics::Texture overloads operator bool or has a simple check.
		// If texture_value overloads 'operator->' to access TextureData, we can check the pointer directly.
		if (!texture_value.operator->()) { // Check if the underlying shared_ptr is null
			return Engine::ECS::create_entity();
		}

		Engine::ECS::Entity new_entity = Engine::ECS::create_entity();

		// 2. Add required components
		new_entity.add_component<Components::Transform2D>(
			400.0f, 400.0f, 0.0f, 100.0f, 100.0f);
		new_entity.add_component<Components::Color>(1.0f, 1.0f, 1.0f, 0.0f);

		// Pass the texture value directly to the component constructor
		new_entity.add_component<Components::Texture>(texture_value);

		// 3. Initialize Texture Atlas. Access members of TextureData via operator->
		new_entity.add_component<Components::TextureAtlas>(
			(f32)texture_value->width, (f32)texture_value->height,
			1, 1, 1
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

 
