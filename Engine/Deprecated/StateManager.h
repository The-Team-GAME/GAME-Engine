// DEPRECATED FILE - DO NOT USE
//-----------------------------------------------------------------------------------
// file:    StateManager.h
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   This file defines the game state management system, including the base
//          state interface, template for state implementation, and the StateManager
//          class for handling state transitions and game lifecycle.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
// DEPRECATED FILE - DO NOT USE



//#pragma once
//
//#include <memory> 
//#include <any>
//#include <vector>
//
//namespace Engine
//{
//
//  /*=================================================================================
//  Class Brief:
//  Abstract base class for game states. Defines the interface that all game states
//  must implement, including lifecycle methods for loading, initialization, updates,
//  rendering, freeing, and unloading of resources.
//  ==================================================================================*/
//  class GameStateBase
//  {
//  public:
//    /*=================================================================================
//    Function Brief:
//    Loads resources for the state. Responsible for loading any assets or resources
//    required by the state.
//
//    Where to call it:
//    Called when the state is first created or when transitioning to this state.
//    ==================================================================================*/
//    virtual void load() {};
//
//    /*=================================================================================
//    Function Brief:
//    Initializes the state. Sets up the initial configuration or variables for the
//    state.
//
//    Where to call it:
//    Called after Load() and is meant to prepare the state for updates and rendering.
//    ==================================================================================*/
//    virtual void init() {};
//
//    /*=================================================================================
//    Function Brief:
//    Updates the state logic. Called every frame to handle game logic, user input,
//    and other dynamic aspects of the state.
//
//    Where to call it:
//    Called every frame during the game loop.
//    ==================================================================================*/
//    virtual void update() {};
//
//    /*=================================================================================
//    Function Brief:
//    Renders the state. Responsible for drawing the visual components of the state.
//
//    Where to call it:
//    Called every frame after Update() to render the current state to the screen.
//    ==================================================================================*/
//    virtual void draw() {};
//
//    /*=================================================================================
//    Function Brief:
//    Frees static resources used by the state. Releases any static resources (e.g.,
//    memory, objects) that the state uses.
//
//    Where to call it:
//    Typically called when the state transitions to another state or when the game ends.
//    ==================================================================================*/
//    virtual void free() {};
//
//    /*=================================================================================
//    Function Brief:
//    Unloads dynamic resources used by the state. Releases dynamically allocated
//    resources (e.g., assets, memory) for the state.
//
//    Where to call it:
//    Called when the state is completely removed from memory.
//    ==================================================================================*/
//    virtual void unload() {};
//
//    /*=================================================================================
//    Function Brief:
//    Virtual destructor. Ensures that derived classes are properly destroyed when
//    the state is deleted.
//    ==================================================================================*/
//    virtual ~GameStateBase() {}
//  };
//
//  /*=================================================================================
//  Class Brief:
//  Template class for managing singleton game states. Provides a singleton
//  implementation for game states derived from GameStateBase. Ensures that only
//  one instance of the derived game state exists at any given time.
//
//  Example usage:
//  class MainMenuState : public GameState<MainMenuState>
//  {
//  public:
//      void Load() override { // Load resources }
//      void Init() override { // Initialize layout }
//      void Update() override { // Handle input and logic }
//      void Draw() override { // Render }
//      void Free() override { // Free static resources }
//      void Unload() override { // Unload resources }
//  };
//
//  // Initial run state
//  StateManager::Init(MainMenuState::Get());
//
//  // Change to main menu state
//  StateManager::change_state(MainMenuState::Get());
//  ==================================================================================*/
//  template <typename Derived>
//  class GameState : public GameStateBase
//  {
//  public:
//    /*=================================================================================
//    Function Brief:
//    Returns the singleton instance of the derived game state class.
//
//    Returns:
//    Pointer to the singleton instance of the derived state.
//    ==================================================================================*/
//    static Derived* get()
//    {
//      static Derived instance{};
//      return &instance;
//    }
//  };
//
//  /*=================================================================================
//  Class Brief:
//  Manages the game state stack, lifecycle, and state transitions. Responsible for
//  initializing, updating, and switching between game states using a stack-based
//  approach for state management.
//  ==================================================================================*/
//  class StateManager
//  {
//  public:
//    /*=================================================================================
//    Function Brief:
//    Changes the current game state by clearing the entire state stack and adding
//    the new state. Calls Free() and Unload() on all existing states before loading
//    and initializing the new state.
//
//    Parameters:
//    newState - Pointer to the new game state to switch to.
//
//    Where to call it:
//    Use when you want to completely replace the current state(s) with a new state,
//    such as transitioning from main menu to gameplay.
//    ==================================================================================*/
//    static void change_state(GameStateBase* newState);
//
//    /*=================================================================================
//    Function Brief:
//    Pushes a new state onto the state stack without removing existing states. The
//    new state becomes the active state while previous states remain in memory.
//
//    Parameters:
//    newState - Pointer to the new game state to push onto the stack.
//
//    Where to call it:
//    Use when you want to overlay a new state on top of existing ones, such as
//    opening a pause menu or settings screen.
//    ==================================================================================*/
//    static void push_state(GameStateBase* newState);
//
//    /*=================================================================================
//    Function Brief:
//    Pops the current state from the stack and returns to the previous state. If
//    the stack becomes empty, the game will stop.
//
//    Returns:
//    True if successfully popped a state, false if stack was empty.
//
//    Where to call it:
//    Use when closing an overlaid state to return to the previous state, such as
//    closing a pause menu to resume gameplay.
//    ==================================================================================*/
//    static bool pop_state();
//
//    /*=================================================================================
//    Function Brief:
//    Restarts the current game state by calling Free() and Init() on it, effectively
//    resetting it to its default state without changing states.
//
//    Where to call it:
//    Use when you want to reset the current state, such as restarting a level after
//    a game over.
//    ==================================================================================*/
//    static void restart();
//
//    /*=================================================================================
//    Function Brief:
//    Initializes the state manager with an initial game state. This should be called
//    once at engine startup.
//
//    Parameters:
//    initialState - Pointer to the initial game state to start with.
//
//    Where to call it:
//    Call once during engine initialization before entering the main game loop.
//    ==================================================================================*/
//    static void init(GameStateBase* initialState);
//
//    /*=================================================================================
//    Function Brief:
//    Cleans up all states in the stack by calling Free() and Unload() on each state
//    and clearing the stack.
//
//    Where to call it:
//    Call during engine shutdown to properly clean up all game states.
//    ==================================================================================*/
//    static void exit();
//
//    /*=================================================================================
//    Function Brief:
//    Gets the current active state from the top of the stack.
//
//    Returns:
//    Pointer to the current game state.
//    ==================================================================================*/
//    static GameStateBase* get_current_state();
//
//  private:
//    inline static std::vector<std::unique_ptr<GameStateBase>> state_stack;
//  };
//}
