#include "Core.h"
#include "Logger.h"
#include <Windows.h>
#include <exception>
#include <stdexcept>


int main()
{
  // Enable memory leak detection
  int tmpDbgFlag = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);
  tmpDbgFlag |= _CRTDBG_LEAK_CHECK_DF;
  _CrtSetDbgFlag(tmpDbgFlag);
  _CrtSetBreakAlloc(-1);

  try
  {
    // Start the engine
    Engine::Core engine{};
    engine.run();
  }
  catch (const std::exception& e)
  {
    Log::crash(std::string("Unhandled exception in main: ") + e.what());
    return 1;
  }
  catch (...)
  {
    Log::crash("Unknown unhandled exception in main");
    return 1;
  }

  return 0;
}
