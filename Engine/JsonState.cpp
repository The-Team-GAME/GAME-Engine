#include "JsonState.h"
#include <iostream>
#include <nlohmann/json.hpp>
#include <fstream>
#include "core.h"

using json = nlohmann::json;

void JsonState::Load()
{
}

void JsonState::Init()
{
  std::ifstream f("Assets/config/example.json");

  json data = json::parse(f);

  std::cout << "Position: " << data["position"] << "\n";
  std::cout << "Velocity: " << data["velocity"] << "\n";
}

void JsonState::Update()
{ 
}

void JsonState::Draw()
{
  Engine::GRAPHICS->clearScreen(0.1f, 0.1f, 0.1f, 1.0f); // Dark gray background
}

void JsonState::Free()
{
}

void JsonState::Unload()
{
}