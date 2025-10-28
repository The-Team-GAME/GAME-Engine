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
	
		Engine::Resources::set_resource_path("Assets/"); //default resource path

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
		bro.get_component<Components::Animation>()->isPlaying = true;
		const s32 bro_layer = 0;
		const s32 bro_start_frame = 10;
		const s32 bro_duration = 60;

		//banana.get_component<Components::TextureAtlas>()->drawIndex = 1;
		s32 bro_id = bro.get_id();
		timeline.set_selected_entity_id(bro_id);
		entity_visibility[bro_id] = false;
		timeline_entities.push_back(bro);


		monkey = Engine::ECS::create_entity()
			.add_component<Components::Transform2D>(600.0f, 400.0f, 0.0f, 120.0f, 150.0f)
			.add_component<Components::Color>(1.0f, 1.0f, 1.0f, 0.0f)
			.add_component<Components::Texture>(Engine::Resources::load_texture("images/monkey.png"));
		const s32 monkey_layer = 1;
		const s32 monkey_start_frame = 5;
		const s32 monkey_duration = 40;

		s32 monkey_id = monkey.get_id();
		timeline.set_selected_entity_id(monkey_id);
		entity_visibility[monkey_id] = false;
		timeline_entities.push_back(monkey);


		// Add the clip to the timeline
		timeline.add_clip(
			bro_id,  // Texture ID (Placeholder)
			0,                  // Layer 0
			10,                 // Start Frame 10
			60                  // Duration 60 frames
		);

		timeline.add_clip(
			monkey_id,  // Texture ID (Placeholder)
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


		// 1. **RESET ALL ENTITIES to INVISIBLE**
		for (auto& pair : entity_visibility) {
			pair.second = false;
		}

		// 2. **CHECK CLIPS TO SET VISIBILITY**
		for (const auto& clip : timeline.get_clips()) {
			s32 entity_id = clip.texture_id; // Using texture_id as Entity ID

			s32 end_frame = clip.start_frame + clip.duration_frames;

			if (current_frame >= clip.start_frame && current_frame < end_frame) {
				// The entity should be visible in this frame!
				// Set its visibility flag to true.
				if (entity_visibility.count(entity_id)) {
					entity_visibility[entity_id] = true;
				}

				// Optional: If you only have one clip per entity ID and you are
				// ONLY checking existence, you could break here for that ID.
				// But since multiple clips for one entity are possible (e.g., entity
				// appears, disappears, reappears), it's safer to keep iterating
				// to ensure all clips for all entities are checked.
			}
		}

		// 3. **DRAW VISIBLE ENTITIES**
		// This is the part that interacts with your engine's drawing system.
		if (entity_visibility[bro.get_id()]) {
			// You must call your engine's draw function for the 'bro' entity here.
			// Assuming you have a way to draw an entity based on its ID or object.
			// Example (replace with your actual engine call):
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


	void free() {};
	void unload() {};
};


