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
      Engine::ECS::Entity chatbox_overlay_entity;

      // Dialogues loaded from JSON
      std::vector<Dialogue> dialogues;

      // Follow target (optional)
      Engine::ECS::Entity follow_entity;
      Math::Vec2D follow_offset_world{ 0.0f, 0.0f };

      // State
      bool is_active = false;
      size_t current_dialogue_index = 0;
      bool dialogue_finished = false;

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

      // Chatbox configuration
      const f32 CHATBOX_X = 1050.0f;
      const f32 CHATBOX_Y = 1100.0f;
      const f32 CHATBOX_WIDTH = 600.0f;
      const f32 CHATBOX_HEIGHT = 150.0f;

      const f32 TEXT_X = 100.0f;
      const f32 TEXT_Y = 830.0f;
      const f32 TEXT_SCALE = 0.7f;

      // Text padding from edges of overlay rectangle
      const f32 TEXT_PADDING_LEFT = 20.0f;
      const f32 TEXT_PADDING_RIGHT = 20.0f;
      const f32 TEXT_PADDING_TOP = 15.0f;

    public:
      ChatBox() {}

      void set_follow_entity(const Engine::ECS::Entity& entity) {
        follow_entity = entity;
      }

      void set_follow_offset(const Math::Vec2D& offset) {
        follow_offset_world = offset;
      }

      void init(Engine::Graphics::Font font) {
        if (!chatbox_background_entity.is_valid()) {
          chatbox_background_entity = Engine::ECS::create_entity();
        }

        if (!chatbox_text_entity.is_valid()) {
          chatbox_text_entity = Engine::ECS::create_entity()
            .add_component<Components::Transform2D>(TEXT_X, TEXT_Y, 0.0f, TEXT_SCALE, TEXT_SCALE)
            .add_component<Components::Font>(font)
            .add_component<Components::Text>("Press F to start dialogue")
            .add_component<Components::Color>(1.0f, 1.0f, 1.0f, 0.0f);
        }
        else {
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
            c->a = 0.0f;
          }

          if (!chatbox_text_entity.has_component<Components::Transform2D>()) {
            chatbox_text_entity.add_component<Components::Transform2D>(TEXT_X, TEXT_Y, 0.0f, TEXT_SCALE, TEXT_SCALE);
          }
        }

        is_active = false;
        current_dialogue_index = 0;
        dialogue_finished = false;

        load_dialogues_from_file("Assets/Dialogue/dialogue_test.json");

        if (!dialogues.empty()) {
          show();
        }
      }

      void set_text_entity(const Engine::ECS::Entity& entity) {
        chatbox_text_entity = entity;
      }

      void set_background_entity(const Engine::ECS::Entity& entity) {
        chatbox_background_entity = entity;
      }

      void set_overlay_entity(const Engine::ECS::Entity& entity) {
        chatbox_overlay_entity = entity;
      }

      bool load_dialogues_from_file(const std::string& filepath) {
        std::string full_path = filepath;
        if (!Engine::Resources::resource_path.empty()) {
          Engine::Path p(Engine::Resources::resource_path);
          full_path = p.join(filepath).to_string();
        }

        auto opt = Engine::FileIO::read_string_from_file(full_path);
        if (!opt.has_value()) {
          opt = Engine::FileIO::read_string_from_file(filepath);
          if (!opt.has_value()) {
            return false;
          }
        }

        std::string json_str = opt.value();

        try {
          std::regex trailing_comma_re(",\\s*([\\]})");
          json_str = std::regex_replace(json_str, trailing_comma_re, "$1");
        }
        catch (...) {
        }

        auto parsed = ReflectionLibrary::generic_value_from_json(json_str);
        if (!parsed.has_value()) {
          return false;
        }

        ReflectionLibrary::GenericValue root = parsed.value();

        ReflectionLibrary::GenericValue arr;
        if (root.isArray()) {
          arr = root;
        }
        else if (root.isObject() && root.has("dialogues") && root["dialogues"].isArray()) {
          arr = root["dialogues"];
        }
        else {
          return false;
        }

        std::vector<Dialogue> loaded;
        for (size_t i = 0; i < arr.size(); ++i) {
          const ReflectionLibrary::GenericValue& elem = arr[i];
          if (!elem.isObject()) continue;

          std::string speaker, text;

          if (elem.has("speaker") && elem["speaker"].isString()) {
            speaker = elem["speaker"].getString();
          }

          if (elem.has("text") && elem["text"].isString()) {
            text = elem["text"].getString();
          }
          else if (elem.has("text_content") && elem["text_content"].isString()) {
            text = elem["text_content"].getString();
          }

          if (!text.empty()) {
            loaded.push_back({ speaker, text });
          }
        }

        if (!loaded.empty()) {
          dialogues = std::move(loaded);
          reset_dialogue();
          return true;
        }

        return false;
      }

      void update_entities(f32 delta_time) {
        handle_input(delta_time);

        if (is_active && is_typing) {
          update_typing_effect(delta_time);
        }

        if (follow_entity.is_valid()) {
          auto* target_xf = follow_entity.get_component<Components::Transform2D>();
          if (target_xf) {
            f32 vertical_gap_world = 10.0f;
            f32 anchor_y_world = target_xf->position.y - (target_xf->scale.y * 0.5f) - vertical_gap_world;
            Math::Vec2D anchor_world_pos(target_xf->position.x + follow_offset_world.x, anchor_y_world + follow_offset_world.y);

            Math::Vec2D ui_center = Engine::Graphics::world_to_virtual(anchor_world_pos);

            if (chatbox_background_entity.is_valid()) {
              if (!chatbox_background_entity.has_component<Components::Transform2D>()) {
                chatbox_background_entity.add_component<Components::Transform2D>(ui_center.x, ui_center.y, 0.0f, CHATBOX_WIDTH, CHATBOX_HEIGHT);
              }
              else {
                auto* bg_xf = chatbox_background_entity.get_component<Components::Transform2D>();
                bg_xf->position.x = ui_center.x;
                bg_xf->position.y = ui_center.y;

                auto* texCompBg = chatbox_background_entity.get_component<Components::Texture>();
                f32 final_w = CHATBOX_WIDTH;
                f32 final_h = CHATBOX_HEIGHT;

                if (texCompBg && texCompBg->texture_handle) {
                  auto& tex = *texCompBg->texture_handle;
                  if (tex.width > 0 && tex.height > 0) {
                    float texAspect = static_cast<float>(tex.width) / static_cast<float>(tex.height);
                    float boxAspect = CHATBOX_WIDTH / CHATBOX_HEIGHT;

                    if (texAspect > boxAspect) {
                      final_w = CHATBOX_WIDTH;
                      final_h = CHATBOX_WIDTH / texAspect;
                    }
                    else {
                      final_h = CHATBOX_HEIGHT;
                      final_w = CHATBOX_HEIGHT * texAspect;
                    }
                  }
                }

                bg_xf->scale.x = final_w;
                bg_xf->scale.y = final_h;
              }
            }

            if (chatbox_overlay_entity.is_valid() && chatbox_overlay_entity.has_component<Components::Transform2D>()) {
              auto* overlay_xf = chatbox_overlay_entity.get_component<Components::Transform2D>();
              overlay_xf->position.x = ui_center.x;
              overlay_xf->position.y = ui_center.y;
            }

            update_text_position();
          }
        }
        else {
          // Update text position in static mode
          update_text_position();
        }
      }

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
        if (text_color) {
          text_color->r = 1.0f;
          text_color->g = 1.0f;
          text_color->b = 1.0f;
          text_color->a = 1.0f;
        }

        if (chatbox_overlay_entity.is_valid() && chatbox_overlay_entity.has_component<Components::Color>()) {
          auto* overlay_color = chatbox_overlay_entity.get_component<Components::Color>();
          overlay_color->a = 0.35f;
        }

        reset_dialogue();
        advance_dialogue();
      }

      void hide() {
        is_active = false;

        auto* text_color = chatbox_text_entity.get_component<Components::Color>();
        if (text_color) {
          text_color->a = 0.0f;
        }

        if (chatbox_overlay_entity.is_valid() && chatbox_overlay_entity.has_component<Components::Color>()) {
          chatbox_overlay_entity.get_component<Components::Color>()->a = 0.0f;
        }
      }

      bool is_chatbox_active() const {
        return is_active;
      }

      void load_dialogues(const std::vector<Dialogue>& new_dialogues) {
        dialogues = new_dialogues;
        reset_dialogue();
      }

      void cleanup() {
        if (chatbox_background_entity.is_valid()) {
          chatbox_background_entity.destroy();
        }
        if (chatbox_overlay_entity.is_valid()) {
          chatbox_overlay_entity.destroy();
        }
        if (chatbox_text_entity.is_valid()) {
          chatbox_text_entity.destroy();
        }
      }

    private:
      void update_text_position() {
        if (!chatbox_text_entity.is_valid()) return;
        if (!chatbox_text_entity.has_component<Components::Transform2D>()) return;

        auto* txt_xf = chatbox_text_entity.get_component<Components::Transform2D>();

        // Get overlay rectangle position and size
        Math::Vec2D overlay_center(CHATBOX_X, CHATBOX_Y);
        Math::Vec2D overlay_size(CHATBOX_WIDTH, CHATBOX_HEIGHT);

        if (chatbox_overlay_entity.is_valid() && chatbox_overlay_entity.has_component<Components::Transform2D>()) {
          auto* overlay_xf = chatbox_overlay_entity.get_component<Components::Transform2D>();
          overlay_center = Math::Vec2D(overlay_xf->position.x, overlay_xf->position.y);
          overlay_size = Math::Vec2D(overlay_xf->scale.x, overlay_xf->scale.y);
        }

        // Calculate top-left corner of overlay
        Math::Vec2D top_left(
          overlay_center.x - overlay_size.x * 0.5f,
          overlay_center.y - overlay_size.y * 0.5f
        );

        // Position text with padding from top-left
        txt_xf->position.x = top_left.x + TEXT_PADDING_LEFT;
        txt_xf->position.y = top_left.y + TEXT_PADDING_TOP;
      }

      std::string wrap_text(const std::string& text) {
        if (!chatbox_text_entity.is_valid()) return text;

        auto* font_comp = chatbox_text_entity.get_component<Components::Font>();
        auto* transform = chatbox_text_entity.get_component<Components::Transform2D>();
        if (!font_comp || !transform) return text;

        // Calculate available width for text
        f32 overlay_width = CHATBOX_WIDTH;
        if (chatbox_overlay_entity.is_valid() && chatbox_overlay_entity.has_component<Components::Transform2D>()) {
          auto* overlay_xf = chatbox_overlay_entity.get_component<Components::Transform2D>();
          overlay_width = overlay_xf->scale.x;
        }

        f32 available_width = overlay_width - TEXT_PADDING_LEFT - TEXT_PADDING_RIGHT;
        f32 text_scale = (transform->scale.x + transform->scale.y) * 0.5f;

        std::string result;
        std::string current_line;

        // Split text into words
        size_t pos = 0;
        while (pos < text.length()) {
          // Find the next word
          size_t space_pos = text.find(' ', pos);
          size_t newline_pos = text.find('\n', pos);
          
          // Determine the end of the current word
          size_t end_pos;
          bool is_newline = false;
          
          if (newline_pos != std::string::npos && (space_pos == std::string::npos || newline_pos < space_pos)) {
            end_pos = newline_pos;
            is_newline = true;
          }
          else if (space_pos != std::string::npos) {
            end_pos = space_pos;
          }
          else {
            end_pos = text.length();
          }

          std::string word = text.substr(pos, end_pos - pos);

          // If we hit a newline in the original text, flush the current line
          if (is_newline) {
            if (!current_line.empty()) {
              if (!result.empty()) result += "\n";
              result += current_line;
              current_line.clear();
            }
            else {
              // Empty line (double newline)
              if (!result.empty()) result += "\n";
            }
            pos = end_pos + 1; // Skip the newline character
            continue;
          }

          // Try adding the word to the current line
          std::string test_line = current_line.empty() ? word : current_line + " " + word;
          Math::Vec2D text_size = Engine::Graphics::measure_text(font_comp->font_handle, test_line, text_scale);

          if (text_size.x > available_width && !current_line.empty()) {
            // Line would be too long, start new line
            if (!result.empty()) result += "\n";
            result += current_line;
            current_line = word;
          }
          else {
            current_line = test_line;
          }

          // Move to next position
          if (space_pos != std::string::npos && end_pos == space_pos) {
            pos = end_pos + 1; // Skip the space
          }
          else {
            pos = end_pos;
          }
        }

        // Add remaining text
        if (!current_line.empty()) {
          if (!result.empty()) result += "\n";
          result += current_line;
        }

        return result;
      }

      void handle_input(f32 delta_time) {
        if (input_cooldown) {
          cooldown_timer -= delta_time;
          if (cooldown_timer <= 0.0f) {
            input_cooldown = false;
          }
        }

        if (Engine::Input::is_pressed(Engine::KeyCode::F)) {
          if (!is_active) {
            toggle();
            input_cooldown = true;
            cooldown_timer = COOLDOWN_DURATION;
            return;
          }

          if (is_typing) {
            skip_typing();
          }
          else {
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

        if (!is_active || input_cooldown) return;

        if (Engine::Input::is_pressed(Engine::KeyCode::Enter)) {
          if (is_typing) {
            skip_typing();
          }
          else {
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

        if (current_char_index >= full_text.length()) {
          is_typing = false;

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
      }

      void skip_typing() {
        displayed_text = full_text;
        current_char_index = full_text.length();
        is_typing = false;
        update_text_display();

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

        if (dialogues.empty()) {
          is_typing = false;
          dialogue_finished = true;
          auto* text = chatbox_text_entity.get_component<Components::Text>();
          if (text) {
            text->content = "No dialogues loaded";
          }
          return;
        }

        if (current_dialogue_index < dialogues.size()) {
          const auto& dialogue = dialogues[current_dialogue_index];
          start_typing(dialogue.speaker + ": " + dialogue.text);
          current_dialogue_index++;
        }
        else {
          dialogue_finished = true;
          auto* text = chatbox_text_entity.get_component<Components::Text>();
          if (text) {
            text->content = "Press F to restart dialogue";
          }
          is_typing = false;
        }
      }

      void start_typing(const std::string& text) {
        // Wrap text before starting typing animation
        full_text = wrap_text(text);
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