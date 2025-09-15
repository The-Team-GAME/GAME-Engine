#include "Core.h"
#include <Windows.h>


INT WINAPI WinMain(
  _In_ HINSTANCE hInst,
  _In_opt_ HINSTANCE hPrevInstance,
  _In_ LPSTR lpCmdLine,
  _In_ INT nCmdShow)
{
  UNREFERENCED_PARAMETER(hInst);
  UNREFERENCED_PARAMETER(hPrevInstance);
  UNREFERENCED_PARAMETER(lpCmdLine);
  UNREFERENCED_PARAMETER(nCmdShow);

  AllocConsole();

  // Redirect stdout to the console
  FILE* fp;
  if (freopen_s(&fp, "CONOUT$", "w", stdout) != 0) {
    // Handle error if redirection fails
    MessageBoxA(NULL, "Failed to redirect stdout", "Error", MB_OK);
  }

  // Enable memory leak detection
  int tmpDbgFlag = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);
  tmpDbgFlag |= _CRTDBG_LEAK_CHECK_DF;
  _CrtSetDbgFlag(tmpDbgFlag);
  _CrtSetBreakAlloc(-1);

  // Start the engine
  Engine::Core* engine = new Engine::Core();
  engine->run();

  // Cleanup
  delete engine;
  FreeConsole();

  return 0;
}
