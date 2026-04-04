//-----------------------------------------------------------------------------------
// file:    EngineExport.h
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   DLL export/import macro for the engine. When building the engine EXE
//          (ENGINE_EXPORTS defined), symbols are exported. When building a DLL
//          that links against the engine, symbols are imported.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#ifdef ENGINE_EXPORTS
#define ENGINE_API __declspec(dllexport)
#else
#define ENGINE_API __declspec(dllimport)
#endif

