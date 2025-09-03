
#pragma once

#include <memory> 
#include <any>
#include <vector>

/**
 * @class GameStateBase
 * @brief Abstract base class for game states.
 *
 * This class defines the interface for all game states. Each game state must implement
 * the following lifecycle methods to handle loading, initialization, updates, rendering,
 * freeing, and unloading of resources.
 */
class GameStateBase
{
public: 
  /**
   * @brief Loads resources for the state.
   *
   * This method is responsible for loading any assets or resources required by the state.
   * It is called when the state is first created or when transitioning to this state.
   */
  virtual void Load() {};

  /**
   * @brief Initializes the state.
   *
   * This method sets up the initial configuration or variables for the state. It is called
   * after `load()` and is meant to prepare the state for updates and rendering.
   */
  virtual void Init() {};

  /**
   * @brief Updates the state logic.
   *
   * This method is called every frame to handle game logic, user input, and other dynamic
   * aspects of the state.
   */
  virtual void Update() {};

  /**
   * @brief Renders the state.
   *
   * This method is responsible for drawing the visual components of the state. It is called
   * every frame after `update()` to render the current state to the screen.
   */
  virtual void Draw() {};

  /**
   * @brief Frees static resources used by the state.
   *
   * This method releases any static resources (e.g., memory, objects) that the state uses.
   * It is typically called when the state transitions to another state or when the game ends.
   */
  virtual void Free() {};

  /**
   * @brief Unloads dynamic resources used by the state.
   *
   * This method releases dynamically allocated resources (e.g., assets, memory) for the state.
   * It is called when the state is completely removed from memory.
   */
  virtual void Unload() {};

  /**
   * @brief Virtual destructor.
   *
   * Ensures that derived classes are properly destroyed when the state is deleted.
   */
  virtual ~GameStateBase() {}
};

/**
 * @class GameState
 * @brief Template class for managing singleton game states.
 *
 * This template class provides a singleton implementation for game states derived from `GameStateBase`.
 * It ensures that only one instance of the derived game state exists at any given time.
 *
 * @example
 * Example usage
 * @code{.cpp}
 * class MainMenuState : public GameState<MainMenuState> // <-- Inherits GameState derived from MainMenuState
 * {
 * public:
 *     void Load() override
 *     {
 *         // Load resources for the main menu
 *     }
 *
 *     void Init() override
 *     {
 *         // Initialize the main menu layout
 *     }
 *
 *     void Update() override
 *     {
 *         // Handle input and logic for the main menu
 *     }
 *
 *     void Draw() override
 *     {
 *         // Render the main menu
 *     }
 *
 *     void Free() override
 *     {
 *         // Free any static resources
 *     }
 *
 *     void Unload() override
 *     {
 *         // Unload resources for the main menu
 *     }
 * };
 *
 * // Initial run state
 * GameManager::run(MainMenuState::get(), hInstance, nCmdShow);
 *
 * // Change to main menu state
 * GameManager::ChangeState(MainMenuState::get());
 *
 * @endcode
 *
 * @tparam Derived The derived class type that represents the specific game state.
 */
template <typename Derived>
class GameState : public GameStateBase
{
public:
  static Derived* Get()
  {
    static Derived instance{};
    return &instance;
  }
};

/**
 * @class GameManager
 * @brief Manages the game state, lifecycle, and rendering resources.
 *
 * The GameManager class is responsible for initializing, updating, and switching between game states,
 * as well as managing shared rendering resources like meshes and frame timing.
 */
class GameManager
{
public:
  /**
   * @brief Changes the current game state.
   *
   * This function clears the entire state stack and adds the new state.
   *
   * @param newState Pointer to the new game state.
   * @param data Optional data to pass to the new state.
   */
  static void ChangeState(GameStateBase* newState, std::any data = std::any());

  /**
   * @brief Pushes a new state onto the state stack.
   *
   * This function adds a new state to the top of the stack.
   *
   * @param newState Pointer to the new game state.
   * @param data Optional data to pass to the new state.
   */
  static void PushState(GameStateBase* newState, std::any data = std::any());

  /**
   * @brief Pops the current state from the stack.
   *
   * This function removes the top state from the stack and returns to the previous state.
   * If the stack becomes empty, the game will stop.
   *
   * @return True if successfully popped a state, false if stack was empty.
   */
  static bool PopState();

  /**
   * @brief Pops the current state from the stack and passes data to the previous state.
   *
   * @param data Data to pass to the previous state.
   * @return True if successfully popped a state, false if stack was empty.
   */
  static bool PopState(std::any data);

  /**
   * @brief Restarts the current game state.
   *
   * This function reinitializes the current state, resetting it to its default state.
   */
  static void Restart();

  /**
   * @brief Stops the game loop.
   *
   * This function sets the running flag to false, terminating the game loop.
   */
  static void Stop();

  /**
   * @brief Runs the game loop with the initial game state.
   *
   * This function initializes the game engine, loads the initial state, and
   * runs the main game loop until the game is stopped.
   *
   * @param initialState Pointer to the initial game state.
   */
  static void Run(GameStateBase* initialState);

  /**
   * @brief Gets the current state data.
   *
   * Helper function to access the data for the current state (top of the stack).
   *
   * @return Reference to the current state's data.
   */
  static std::any& GetStateData();

  /**
   * @brief Gets the current state.
   *
   * Helper function to access the current state (top of the stack).
   *
   * @return Pointer to the current state.
   */
  static GameStateBase* GetCurrentState();

  // Getters
  static float GetDt();

private:
  inline static bool running = false;
  inline static float dt = 0.0f;
  inline static bool stopTime = false;
  inline static bool slowTime = false;
  inline static bool godMode = false;
  inline static std::vector<GameStateBase*> stateStack;
  inline static std::vector<std::any> stateDataStack;
};

//inline DevConsole console; // this allows all states to access the console
