#include "Core.h"
#include <Windows.h>


int main()
{
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

  return 0;
}
