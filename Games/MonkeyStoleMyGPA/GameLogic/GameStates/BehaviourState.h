//-----------------------------------------------------------------------------------
// file:    BehaviourState.h
// project: Monkey Stole My GPA
//
// author:  How Hong Chin Ryan (100%)
//          Primary author
//
// email:   hongchinryan.how@digipen.edu
//
// brief:   Declares the BehaviourState scene for the game engine. Handles resource
//          management, scene lifecycle, and cleanup of physics-related entities.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once


#include "pch.h"

#include "ShowcaseState.h"

class BehaviourState : public Engine::Scene
{
private:

public:
  void load() {
  };

  void init() {};

  void update() {
  };

  void draw() {
    Engine::Graphics::clear_screen({ 0.2f, 0.2f, 0.2f, 1.0f });
  };

  void free() {
  };

  void unload() {
    Engine::ECS::destroy_entities_with<Components::Tag<"BehaviourState">>();

  };
};