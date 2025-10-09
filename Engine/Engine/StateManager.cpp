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
  void StateManager::change_state(GameStateBase* newState)
  {
    // Clear all existing states
    while (!state_stack.empty())
    {
      GameStateBase* state = get_current_state();
      state->free();
      state->unload();
      state_stack.pop_back();
      //stateDataStack.pop_back();
    }

    // Add the new state
    state_stack.push_back(newState);
    //stateDataStack.push_back(data);
    newState->load();
    newState->init();
  }

  /* ===================== //
  // Stack Operations      //
  // ===================== */
  void StateManager::push_state(GameStateBase* newState)
  {
    state_stack.push_back(newState);
    //stateDataStack.push_back(data);
    get_current_state()->load();
    get_current_state()->init();
  }

  bool StateManager::pop_state()
  {
    //if (state_stack.empty() || stateDataStack.empty())
    if (state_stack.empty())
    {
      // No state to pop
      return false;
    }

    // Unload current state 
    get_current_state()->free();
    get_current_state()->unload();
    state_stack.pop_back();
    //stateDataStack.pop_back();

    // If we still have states, initialize the new top state
    if (!state_stack.empty())
    {
      // dont need to load again because prev data is still preserved
      get_current_state()->init();
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
  void StateManager::restart()
  {
    if (!state_stack.empty())
    {
      get_current_state()->free();
      get_current_state()->init();
    }
  }

  void StateManager::init(GameStateBase* initialState)
  {
    state_stack.push_back(initialState);
    get_current_state()->load();
    get_current_state()->init();
  }

  void StateManager::exit()
  {
    // Clean up all states in the stack
    while (!state_stack.empty())
    {
      get_current_state()->free();
      get_current_state()->unload();
      state_stack.pop_back();
    }
  }

  /* ===================== //
  // State Access          //
  // ===================== */
  GameStateBase* StateManager::get_current_state()
  {
    return state_stack.back();
  }
}
