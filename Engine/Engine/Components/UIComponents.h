//-----------------------------------------------------------------------------------
// file:    UIComponents.h
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   ECS component definitions for user interface elements. Includes
//          Clickable component for handling mouse interactions and button
//          functionality. Provides the foundation for interactive UI elements
//          in the game and editor interface.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include <functional>
namespace Components
{
  struct Clickable
  {
    std::function<void()> on_click;

    std::function<void(Engine::ECS::Entity)> on_hover;

    std::function<void(Engine::ECS::Entity)> exit_hover;
  };

  struct Chat 
  {

    // Path to the json dialogue texts
    std::string json_file_path;

    // The actual text content loaded from the file
    std::string display_text;

    // Configuration for rendering
    int font_size = 18;
    glm::vec3 font_color = glm::vec3(1.0f, 1.0f, 1.0f); //RGB

    //Maximum characters to display per line for wrapping
    unsigned int max_line_length = 50;
  };
}