//-----------------------------------------------------------------------------------
// file:    Shader.h
// project: Monkey Stole My GPA
//
// author:  Ong Zi Ming (100%)
//          Primary author
//
// email:   o.ziming@digipen.edu
//
// brief:   This file defines the Shader class which handles OpenGL shader program
//          compilation, linking, and uniform variable management for vertex and
//          fragment shaders.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include "Precompiled.h"
#include <string>
#include "TypeDef.h"

class Shader {
private:
  GLuint m_id;

public:
  Shader() = default;

  // Delete copy and move operations for RAII safety
  Shader(Shader const& other) = delete;
  Shader(Shader&& other) = delete;
  Shader& operator=(Shader const& other) = delete;
  Shader& operator=(Shader&& other) = delete;

  /* ===================== //
  // Initialization        //
  // ===================== */
  /*=================================================================================
  Function Brief:
  Initializes the shader by loading, compiling, and linking vertex and fragment
  shader files into an OpenGL shader program.

  Parameters:
  vertexPath - File path to the vertex shader source code
  fragmentPath - File path to the fragment shader source code
  ==================================================================================*/
  void init(const char* vertexPath, const char* fragmentPath);

  /*=================================================================================
  Function Brief:
  Destroys the shader program and frees OpenGL resources.

  Where to call it:
  During cleanup or when the shader is no longer needed.
  ==================================================================================*/
  void destroy();

  /*=================================================================================
  Function Brief:
  Activates this shader program for use in the OpenGL rendering pipeline.

  Where to call it:
  Before drawing operations that require this shader.
  ==================================================================================*/
  void use() const;

  /*=================================================================================
  Function Brief:
  Returns the OpenGL program ID for this shader.

  Returns:
  The GLuint program ID.
  ==================================================================================*/
  GLuint getId() const;

  /* ===================== //
  // Uniform Setters       //
  // ===================== */
  void set_bool(const std::string& name, bool value) const;
  void set_int(const std::string& name, s32 value) const;
  void set_float(const std::string& name, f32 value) const;
  void set_vec3(const std::string& name, glm::vec3 const& list) const;
  void set_vec3(const std::string& name, f32 x, f32 y, f32 z) const;
  void set_vec4(const std::string& name, glm::vec4 const& list) const;
  void set_vec4(const std::string& name, f32 a, f32 b, f32 c, f32 d) const;
  void set_matrix(const std::string& name, const glm::mat4x4& matrix, bool transpose = false) const;
  void set_int_array(const std::string& name, s32 count, const int* value) const;
};