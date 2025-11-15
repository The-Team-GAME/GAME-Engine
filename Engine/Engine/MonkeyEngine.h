//-----------------------------------------------------------------------------------
// file:    MonkeyEngine.h
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   Main header file that includes all core engine systems and provides
//          the primary interface for game development. This is the central include
//          file that brings together ECS, graphics, audio, input, resources,
//          reflection, and all other engine subsystems into a unified API.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

// TODO: Documentation

#include "Precompiled.h"
#include "Core.h"
#include "ECS/ECS.h"
#include "Graphics/Renderer.h"
#include "StateManager.h"
#include "SceneManager.h"
#include "TypeDef.h"
#include "InputManager.h"
#include "Time.h"
#include "Perf.h"
#include "Audio/AudioPlayer.h"
#include "Resources.h"
#include "Components/Components.h"
#include "WindowManager.h"
#include "Logger.h"
#include "Math/Vector2D.h"
#include "Math/Vector3D.h"
#include "Math/Vector4D.h"
#include "FileHandler.h"
#include "Reflection/ReflectionLibrary.h"
#include "ImGuiManager.h"
#include "EventManager/EventManager.h"
#include "FileIO/PrefabsFileIO.h"
#include "Systems/PhysicsSystem.h"
#include "Lua/LuaManager.h"
#include "FileIO/Settings.h" // Remove in the future, we should not expose this directly
#include "Level.h"
#include "FileIO/LevelFileIO.h"