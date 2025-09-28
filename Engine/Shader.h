#pragma once

#include "Precompiled.h"
#include <string>

class Shader {
private:
  GLuint m_id;

public:
  Shader() = default;


  Shader(Shader const& other) = delete;
  Shader(Shader&& other) = delete;
  Shader& operator=(Shader const& other) = delete;
  Shader& operator=(Shader&& other) = delete;



  void init(const char* vertexPath, const char* fragmentPath);
  void destroy();
  void use() const;
  GLuint getId() const;

  // utility uniform functions
  void setBool(const std::string& name, bool value) const;
  void setInt(const std::string& name, int value) const;
  void setFloat(const std::string& name, float value) const;
  void setVec3(const std::string& name, glm::vec3 const& list) const;
  void setVec3(const std::string& name, float x, float y, float z) const;
  void setVec4(const std::string& name, glm::vec4 const& list) const;
  void setVec4(const std::string& name, float a, float b, float c, float d) const;
  void setMatrix(const std::string& name, const glm::mat4x4& matrix, bool transpose = false) const;
  void setIntArray(const std::string& name, int count, const int* value) const;
};