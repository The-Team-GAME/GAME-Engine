//-----------------------------------------------------------------------------------
// file:    ChatBoxSystem.h
// project: Monkey Stole My GPA
//
// author:  Ryan Ang An Kiat (100%)
//          Primary author
//
// email:   a.ryanankiat@digipen.edu
//
// brief:   .
//
// Copyright � 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------

#include "../ECS/ECS.h"
#include "../Components/Components.h"
#include "../InputManager.h"
#include <fstream>
#include <sstream>
#include <iostream>

namespace Engine
{
  namespace Systems
  {
    struct Dialogue {
      std::string speaker;
      std::string text;
    };

    std::vector<Dialogue> sample_dialogues = {
      {"Alice", "Hello there! Welcome to our game."},
      {"Bob", "Thanks, Alice! I'm excited to be here."},
      {"Alice", "Feel free to explore and have fun!"},
      {"Bob", "Will do! Let's get started."}
    };

    class ChatBox : public Engine::ECS::System {

    private:
      // State tracking for the current dialogue line
      size_t current_dialogue_index = 0;
      bool input_cooldown = false;

      // Helper to advance to the next line of dialogue
      void advance_dialogue(Components::Text& text_comp) {
        if (current_dialogue_index < sample_dialogues.size()) {
          const auto& dialogue = sample_dialogues[current_dialogue_index];
          // Format the text component content to include speaker
          text_comp.content = dialogue.speaker + ": " + dialogue.text;
          current_dialogue_index++;
        }
        else {
          // Dialogue finished, maybe hide the entity or reset.
          text_comp.content = "--- End of Dialogue ---";
          current_dialogue_index = sample_dialogues.size(); // Keep index fixed at end
        }
      }


    public:
      ChatBox() {
        require_components < Components::Chat,
          Components::Transform2D,
          Components::Text,
          Components::Font,
          Components::Color>();
      }

       
      // Update method of your ChatBox System (where rendering would occur)
      void draw_entity(const ECS::Entity& entity) override
      {
        // Use pointer dereferencing for component access, similar to TimelineState.h
        auto* chat = entity.get_component<Components::Chat>();
        auto* transform = entity.get_component<Components::Transform2D>();
        auto* text = entity.get_component<Components::Text>();
        auto* font = entity.get_component<Components::Font>();
        auto* color = entity.get_component<Components::Color>();

        // Defensive check, though for_each_entity_with should guarantee existence
        if (!chat || !transform) return;


        if (!text->content.empty())
        {
          // This is the functional call to draw the text on screen.
          // Assuming Engine::Graphics::RenderText is the utility used by your engine
          // for entities with Components::Text and Components::Font (as seen in ShowcaseState).
          void draw_text_ui(Engine::Graphics::Font font, const std::string & text, const Math::Vec2D & position, f32 scale, const Math::Vec4D & color);
      }
    };

  }
};