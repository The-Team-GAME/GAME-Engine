#include <functional>
namespace Components
{
  struct Clickable
  {
    std::function<void()> on_click;
  };
}