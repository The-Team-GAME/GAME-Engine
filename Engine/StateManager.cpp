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
  void StateManager::ChangeState(GameStateBase* newState)
  {
    // Clear all existing states
    while (!stateStack.empty())
    {
      GameStateBase* state = GetCurrentState();
      state->Free();
      state->Unload();
      stateStack.pop_back();
      //stateDataStack.pop_back();
    }

    // Add the new state
    stateStack.push_back(newState);
    //stateDataStack.push_back(data);
    newState->Load();
    newState->Init();
  }

  void StateManager::PushState(GameStateBase* newState)
  {
    stateStack.push_back(newState);
    //stateDataStack.push_back(data);
    GetCurrentState()->Load();
    GetCurrentState()->Init();
  }

  bool StateManager::PopState()
  {
    //if (stateStack.empty() || stateDataStack.empty())
    if (stateStack.empty())
    {
      // No state to pop
      return false;
    }

    // Unload current state 
    GetCurrentState()->Free();
    GetCurrentState()->Unload();
    stateStack.pop_back();
    //stateDataStack.pop_back();

    // If we still have states, initialize the new top state
    if (!stateStack.empty())
    {
      // dont need to load again because prev data is still preserved
      GetCurrentState()->Init();
      return true;
    }
    else
    {
      // TODO: No more states, stop the game
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

  void StateManager::Init(GameStateBase* initialState)
  {
    stateStack.push_back(initialState);
    GetCurrentState()->Load();
    GetCurrentState()->Init();
  }

  void StateManager::Exit()
  {
    // Clean up all states in the stack
    while (!stateStack.empty())
    {
      GetCurrentState()->Free();
      GetCurrentState()->Unload();
      stateStack.pop_back();
    }
  }

  GameStateBase* StateManager::GetCurrentState()
  {
    return stateStack.back();
  }
}
