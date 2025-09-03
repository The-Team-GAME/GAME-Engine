//-----------------------------------------------------------------------------------
// file:    StateManager.cpp
// project: CLIMB
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   Defines the game state management system, including the base state
//          interface, template for state implementation, and the StateManager
//          namespace for handling state transitions and game lifecycle.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------

#include "StateManager.h" 
#include <memory> 

namespace Engine
{
  void StateManager::ChangeState(GameStateBase* newState, std::any data)
  {
    // Clear all existing states
    while (!stateStack.empty())
    {
      GameStateBase* state = GetCurrentState();
      state->Free();
      state->Unload();
      stateStack.pop_back();
      stateDataStack.pop_back();
    }

    // Add the new state
    stateStack.push_back(newState);
    stateDataStack.push_back(data);
    newState->Load();
    newState->Init();
  }

  void StateManager::PushState(GameStateBase* newState, std::any data)
  {
    stateStack.push_back(newState);
    stateDataStack.push_back(data);
    GetCurrentState()->Load();
    GetCurrentState()->Init();
  }

  bool StateManager::PopState()
  {
    if (stateStack.empty() || stateDataStack.empty())
    {
      // No state to pop
      return false;
    }

    // Unload current state 
    GetCurrentState()->Free();
    GetCurrentState()->Unload();
    stateStack.pop_back();
    stateDataStack.pop_back();

    // If we still have states, initialize the new top state
    if (!stateStack.empty())
    {
      // dont need to load again because prev data is still preserved
      GetCurrentState()->Init();
      return true;
    }
    else
    {
      // No more states, stop the game
      Stop();
      return false;
    }
  }

  bool StateManager::PopState(std::any data)
  {
    if (stateStack.empty() || stateDataStack.empty())
    {
      // No state to pop
      return false;
    }

    // Unload current state 
    GetCurrentState()->Free();
    GetCurrentState()->Unload();
    stateStack.pop_back();
    stateDataStack.pop_back();

    // If we still have states, initialize the new top state with the new data
    if (!stateStack.empty())
    {
      // Replace the data for the new current state
      stateDataStack.back() = data;

      GetCurrentState()->Init();
      return true;
    }
    else
    {
      // No more states, stop the game
      Stop();
      return false;
    }
  }

  void StateManager::Restart()
  {
    if (!stateStack.empty())
    {
      GetCurrentState()->Free();
      GetCurrentState()->Init();
    }
  }

  void StateManager::Stop()
  {
    //running = false;
  }

  void StateManager::Run(GameStateBase* initialState)
  {

    //// Start with the initial state
    //stateStack.push_back(initialState);
    //stateDataStack.push_back(std::any()); // Empty data for initial state 

    //GetCurrentState()->Load();
    //GetCurrentState()->Init();

    //while (running && !stateStack.empty())
    //{
    //  /*AESysFrameStart();
    //  dt = (float)AEFrameRateControllerGetFrameTime();*/


    //  /*if (AESysDoesWindowExist() == 0)
    //    StateManager::Stop();*/


    //    // Get current state from top of stack
    //  GetCurrentState()->Update();
    //  GetCurrentState()->Draw();


    //  //AESysFrameEnd();
    //}

    //// Clean up all states in the stack
    //while (!stateStack.empty())
    //{
    //  GetCurrentState()->Free();
    //  GetCurrentState()->Unload();
    //  stateStack.pop_back();
    //  stateDataStack.pop_back();
    //}

  }

  GameStateBase* StateManager::GetCurrentState()
  {
    return stateStack.back();
  }

  std::any& StateManager::GetStateData()
  {
    return stateDataStack.back();
  }


}
