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
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#include "Core.h"
#include "Logger.h"
#include <exception>
#include <stdexcept>

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
    // Start the engine
    Engine::Core engine{};
    engine.run();
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
