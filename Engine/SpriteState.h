#pragma once

#include "StateManager.h" 

struct Sprite {
	bool isAnimated = false;
	int frames = 0;
	int speed = 100;
	int currentFrame = 0;
	float animationTimer = 0.0f;
	int columns = 1;
	int rows = 1;
	int width = 0;
	int height = 0;
};

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
	 * @brief Renders any miscellanous objects.
	 */
	void Draw();

	/*!
	 * @brief Renders the sprite manager interface.
	 */
	void DrawManager();

	/*!
	 * @brief Frees runtime resources used by the main menu state.
	 */
	void Free();

	/*!
	 * @brief Unloads resources used by the main menu state.
	 */
	void Unload();
};