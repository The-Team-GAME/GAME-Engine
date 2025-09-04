#pragma once

#include "StateManager.h" 

class SpriteState : public Engine::GameState<SpriteState>
{
private:

public:
	/*!
	 * @brief Loads resources needed by the main menu state.
	 */
	void Load();

	/*!
	 * @brief Initializes the main menu state.
	 */
	void Init();

	/*!
	 * @brief Updates the main menu state logic.
	 */
	void Update();

	/*!
	 * @brief Renders the main menu interface.
	 */
	void Draw();

	/*!
	 * @brief Frees runtime resources used by the main menu state.
	 */
	void Free();

	/*!
	 * @brief Unloads resources used by the main menu state.
	 */
	void Unload();
};