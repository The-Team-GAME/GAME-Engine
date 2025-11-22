//-----------------------------------------------------------------------------------
// file:    ChatBoxSystem.h
// project: Monkey Stole My GPA
//
// author:  Ryan Ang An Kiat (100%)
//          Primary author
//
// email:   a.ryanankiat@digipen.edu
//
// brief:  .
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------

#include "../ECS/ECS.h"
#include "../Components/Components.h"
#include "../InputManager.h"
#include "../FileIO/Helpers.h"
#include "../Reflection/ReflectionLibrary.h"
#include "../Path.h"
#include "../Resources.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>
#include <string>
#include <regex>

namespace Engine
{
  namespace Systems
  {
    struct Dialogue {
      std::string speaker;
      std::string text;
    };

    //inline std::vector<Dialogue> sample_dialogues = {
    //  {"Alice", "Hello there! Welcome to our game."},
    //  {"Bob", "Thanks, Alice! I'm excited to be here."},
    //  {"Alice", "Feel free to explore and have fun!"},
    //  {"Bob", "Will do! Let's get started."}
    //};

    class ChatBox : public Engine::ECS::System {

    private:

      // Entities
      Engine::ECS::Entity chatbox_background_entity;
      Engine::ECS::Entity chatbox_text_entity;

      // Dialogues loaded from JSON
      std::vector<Dialogue> dialogues;

      // State
      bool is_active = false;
      size_t current_dialogue_index = 0;
      bool dialogue_finished = false; // true when we've displayed all dialogues and finished typing

      // Input handling
      bool input_cooldown = false;
      f32 cooldown_timer = 0.0f;
      const f32 COOLDOWN_DURATION = 0.3f;

      // Typing effect
      bool is_typing = false;
      std::string full_text = "";
      std::string displayed_text = "";
      size_t current_char_index = 0;
      f32 char_timer = 0.0f;
      const f32 CHARS_PER_SECOND = 40.0f;

      // Chatbox configuration (adjust these for your screen)
      const f32 CHATBOX_X = 960.0f;
      const f32 CHATBOX_Y = 900.0f;
      const f32 CHATBOX_WIDTH = 1800.0f;
      const f32 CHATBOX_HEIGHT = 180.0f;

      const f32 TEXT_X = 100.0f;
      const f32 TEXT_Y = 830.0f;
      const f32 TEXT_SCALE = 0.7f;

    public:
      ChatBox() {}


      // Initialize the chatbox (call this once in your state's init())
      void init(Engine::Graphics::Font font) {
        // Only create a background entity if none was provided externally
        if (!chatbox_background_entity.is_valid()) {
          chatbox_background_entity = Engine::ECS::create_entity();
        }

        // If a text entity wasn't supplied, create a default one
        if (!chatbox_text_entity.is_valid()) {
          chatbox_text_entity = Engine::ECS::create_entity()
            .add_component<Components::Transform2D>(TEXT_X, TEXT_Y, 0.0f, TEXT_SCALE, TEXT_SCALE)
            .add_component<Components::Font>(font)
            .add_component<Components::Text>("Press F to start dialogue")
            .add_component<Components::Color>(1.0f, 1.0f, 1.0f, 0.0f); // Start hidden (alpha = 0)
        }
        else {
          // Ensure provided entity has required components
          if (!chatbox_text_entity.has_component<Components::Font>()) {
            chatbox_text_entity.add_component<Components::Font>(font);
          }
          if (!chatbox_text_entity.has_component<Components::Text>()) {
            chatbox_text_entity.add_component<Components::Text>("Press F to start dialogue");
          }
          if (!chatbox_text_entity.has_component<Components::Color>()) {
            chatbox_text_entity.add_component<Components::Color>(1.0f, 1.0f, 1.0f, 0.0f);
          }
          else {
            auto* c = chatbox_text_entity.get_component<Components::Color>();
            c->a = 0.0f; // hide initially
          }
        }

        is_active = false;
        current_dialogue_index = 0;
        dialogue_finished = false;

        // Try to auto-load dialogues from the standard JSON path in resources
        // This will silently fall back to having no dialogues if loading fails
        load_dialogues_from_file("Assets/Dialogue/dialogue_test.json");
      }

      // Allow external code to set the text entity (ShowcaseState uses this)
      void set_text_entity(const Engine::ECS::Entity& entity) {
        chatbox_text_entity = entity;
      }

      // Allow external code to set the background entity
      void set_background_entity(const Engine::ECS::Entity& entity) {
        chatbox_background_entity = entity;
      }

      // Public API: load dialogues from JSON file (path relative to resource path or absolute)
      bool load_dialogues_from_file(const std::string& filepath) {
        // Build candidate full path using resource_path if available
        std::string full_path = filepath;
        if (!Engine::Resources::resource_path.empty()) {
          Engine::Path p(Engine::Resources::resource_path);
          full_path = p.join(filepath).to_string();
        }

        // Try reading file via FileIO helper
        auto opt = Engine::FileIO::read_string_from_file(full_path);
        if (!opt.has_value()) {
          // Fallback: try filepath as given
          opt = Engine::FileIO::read_string_from_file(filepath);
          if (!opt.has_value()) {
            return false;
          }
        }

        std::string json_str = opt.value();

        // Sanitize common trailing-comma errors (remove commas before ] or })
        try {
          std::regex trailing_comma_re(",\s*([\]}])");
          json_str = std::regex_replace(json_str, trailing_comma_re, "$1");
        }
        catch (...) {
        }

        // Parse using reflection helper
        std::optional<ReflectionLibrary::GenericValue> parsed = ReflectionLibrary::generic_value_from_json(json_str);
        if (!parsed.has_value()) {
          return false;
        }

        ReflectionLibrary::GenericValue root = parsed.value();

        // Expecting an array of dialogue objects; if it's an object with a field, try to find it
        ReflectionLibrary::GenericValue arr;
        if (root.isArray()) {
          arr = root;
        }
        else if (root.isObject()) {
          // Common wrappers could be { "dialogues": [ ... ] }
          if (root.has("dialogues") && root["dialogues"].isArray()) {
            arr = root["dialogues"];
          }
          else {
            return false;
          }
        }

        // Convert array elements to Dialogue structures
        std::vector<Dialogue> loaded;
        for (size_t i = 0; i < arr.size(); ++i) {
          const ReflectionLibrary::GenericValue& elem = arr[i];
          if (!elem.isObject()) continue;

          std::string speaker;
          std::string text;

          if (elem.has("speaker") && elem["speaker"].isString()) {
            speaker = elem["speaker"].getString();
          }

          // Support both "text" and "text_content" keys
          if (elem.has("text") && elem["text"].isString()) {
            text = elem["text"].getString();
          }
          else if (elem.has("text_content") && elem["text_content"].isString()) {
            text = elem["text_content"].getString();
          }

          if (!text.empty()) {
            loaded.push_back(Dialogue{ speaker, text });
          }
        }

        if (!loaded.empty()) {
          dialogues = std::move(loaded);
          reset_dialogue();
          return true;
        }

        return false;
      }

      // Update logic (call this every frame)
      void update_entities(f32 delta_time) {
        handle_input(delta_time);

        if (is_active && is_typing) {
          update_typing_effect(delta_time);
        }
      }

      // Draw the chatbox background
      void draw() {
        if (!is_active) return;

				// Background chatbox rectangle
        Engine::Graphics::draw_rect_ui(
          Math::Vec2D(CHATBOX_X, CHATBOX_Y),
          Math::Vec2D(CHATBOX_WIDTH, CHATBOX_HEIGHT),
          0.0f,
          Math::Vec4D(0.0f, 0.0f, 0.0f, 0.85f)
        );

				// draw background entity if valid
        if (chatbox_background_entity.is_valid()) {
          auto* texComp = chatbox_background_entity.get_component<Components::Texture>();
          auto* xf = chatbox_background_entity.get_component<Components::Transform2D>();
          auto* col = chatbox_background_entity.get_component<Components::Color>();

          if (texComp && xf) {
            Math::Vec4D tint = col ? Math::Vec4D(col->r, col->g, col->b, col->a) : Math::Vec4D(1.0f, 1.0f, 1.0f, 1.0f);
            Engine::Graphics::draw_texture_ui(
              texComp->texture_handle,
              Math::Vec2D(xf->position.x, xf->position.y),
              Math::Vec2D(xf->scale.x, xf->scale.y),
              xf->rotation,
              tint
            );
            return;
          }
        }
      }

      // Toggle chatbox visibility
      void toggle() {
        is_active = !is_active;

        if (is_active) {
          show();
        }
        else {
          hide();
        }
      }

      void show() {
        is_active = true;

        auto* text_color = chatbox_text_entity.get_component<Components::Color>();
        if (text_color) text_color->a = 1.0f;

        reset_dialogue();
        advance_dialogue();
      }

      void hide() {
        is_active = false;

        auto* text_color = chatbox_text_entity.get_component<Components::Color>();
        if (text_color) text_color->a = 0.0f;
      }

      bool is_chatbox_active() const {
        return is_active;
      }

      // Load dialogues from vector (for event-driven dialogues)
      void load_dialogues(const std::vector<Dialogue>& new_dialogues) {
        dialogues = new_dialogues;
        reset_dialogue();
      }

      // Cleanup (call in state's free())
      void cleanup() {
        if (chatbox_background_entity.is_valid()) {
          chatbox_background_entity.destroy();
        }
        if (chatbox_text_entity.is_valid()) {
          chatbox_text_entity.destroy();
        }
      }

    private:
      void handle_input(f32 delta_time) {
        // Handle cooldown
        if (input_cooldown) {
          cooldown_timer -= delta_time;
          if (cooldown_timer <= 0.0f) {
            input_cooldown = false;
          }
        }

        // Toggle chatbox with F key (or advance when active)
        if (Engine::Input::is_pressed(Engine::KeyCode::F)) {
          // If chatbox is not active, open it
          if (!is_active) {
            toggle();
            input_cooldown = true;
            cooldown_timer = COOLDOWN_DURATION;
            return;
          }

          // If active, treat F as 'advance' key: skip typing if still typing, otherwise advance to next dialogue
          if (is_typing) {
            skip_typing();
          } else {
            // If we've finished the whole list, restart on F
            if (dialogue_finished) {
              reset_dialogue();
              advance_dialogue();
            }
            else {
              advance_dialogue();
            }
          }

          input_cooldown = true;
          cooldown_timer = COOLDOWN_DURATION;
          return;
        }

        // Only handle Enter if chatbox is active
        if (!is_active || input_cooldown) return;

        if (Engine::Input::is_pressed(Engine::KeyCode::Enter)) {
          if (is_typing) {
            // Skip typing animation
            skip_typing();
          }
          else {
            // Advance to next dialogue
            advance_dialogue();
          }

          input_cooldown = true;
          cooldown_timer = COOLDOWN_DURATION;
        }
      }

      void update_typing_effect(f32 delta_time) {
        char_timer += delta_time;
        f32 time_per_char = 1.0f / CHARS_PER_SECOND;

        while (char_timer >= time_per_char && current_char_index < full_text.length()) {
          displayed_text += full_text[current_char_index];
          current_char_index++;
          char_timer -= time_per_char;

          update_text_display();
        }

        // Check if typing is complete
        if (current_char_index >= full_text.length()) {
          is_typing = false;
          // When Dialogue is done
          if (current_dialogue_index >= dialogues.size()) {
            dialogue_finished = true;

            // Append the restart prompt to the displayed text so players see how to restart
            if (chatbox_text_entity.is_valid()) {
              auto* text = chatbox_text_entity.get_component<Components::Text>();
              if (text) {
                text->content = displayed_text + "\nPress F to restart dialogue";
          }
        }
      }
        }
      }

      void skip_typing() {
        displayed_text = full_text;
        current_char_index = full_text.length();
        is_typing = false;
        update_text_display();

        // If this was the last dialogue, also show the restart prompt
        if (current_dialogue_index >= dialogues.size()) {
          dialogue_finished = true;
          if (chatbox_text_entity.is_valid()) {
            auto* text = chatbox_text_entity.get_component<Components::Text>();
            if (text) {
              text->content = displayed_text + "\nPress F to restart dialogue";
            }
          }
        }
      }

      void update_text_display() {
        if (!chatbox_text_entity.is_valid()) return;

        auto* text = chatbox_text_entity.get_component<Components::Text>();
        if (text) {
          text->content = displayed_text;
        }
      }

      void advance_dialogue() {
        if (!chatbox_text_entity.is_valid()) return;

        // If there are no dialogues, do nothing (and show hint)
        if (dialogues.empty()) {
          is_typing = false;
          dialogue_finished = true;
          auto* text = chatbox_text_entity.get_component<Components::Text>();
          if (text) text->content = "No dialogues loaded";
          return;
        }

        // If there are remaining dialogues, start the next one
        if (current_dialogue_index < dialogues.size()) {
          const auto& dialogue = dialogues[current_dialogue_index];
          start_typing(dialogue.speaker + ": " + dialogue.text);
          current_dialogue_index++;
        }
        else {
          // No more dialogues - mark finished
          dialogue_finished = true;
          auto* text = chatbox_text_entity.get_component<Components::Text>();
          if (text) {
            text->content = "Press F to restart dialogue";
          }
          is_typing = false;
        }
      }

      void start_typing(const std::string& text) {
        full_text = text;
        displayed_text = "";
        current_char_index = 0;
        is_typing = true;
        char_timer = 0.0f;
        update_text_display();
      }

      void reset_dialogue() {
        current_dialogue_index = 0;
        is_typing = false;
        displayed_text = "";
        full_text = "";
        dialogue_finished = false;
      }

      bool is_dialogue_finished() const {
        return current_dialogue_index >= dialogues.size();
      }

    };

  } // namespace Systems
} // namespace Engine