//-----------------------------------------------------------------------------------
// file:    Shader.h
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   OpenGL shader program class definition. Provides shader loading,
//          compilation, and uniform setting functionality. Supports loading
//          from source strings or files, with automatic resource management
//          and error handling for the graphics pipeline.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include "../pch.h"
#include <string>
#include <unordered_map>
#include <memory>
#include <glm/glm.hpp>

namespace Engine {
  namespace Graphics {

    class Shader {
    public:
      // Constructor reads and builds the shader
      Shader(const std::string& vertexSource, const std::string& fragmentSource);

      // Destructor
      ~Shader();

      // Load shader from files
      static std::unique_ptr<Shader> load_from_file(const std::string& vertexPath, const std::string& fragmentPath);

      // Delete copy constructor and assignment operator
      Shader(const Shader&) = delete;
      Shader& operator=(const Shader&) = delete;

      // Use/activate the shader
      void use() const;

      // Utility uniform functions
      void set_bool(const std::string& name, bool value);
      void set_int(const std::string& name, int value);
      void set_float(const std::string& name, float value);
      void set_vec2(const std::string& name, const glm::vec2& value);
      void set_vec2(const std::string& name, float x, float y);
      void set_vec3(const std::string& name, const glm::vec3& value);
      void set_vec3(const std::string& name, float x, float y, float z);
      void set_vec4(const std::string& name, const glm::vec4& value);
      void set_vec4(const std::string& name, float x, float y, float z, float w);
      void set_mat2(const std::string& name, const glm::mat2& mat);
      void set_mat3(const std::string& name, const glm::mat3& mat);
      void set_mat4(const std::string& name, const glm::mat4& mat);

      // Get the program ID
      unsigned int get_id() const { return program_id; }

      // Check if shader is valid
      bool is_valid() const { return program_id != 0; }

    private:
      unsigned int program_id;

      // Cache for uniform locations
      mutable std::unordered_map<std::string, int> uniform_location_cache;

      // Get uniform location (with caching)
      int get_uniform_location(const std::string& name) const;

      // Compile a shader
      unsigned int compile_shader(unsigned int type, const std::string& source);

      // Link the shader program
      bool link_program(unsigned int vertexShader, unsigned int fragmentShader);

      // Check for compilation/linking errors
      void check_compile_errors(unsigned int shader, const std::string& type);
    };

  } // namespace Graphics
} // namespace Engine