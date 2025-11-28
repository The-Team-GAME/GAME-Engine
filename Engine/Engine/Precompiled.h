//-----------------------------------------------------------------------------------
// file:    Precompiled.h
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   This file serves as the precompiled header, including commonly used
//          third-party library headers such as OpenGL, GLFW, GLM, FMOD, and
//          FreeType to improve compilation times.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <stb_image.h>

#include <glm/glm.hpp> // TODO: make sure to not allow Game Devs to include this directly
#include <glm/gtc/matrix_transform.hpp>
#include <glm/fwd.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <fmod.hpp>

#include <ft2build.h>
#include FT_FREETYPE_H

#include <imgui.h>
#include <imgui_internal.h>