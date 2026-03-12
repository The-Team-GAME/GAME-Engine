//-----------------------------------------------------------------------------------
// file:    Shader.cpp
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   OpenGL shader program implementation for the graphics renderer.
//          Handles shader compilation, linking, and uniform management.
//          Supports vertex and fragment shaders with error logging and
//          resource cleanup for the rendering pipeline.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#include "Shader.h"
#include "../Logger.h"
#include "../FileIO/Helpers.h"

namespace Engine {
  namespace Graphics {

    Shader::Shader(const std::string& vertexSource, const std::string& fragmentSource)
      : program_id(0)
    {
      // Compile shaders
      unsigned int vertex = compile_shader(GL_VERTEX_SHADER, vertexSource);
      unsigned int fragment = compile_shader(GL_FRAGMENT_SHADER, fragmentSource);

      if (vertex == 0 || fragment == 0) {
        Log::error("Failed to compile shaders");
        if (vertex != 0) glDeleteShader(vertex);
        if (fragment != 0) glDeleteShader(fragment);
        return;
      }

      // Link shaders into a program
      if (!link_program(vertex, fragment)) {
        Log::error("Failed to link shader program");
      }

      // Delete shaders as they're linked into our program now and no longer necessary
      glDeleteShader(vertex);
      glDeleteShader(fragment);
    }

    Shader::~Shader() {
      if (program_id != 0) {
        glDeleteProgram(program_id);
        program_id = 0;
      }
    }

    std::unique_ptr<Shader> Shader::load_from_file(const std::string& vertexPath, const std::string& fragmentPath)
    {
      // Read vertex shader source from file
      auto vertexSource = Engine::FileIO::read_string_from_file(vertexPath);
      if (!vertexSource.has_value()) {
        Log::error("Failed to read vertex shader file: {}", vertexPath);
        return std::make_unique<Shader>("", ""); // Return invalid shader
      }

      // Read fragment shader source from file
      auto fragmentSource = Engine::FileIO::read_string_from_file(fragmentPath);
      if (!fragmentSource.has_value()) {
        Log::error("Failed to read fragment shader file: {}", fragmentPath);
        return std::make_unique<Shader>("", ""); // Return invalid shader
      }

      // Create shader using the constructor
      return std::make_unique<Shader>(vertexSource.value(), fragmentSource.value());
    }

    void Shader::use() const {
      if (program_id != 0) {
        glUseProgram(program_id);
      }
    }

    void Shader::set_bool(const std::string& name, bool value) {
      glUniform1i(get_uniform_location(name), (int)value);
    }

    void Shader::set_int(const std::string& name, int value) {
      glUniform1i(get_uniform_location(name), value);
    }

    void Shader::set_float(const std::string& name, float value) {
      glUniform1f(get_uniform_location(name), value);
    }

    void Shader::set_vec2(const std::string& name, const glm::vec2& value) {
      glUniform2fv(get_uniform_location(name), 1, &value[0]);
    }

    void Shader::set_vec2(const std::string& name, float x, float y) {
      glUniform2f(get_uniform_location(name), x, y);
    }

    void Shader::set_vec3(const std::string& name, const glm::vec3& value) {
      glUniform3fv(get_uniform_location(name), 1, &value[0]);
    }

    void Shader::set_vec3(const std::string& name, float x, float y, float z) {
      glUniform3f(get_uniform_location(name), x, y, z);
    }

    void Shader::set_vec4(const std::string& name, const glm::vec4& value) {
      glUniform4fv(get_uniform_location(name), 1, &value[0]);
    }

    void Shader::set_vec4(const std::string& name, float x, float y, float z, float w) {
      glUniform4f(get_uniform_location(name), x, y, z, w);
    }

    void Shader::set_mat2(const std::string& name, const glm::mat2& mat) {
      glUniformMatrix2fv(get_uniform_location(name), 1, GL_FALSE, &mat[0][0]);
    }

    void Shader::set_mat3(const std::string& name, const glm::mat3& mat) {
      glUniformMatrix3fv(get_uniform_location(name), 1, GL_FALSE, &mat[0][0]);
    }

    void Shader::set_mat4(const std::string& name, const glm::mat4& mat) {
      glUniformMatrix4fv(get_uniform_location(name), 1, GL_FALSE, &mat[0][0]);
    }

    int Shader::get_uniform_location(const std::string& name) const {
      // Check cache first
      auto it = uniform_location_cache.find(name);
      if (it != uniform_location_cache.end()) {
        return it->second;
      }

      // Get location from OpenGL
      int location = glGetUniformLocation(program_id, name.c_str());

      if (location == -1) {
        Log::warning("Uniform '{}' not found in shader", name);
      }

      // Cache the location
      uniform_location_cache[name] = location;
      return location;
    }

    unsigned int Shader::compile_shader(unsigned int type, const std::string& source) {
      unsigned int shader = glCreateShader(type);
      const char* src = source.c_str();
      glShaderSource(shader, 1, &src, nullptr);
      glCompileShader(shader);

      // Check for compilation errors
      std::string type_str = (type == GL_VERTEX_SHADER) ? "VERTEX" : "FRAGMENT";
      check_compile_errors(shader, type_str);

      // Check if compilation succeeded
      int success;
      glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
      if (!success) {
        glDeleteShader(shader);
        return 0;
      }

      return shader;
    }

    bool Shader::link_program(unsigned int vertexShader, unsigned int fragmentShader) {
      program_id = glCreateProgram();
      glAttachShader(program_id, vertexShader);
      glAttachShader(program_id, fragmentShader);
      glLinkProgram(program_id);

      // Check for linking errors
      check_compile_errors(program_id, "PROGRAM");

      // Check if linking succeeded
      int success;
      glGetProgramiv(program_id, GL_LINK_STATUS, &success);
      if (!success) {
        glDeleteProgram(program_id);
        program_id = 0;
        return false;
      }

      return true;
    }

    void Shader::check_compile_errors(unsigned int shader, const std::string& type) {
      int success;
      char infoLog[1024];

      if (type != "PROGRAM") {
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success) {
          glGetShaderInfoLog(shader, 1024, nullptr, infoLog);
          Log::error("Shader compilation error ({}):\n{}", type, infoLog);
        }
      }
      else {
        glGetProgramiv(shader, GL_LINK_STATUS, &success);
        if (!success) {
          glGetProgramInfoLog(shader, 1024, nullptr, infoLog);
          Log::error("Shader linking error:\n{}", infoLog);
        }
      }
    }

  } // namespace Graphics
} // namespace Engine