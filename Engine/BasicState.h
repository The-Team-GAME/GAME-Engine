#pragma once

#include "StateManager.h" 
#include "TypeDef.h"
class BasicState : public Engine::GameState<BasicState>
{
private:
  s32 fontId1 = -1;
  s32 fontId2 = -1;
  s32 moveSoundId = -1;
  f32 rotation = 0.f;

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