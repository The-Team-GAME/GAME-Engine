//-----------------------------------------------------------------------------------
// file:    Precompiled.cpp
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   This file implements the precompiled header by defining necessary
//          implementation macros for third-party libraries such as STB image.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#pragma warning(disable: 4996)
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"