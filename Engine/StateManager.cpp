//-----------------------------------------------------------------------------------
// file:    StateManager.cpp
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   This file implements the game state management system, including state
//          stack operations for transitions, push/pop functionality, and lifecycle
//          management for handling state initialization and cleanup.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------

#include "StateManager.h" 
#include <memory> 

namespace Engine
{
  /* ===================== //
  // State Transitions     //
  // ===================== */
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

  /* ===================== //
  // Stack Operations      //
  // ===================== */
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

  /* ===================== //
  // State Control         //
  // ===================== */
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

  /* ===================== //
  // State Access          //
  // ===================== */
  GameStateBase* StateManager::GetCurrentState()
  {
    return stateStack.back();
  }
}
