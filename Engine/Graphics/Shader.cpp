#include "Shader.h"
#include "../Logger.h"

namespace Engine {
  namespace Graphics {

    Shader::Shader(const std::string& vertexSource, const std::string& fragmentSource)
      : program_id(0)
    {
      // Compile shaders
      unsigned int vertex = compileShader(GL_VERTEX_SHADER, vertexSource);
      unsigned int fragment = compileShader(GL_FRAGMENT_SHADER, fragmentSource);

      if (vertex == 0 || fragment == 0) {
        Log::error("Failed to compile shaders");
        if (vertex != 0) glDeleteShader(vertex);
        if (fragment != 0) glDeleteShader(fragment);
        return;
      }

      // Link shaders into a program
      if (!linkProgram(vertex, fragment)) {
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

    void Shader::use() const {
      if (program_id != 0) {
        glUseProgram(program_id);
      }
    }

    void Shader::setBool(const std::string& name, bool value) {
      glUniform1i(getUniformLocation(name), (int)value);
    }

    void Shader::setInt(const std::string& name, int value) {
      glUniform1i(getUniformLocation(name), value);
    }

    void Shader::setFloat(const std::string& name, float value) {
      glUniform1f(getUniformLocation(name), value);
    }

    void Shader::setVec2(const std::string& name, const glm::vec2& value) {
      glUniform2fv(getUniformLocation(name), 1, &value[0]);
    }

    void Shader::setVec2(const std::string& name, float x, float y) {
      glUniform2f(getUniformLocation(name), x, y);
    }

    void Shader::setVec3(const std::string& name, const glm::vec3& value) {
      glUniform3fv(getUniformLocation(name), 1, &value[0]);
    }

    void Shader::setVec3(const std::string& name, float x, float y, float z) {
      glUniform3f(getUniformLocation(name), x, y, z);
    }

    void Shader::setVec4(const std::string& name, const glm::vec4& value) {
      glUniform4fv(getUniformLocation(name), 1, &value[0]);
    }

    void Shader::setVec4(const std::string& name, float x, float y, float z, float w) {
      glUniform4f(getUniformLocation(name), x, y, z, w);
    }

    void Shader::setMat2(const std::string& name, const glm::mat2& mat) {
      glUniformMatrix2fv(getUniformLocation(name), 1, GL_FALSE, &mat[0][0]);
    }

    void Shader::setMat3(const std::string& name, const glm::mat3& mat) {
      glUniformMatrix3fv(getUniformLocation(name), 1, GL_FALSE, &mat[0][0]);
    }

    void Shader::setMat4(const std::string& name, const glm::mat4& mat) {
      glUniformMatrix4fv(getUniformLocation(name), 1, GL_FALSE, &mat[0][0]);
    }

    int Shader::getUniformLocation(const std::string& name) const {
      // Check cache first
      auto it = uniform_location_cache.find(name);
      if (it != uniform_location_cache.end()) {
        return it->second;
      }

      // Get location from OpenGL
      int location = glGetUniformLocation(program_id, name.c_str());

      if (location == -1) {
        Log::warning("Uniform '" + name + "' not found in shader");
      }

      // Cache the location
      uniform_location_cache[name] = location;
      return location;
    }

    unsigned int Shader::compileShader(unsigned int type, const std::string& source) {
      unsigned int shader = glCreateShader(type);
      const char* src = source.c_str();
      glShaderSource(shader, 1, &src, nullptr);
      glCompileShader(shader);

      // Check for compilation errors
      std::string type_str = (type == GL_VERTEX_SHADER) ? "VERTEX" : "FRAGMENT";
      checkCompileErrors(shader, type_str);

      // Check if compilation succeeded
      int success;
      glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
      if (!success) {
        glDeleteShader(shader);
        return 0;
      }

      return shader;
    }

    bool Shader::linkProgram(unsigned int vertexShader, unsigned int fragmentShader) {
      program_id = glCreateProgram();
      glAttachShader(program_id, vertexShader);
      glAttachShader(program_id, fragmentShader);
      glLinkProgram(program_id);

      // Check for linking errors
      checkCompileErrors(program_id, "PROGRAM");

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

    void Shader::checkCompileErrors(unsigned int shader, const std::string& type) {
      int success;
      char infoLog[1024];

      if (type != "PROGRAM") {
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success) {
          glGetShaderInfoLog(shader, 1024, nullptr, infoLog);
          Log::error("Shader compilation error (" + type + "):\n" + std::string(infoLog));
        }
      }
      else {
        glGetProgramiv(shader, GL_LINK_STATUS, &success);
        if (!success) {
          glGetProgramInfoLog(shader, 1024, nullptr, infoLog);
          Log::error("Shader linking error:\n" + std::string(infoLog));
        }
      }
    }

  } // namespace Graphics
} // namespace Engine