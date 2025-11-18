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
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
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

        class ChatBox : public Engine::ECS::System {
        public:
          ChatBox() {
            require_components<Components::Chat, Components::Transform2D > ();
          }


          void LoadTextFromJSON(Engine::ECS::Entity entity, Components::Chat& chat_component)
          {
            // Check if a path is defined
            if (chat_component.json_file_path.empty())
            {
              std::cerr << "Error: Chat component on entity " << entity.get_id() << " has no JSON file path defined." << std::endl;
              return;
            }

            // Read the file content
            std::ifstream file(chat_component.json_file_path);
            if (!file.is_open())
            {
              std::cerr << "Error: Could not open JSON file: "
                << chat_component.json_file_path << std::endl;
              return;
            }

            // Parse the JSON through JSON library
            try
            {
              // Read file into a string
              std::stringstream buffer;
              buffer << file.rdbuf();
              std::string json_string = buffer.str();

              // Hypothetical JSON Parsing: Replace with your actual JSON library calls
              // auto j = json::parse(json_string);
              // chat_component.display_text = j.at("text_content").get<std::string>();

              // For simplicity, let's assume a dummy load:
              chat_component.display_text = "Text successfully loaded from " + chat_component.json_file_path;
            }
            catch (const std::exception& e)
            {
              std::cerr << "Error parsing JSON for entity " << entity.get_id()
                << ": " << e.what() << std::endl;
            }
          }


          // Update method of your ChatBox System (where rendering would occur)
          void Update(float dt)
          {
            // Use the ECS World query method to iterate over matching entities
            Engine::ECS::for_each_entity_with<Components::Chat, Components::Transform2D>([&](Engine::ECS::Entity entity)
              {
                // Use pointer dereferencing for component access, similar to TimelineState.h
                auto* chat_ptr = entity.get_component<Components::Chat>();
                auto* transform_ptr = entity.get_component<Components::Transform2D>();

                // Defensive check, though for_each_entity_with should guarantee existence
                if (!chat_ptr || !transform_ptr) return;

                // Use references for cleaner code within the loop
                Components::Chat& chat = *chat_ptr;
                Components::Transform2D& transform = *transform_ptr;

                // Initialization/Loading Check
                if (chat.display_text.empty() && !chat.json_file_path.empty())
                {
                  LoadTextFromJSON(entity, chat);
                }

                //// --- Rendering Logic ---
                //if (!chat.display_text.empty())
                //{
                //   Engine::Graphics::TextRenderer::DrawText(
                //       chat.display_text, 
                //       transform.position, 
                //       chat.font_size, 
                //       chat.font_color 
                //   );
                //}
              });

          }
        };

  }
}