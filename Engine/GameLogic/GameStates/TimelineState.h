#pragma once

#include "../../Engine/MonkeyEngine.h"
#include "../../Engine/GUI/Timeline.h"


// Define the class and inherit from the GameState template
class TimelineState : public Engine::GameState<TimelineState> {
private:
	// Declare member variable
	Timeline timeline;
	Engine::ECS::Entity banana;
public:
	// Declare public methods
	void load() {};
	void init() {
	
		Engine::Resources::set_resource_path("Assets/");

		banana = Engine::ECS::create_entity()
			.add_component<Components::Transform2D>(400.0f, 400.0f, 0.0f, 448.0f/ 7.0f, 64.0f)
			.add_component<Components::Color>(1.0f, 1.0f, 1.0f, 1.0f)
			.add_component<Components::Texture>(Engine::Resources::load_texture("images/Bro_Idle_Spritesheet.png"))
			.add_component<Components::TextureAtlas>(448.0f/ 7.0f, 64.0f, 1, 7, 7) //index is set default to 0 playable around with
		  .add_component<Components::Animation>(7, 0.1);
		//.add_component<Components::TextureAtlas>(banana.get_component<Components::Transform2D>()->scale.x, banana.get_component<Components::Transform2D>()->scale.y, 7.0f, 0.0f, 7);

		/*
			.add_component<Components::RigidBody>();

		banana.get_component<Components::RigidBody>()->angular_velocity = 10.0f;*/
		 

		banana.get_component<Components::Animation>()->isPlaying = true;

		//banana.get_component<Components::TextureAtlas>()->drawIndex = 1;
	
	};
	void update() {};
	void draw() {
		Engine::Graphics::clear_screen({ 0.078f, 0.078f, 0.078f, 1.0f });
		timeline.draw();
		std::cout << "hello??";

		//banana.get_component<Components::Transform2D>()->rotation += 100.0f;

		 
	};
	void free() {};
	void unload() {};
};


