//-----------------------------------------------------------------------------------
// file:    WinMain.cpp
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   This file contains the main entry point for the game engine. It sets up
//          memory leak detection, initializes the core engine, and handles top-level
//          exception catching for robust error reporting.
//
// Copyright � 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------


#include "../Engine/MonkeyEngine.h"
#include "GameStates/ShowcaseState.h"
#include "GUI/StateChooser.h"
#include <exception>
#include <stdexcept>
#include "Config.h"

/*=================================================================================
Function Brief:
Main entry point for the application. Enables memory leak detection, initializes
and runs the game engine, and catches any unhandled exceptions for error logging.

Returns:
0 on successful execution, 1 if an exception occurred.
==================================================================================*/
int main()
{
  // Enable memory leak detection
  s32 tmpDbgFlag = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);
  tmpDbgFlag |= _CRTDBG_LEAK_CHECK_DF;
  _CrtSetDbgFlag(tmpDbgFlag);
  _CrtSetBreakAlloc(-1);

  try
  {

    Config::load_config();
    s32 win_width = Config::get_win_width();
    s32 win_height = Config::get_win_height();

    // Start the engine
    Engine::Core engine(win_width, win_height, "Monkey Stole My GPA", ShowcaseState::get());
    ImGuiManager::register_panel(StateChooserPanel);
    engine.run();

    FileHandler::clear_json();

  }
  catch (const std::exception& e)
  {
    Log::crash(std::string("Unhandled exception in main: ") + e.what());
    return 1;
  }
  catch (...)
  {
    Log::crash("Unknown unhandled exception in main");
    return 1;
  }

  return 0;
}
