#include "Core.h"
#include <Windows.h>


INT WINAPI WinMain(
  _In_ HINSTANCE hInst,
  _In_opt_ HINSTANCE hPrevInstance,
  _In_ LPSTR lpCmdLine,
  _In_ INT nCmdShow)
{
  int tmpDbgFlag = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);
  tmpDbgFlag |= _CRTDBG_LEAK_CHECK_DF;
  _CrtSetDbgFlag(tmpDbgFlag);

  _CrtSetBreakAlloc(-1);

  Engine::Core* engine = new Engine::Core();

  engine->init();
  engine->run();
  engine->exit();

  return 0;
}

#pragma warning(pop)