//-----------------------------------------------------------------------------------
// file:    ShowcaseState.cpp
// project: Monkey Stoke My GPA
//
// author:  Ryan Ang An Kiat(100%)
//          Primary author
//
// email:   a.ryanankiat@digipen.edu
//
// brief:   Implements a chat box that prints dialogue lines with typing effect
//          that loads on pressing F key. Handles dialogue progression and rendering.
//          Reads dialogue data from JSON file.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include "../ECS/ECS.h"
#include "../Components/Components.h"
#include "../InputManager.h"
#include "../FileIO/Helpers.h"
#include "../Reflection/ReflectionLibrary.h"
#include "../Path.h"
#include "../Resources.h"
#include "../Graphics/Renderer.h"
#include <regex>
#include <string>
#include <vector>

namespace Engine
{
  namespace Systems
  {
    struct Dialogue {
      std::string speaker; // Name of the character speaking
      std::string text;    // The actual dialogue line
    };

    //inline std::vector<Dialogue> sample_dialogues = {
    //  {"Alice", "Hello there! Welcome to our game."},
    //  {"Bob", "Thanks, Alice! I'm excited to be here."},
    //  {"Alice", "Feel free to explore and have fun!"},
    //  {"Bob", "Will do! Let's get started."}
    //};


    // ChatBox system for displaying and managing dialogue.
    class ChatBox : public Engine::ECS::System {
    private:
      // Dialogues loaded from JSON
      std::vector<Dialogue> dialogues;

      // Follow target configuration (for floating chat boxes)
      Engine::ECS::Entity follow_entity;
      Math::Vec2D follow_offset_world{ 0.0f, 0.0f };

      // Rendering resources
      Engine::Graphics::Texture background_texture;
      Engine::Graphics::Font font;

      // State management
      bool is_active = false;         // If the chat box is currently displayed
      bool dialogue_finished = false; // If all dialogues in the current set have been shown
      bool is_typing = false;         // If text is currently being revealed character by character
      bool marked_for_removal = false; // Set true after final F press
      size_t current_dialogue_index = 0; // Index of the dialogue currently being displayed

      // Typing effect variables
      std::string full_text;          // The complete, word-wrapped text for the current dialogue
      std::string displayed_text;     // The portion of text currently visible
      size_t current_char_index = 0;
      f32 char_timer = 0.0f;          // Accumulates time for character reveal
      const f32 CHARS_PER_SECOND = 40.0f; // Speed of the typing effect

      // Input handling variables
      bool input_cooldown = false;    // Prevents immediate double-input/skips
      f32 cooldown_timer = 0.0f;
      const f32 COOLDOWN_DURATION = 0.3f; // Duration of input cooldown

      // Layout configuration (Static UI space coordinates/sizes)
      const Math::Vec2D BG_SIZE{ 1600.0f, 700.0f };
      const Math::Vec2D BG_CENTER_STATIC{ 800.0f, 500.0f }; // Default center if not following entity

      // Inner overlay for text area
      const Math::Vec2D OVERLAY_SIZE{ 600.0f, 150.0f };
      const Math::Vec2D OVERLAY_CENTER_STATIC{ 1150.0f, 700.0f };

      // Text appearance parameters
      const f32 TEXT_SCALE = 0.7f;
      const f32 TEXT_PADDING_LEFT = 20.0f;
      const f32 TEXT_PADDING_RIGHT = 20.0f;
      const f32 TEXT_PADDING_TOP = 15.0f;

      // Computed layout (updated per frame if following entity)
      Math::Vec2D bg_center_ui{ BG_CENTER_STATIC };
      Math::Vec2D overlay_center_ui{ OVERLAY_CENTER_STATIC };
      Math::Vec2D text_pos_ui{ OVERLAY_CENTER_STATIC }; // Text starting position (top-left)

      // Colors (RGBA)
      Math::Vec4D bg_tint{ 1.0f,1.0f,1.0f,1.0f };
      Math::Vec4D overlay_color{ 0.0f,0.0f,0.0f,0.35f }; // Semi-transparent black overlay
      Math::Vec4D text_color{ 1.0f,1.0f,1.0f,1.0f };      // White text

    public:
      ChatBox() = default;

      // Public setters for resources and follow target
      void set_background_texture(Engine::Graphics::Texture tex) { background_texture = tex; }
      void set_font(Engine::Graphics::Font f) { font = f; }
      void set_follow_entity(const Engine::ECS::Entity entity) { follow_entity = entity; }
      void set_follow_offset(const Math::Vec2D& offset) { follow_offset_world = offset; }

      // Initialize the system with resources and reset state
      void init(Engine::Graphics::Font f, Engine::Graphics::Texture bg_tex) {
        font = f;
        background_texture = bg_tex;

        is_active = false;
        dialogue_finished = false;
        marked_for_removal = false;
        current_dialogue_index = 0;
        full_text.clear();
        displayed_text.clear();

        load_dialogues_from_file("Assets/Dialogue/dialogue_test.json");
        // Start hidden; user will press F to show
      }

      // Loads dialogue data from a JSON file
      bool load_dialogues_from_file(const std::string& filepath) {
        std::string full_path = filepath;
        // Logic to determine the correct resource path
        if (!Engine::Resources::resource_path.empty()) {
          Engine::Path p(Engine::Resources::resource_path);
          full_path = p.join(filepath).to_string();
        }

        // Read file content into string
        auto opt = Engine::FileIO::read_string_from_file(full_path);
        if (!opt.has_value()) {
          opt = Engine::FileIO::read_string_from_file(filepath);
          if (!opt.has_value()) {
            return false; // Failed to read
          }
        }

        std::string json_str = opt.value();
        try {
          // Workaround for potentially non-strict JSON parsers (removes trailing commas)
          std::regex trailing_comma_re(",\\s*([\\}\]])");
          json_str = std::regex_replace(json_str, trailing_comma_re, "$1");
        }
        catch (...) {}

        // Parse JSON string into a GenericValue object
        auto parsed = ReflectionLibrary::generic_value_from_json(json_str);
        if (!parsed.has_value()) return false;

        ReflectionLibrary::GenericValue root = parsed.value();

        // Extract dialogue array from root (handles both array and object structures)
        const ReflectionLibrary::Array* arr_ptr = nullptr;
        if (root.isArray()) {
          arr_ptr = &root.getArray();
        }
        else if (root.isObject()) {
          const auto& obj = root.getObject();
          auto it = obj.find("dialogues");
          if (it != obj.end() && it->second.isArray()) {
            arr_ptr = &it->second.getArray();
          }
        }

        if (!arr_ptr) {
          return false; // Dialogue array not found
        }

        const auto& arr_vec = *arr_ptr;

        std::vector<Dialogue> loaded;
        // Iterate and convert JSON objects to Dialogue structs
        for (size_t i = 0; i < arr_vec.size(); ++i) {
          const ReflectionLibrary::GenericValue& elem = arr_vec[i];
          if (!elem.isObject()) continue;

          std::string speaker, text;
          const auto& obj = elem.getObject();

          if (auto it = obj.find("speaker"); it != obj.end() && it->second.isString()) {
            speaker = it->second.getString();
          }
          if (auto it = obj.find("text"); it != obj.end() && it->second.isString()) {
            text = it->second.getString();
          }
          else if (auto it2 = obj.find("text_content"); it2 != obj.end() && it2->second.isString()) {
              text = it2->second.getString();
            }

          // Normalize escaped newlines ("\\n") to real newlines and treat stray single backslashes as line breaks.
          if (!text.empty()) {
            // First replace proper \n sequences
            size_t pos = 0;
            while ((pos = text.find("\\n", pos)) != std::string::npos) {
              text.replace(pos, 2, "\n");
              pos += 1;
            }
            // Then convert any remaining solitary backslashes into newlines (to tolerate malformed JSON input like "Monkey Stole\\My GPA")
            for (size_t i2 = 0; i2 < text.size(); ++i2) {
              if (text[i2] == '\\') {
                // If it was part of an earlier replacement it will now be followed by something else; convert to newline.
                text[i2] = '\n';
              }
            }
          }

          if (!text.empty()) loaded.push_back({ speaker, text });
        }

        if (!loaded.empty()) {
          dialogues = std::move(loaded); // Replace existing dialogues
          reset_dialogue();
          return true;
        }
        return false;
      }

      // Main update loop, called every frame
      void update_entities(f32 dt) {
        handle_input(dt);
        // Always update layout so position tracks entity even when hidden
        update_layout_positions();
        if (!is_active) return;

        if (is_typing) {
          update_typing_effect(dt);
        }

        update_layout_positions(); // Recalculate UI positions (especially if following entity)
      }

      // Drawing function, called every frame to render UI
      void draw_ui() {
        if (!is_active || marked_for_removal) return;
        if (!background_texture) return;
        if (!font) return;

        // Background image
        Engine::Graphics::draw_texture_ui(background_texture, bg_center_ui, BG_SIZE, 0.0f, bg_tint);

        // Overlay rectangle (tinted)
        Engine::Graphics::draw_rect_ui(overlay_center_ui, OVERLAY_SIZE, 0.0f, overlay_color);

        // Text (with typing effect)
        Engine::Graphics::draw_text_ui(font, displayed_text, text_pos_ui, TEXT_SCALE, text_color);
      }

      //// Toggles visibility and state
      //void toggle() { is_active ? hide() : show(); }

      // Activates the chat box and starts the first dialogue
      void show() {
        if (dialogues.empty()) {
          is_active = true;
          displayed_text = "No dialogues loaded";
          return;
        }
        is_active = true;
        text_color.w = 1.0f;     // Restore full opacity
        overlay_color.w = 0.35f; // Restore overlay opacity
        reset_dialogue();
        advance_dialogue();
      }

      // Hides the chat box
      void hide() {
        is_active = false;
        text_color.w = 0.0f;     // Make text transparent
        overlay_color.w = 0.0f;  // Make overlay transparent
      }

      // Cleanup method (clears dialogue data)
      void cleanup() {
        dialogues.clear();
      }

    private:
      // Calculates the UI positions based on follow entity or static coordinates
      void update_layout_positions() {
        // Calculate position if following an entity
        if (follow_entity.is_valid()) {
          // Requires Transform2D component to follow
          auto* xf = follow_entity.get_component<Components::Transform2D>();
          if (xf) {
            f32 vertical_gap_world = 10.0f;
            // Anchor below the entity, considering its scale
            f32 anchor_y_world = xf->position.y - (xf->scale.y * 0.5f) - vertical_gap_world;
            Math::Vec2D anchor_world_pos(xf->position.x + follow_offset_world.x,
              anchor_y_world + follow_offset_world.y);
            // Convert world position to screen/virtual UI coordinates
            Math::Vec2D ui_anchor = Engine::Graphics::world_to_virtual(anchor_world_pos);

            // Maintain the relative offset between overlay and background
            Math::Vec2D diff_overlay = OVERLAY_CENTER_STATIC - BG_CENTER_STATIC;

            bg_center_ui = ui_anchor;
            overlay_center_ui = ui_anchor + diff_overlay;

          }
          else {
            // Fallback to static if entity is valid but lacks Transform2D
            bg_center_ui = BG_CENTER_STATIC;
            overlay_center_ui = OVERLAY_CENTER_STATIC;
          }
        }
        else {
          // Use static positions if no entity to follow
          bg_center_ui = BG_CENTER_STATIC;
          overlay_center_ui = OVERLAY_CENTER_STATIC;
        }

        // Compute top-left of overlay for text positioning
        Math::Vec2D overlay_top_left(
          overlay_center_ui.x - OVERLAY_SIZE.x * 0.5f,
          overlay_center_ui.y - OVERLAY_SIZE.y * 0.5f
        );

        // Text position with padding (Text is rendered top-left justified)
        text_pos_ui.x = overlay_top_left.x + TEXT_PADDING_LEFT;
        text_pos_ui.y = overlay_top_left.y + TEXT_PADDING_TOP;
      }

      // Performs word wrapping on the text to fit the chat box overlay
      std::string wrap_text(const std::string& text) {
        if (!font || text.empty()) return text;

        f32 available_width = OVERLAY_SIZE.x - TEXT_PADDING_LEFT - TEXT_PADDING_RIGHT;
        f32 scale = TEXT_SCALE;

        std::string result;
        std::string current_line;

        size_t pos = 0;
        while (pos < text.size()) {
          // Find next space or explicit newline
          size_t space_pos = text.find(' ', pos);
          size_t newline_pos = text.find('\n', pos);

          size_t end_pos;
          bool is_newline = false;

          // Handle explicit newline characters first
          if (newline_pos != std::string::npos &&
            (space_pos == std::string::npos || newline_pos < space_pos)) {
            end_pos = newline_pos;
            is_newline = true;
          }
          else if (space_pos != std::string::npos) {
            end_pos = space_pos; // End at the next space for word break
          }
          else {
            end_pos = text.size(); // Last word
          }

          std::string word = text.substr(pos, end_pos - pos);

          if (is_newline) {
            // Commit current line, then force a new line
            if (!current_line.empty()) {
              if (!result.empty()) result += "\n";
              result += current_line;
              current_line.clear();
            }
            else {
              if (!result.empty()) result += "\n";
            }
            pos = end_pos + 1; // Skip the '\n'
            continue;
          }

          std::string test_line = current_line.empty() ? word : current_line + " " + word;
          Math::Vec2D size_test = Engine::Graphics::measure_text(font, test_line, scale);

          // Check if adding the word exceeds the available width
          if (size_test.x > available_width && !current_line.empty()) {
            // Line wrap: commit the previous line and start a new one with the current word
            if (!result.empty()) result += "\n";
            result += current_line;
            current_line = word;
          }
          else {
            current_line = test_line; // Append the word to the current line
          }

          // Move position past the word and potential trailing space
          if (space_pos != std::string::npos && end_pos == space_pos) {
            pos = end_pos + 1; // Skip the space
          }
          else {
            pos = end_pos; // End of string or segment
          }
        }

        // Commit the last line
        if (!current_line.empty()) {
          if (!result.empty()) result += "\n";
          result += current_line;
        }

        return result;
      }

      // Handles user input for showing/hiding/advancing/skipping dialogue
      void handle_input(f32 dt) {
        // Manage input cooldown timer
        if (input_cooldown) {
          cooldown_timer -= dt;
          if (cooldown_timer <= 0.0f) input_cooldown = false;
        }

        // Toggle/Advance/Restart via 'F' key
        if (Engine::Input::is_pressed(Engine::KeyCode::F)) {
          if (!is_active) {
            show();
          }
          else {
            if (is_typing) {
              skip_typing();
            }
            else {
              if (dialogue_finished) {
                // Final press: remove chat box (last line visible until now)
                hide();
                marked_for_removal = true;
                displayed_text.clear();
                full_text.clear();
              }
              else {
                advance_dialogue();
              }
            }
          }
          input_cooldown = true;
          cooldown_timer = COOLDOWN_DURATION;
          return;
        }

        if (!is_active || input_cooldown) return;

        // Advance/Skip via 'Enter' key
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

      // Updates the displayed text for the typing effect
      void update_typing_effect(f32 dt) {
        char_timer += dt;
        f32 time_per_char = 1.0f / CHARS_PER_SECOND;

        // Reveal characters based on accumulated time
        while (char_timer >= time_per_char && current_char_index < full_text.size()) {
          displayed_text += full_text[current_char_index];
          current_char_index++;
          char_timer -= time_per_char;
        }

        // Check if typing is complete
        if (current_char_index >= full_text.size()) {
          is_typing = false;
          if (current_dialogue_index >= dialogues.size()) {
            dialogue_finished = true; // Keep last line visible until user presses F
          }
        }
      }

      // Immediately reveals the remaining text
      void skip_typing() {
        displayed_text = full_text;
        current_char_index = full_text.size();
        is_typing = false;
        if (current_dialogue_index >= dialogues.size()) dialogue_finished = true;
      }

      // Moves to the next line of dialogue
      void advance_dialogue() {
        if (dialogues.empty()) {
          displayed_text = "No dialogues loaded";
          dialogue_finished = true;
          is_typing = false;
          return;
        }

        if (current_dialogue_index < dialogues.size()) {
          const auto& dlg = dialogues[current_dialogue_index];
          // Format: "Speaker: Text" or just "Text"
          start_typing(dlg.speaker.empty() ? dlg.text : dlg.speaker + ": " + dlg.text);
          current_dialogue_index++;
        }
        else {
          dialogue_finished = true; // Do not hide; wait for final F
        }
      }

      // Prepares text for display and starts the typing effect
      void start_typing(const std::string& text) {
        full_text = wrap_text(text); // Pre-wrap the text
        displayed_text.clear();
        current_char_index = 0;
        char_timer = 0.0f;
        is_typing = true;
      }

      // Resets state to the beginning of the dialogue sequence
      void reset_dialogue() {
        current_dialogue_index = 0;
        displayed_text.clear();
        full_text.clear();
        current_char_index = 0;
        char_timer = 0.0f;
        is_typing = false;
        dialogue_finished = false;
        marked_for_removal = false;
      }
    };

  } // namespace Systems
} // namespace Engine