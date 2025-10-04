#pragma once

#include "../Precompiled.h"
#include <string>
#include <unordered_map>
#include <glm/glm.hpp>

namespace Engine {
  namespace Graphics {

    class Shader {
    public:
      // Constructor reads and builds the shader
      Shader(const std::string& vertexSource, const std::string& fragmentSource);

      // Destructor
      ~Shader();

      // Delete copy constructor and assignment operator
      Shader(const Shader&) = delete;
      Shader& operator=(const Shader&) = delete;

      // Use/activate the shader
      void use() const;

      // Utility uniform functions
      void setBool(const std::string& name, bool value);
      void setInt(const std::string& name, int value);
      void setFloat(const std::string& name, float value);
      void setVec2(const std::string& name, const glm::vec2& value);
      void setVec2(const std::string& name, float x, float y);
      void setVec3(const std::string& name, const glm::vec3& value);
      void setVec3(const std::string& name, float x, float y, float z);
      void setVec4(const std::string& name, const glm::vec4& value);
      void setVec4(const std::string& name, float x, float y, float z, float w);
      void setMat2(const std::string& name, const glm::mat2& mat);
      void setMat3(const std::string& name, const glm::mat3& mat);
      void setMat4(const std::string& name, const glm::mat4& mat);

      // Get the program ID
      unsigned int getID() const { return program_id; }

      // Check if shader is valid
      bool isValid() const { return program_id != 0; }

    private:
      unsigned int program_id;

      // Cache for uniform locations
      mutable std::unordered_map<std::string, int> uniform_location_cache;

      // Get uniform location (with caching)
      int getUniformLocation(const std::string& name) const;

      // Compile a shader
      unsigned int compileShader(unsigned int type, const std::string& source);

      // Link the shader program
      bool linkProgram(unsigned int vertexShader, unsigned int fragmentShader);

      // Check for compilation/linking errors
      void checkCompileErrors(unsigned int shader, const std::string& type);
    };

  } // namespace Graphics
} // namespace Engine