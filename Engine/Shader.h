#pragma once
#include <GL/glew.h>
#include <glm/fwd.hpp>

class Shader {
private:
  GLuint m_id;

public:
  Shader(const char* vertexPath, const char* fragmentPath);
  ~Shader();

  Shader(Shader const& other) = delete;
  Shader(Shader&& other) = delete;
  Shader& operator=(Shader const& other) = delete;
  Shader& operator=(Shader&& other) = delete;


  GLuint getId() const;

  void use() const;

  // utility uniform functions
  void setBool(const std::string& name, bool value) const;
  void setInt(const std::string& name, int value) const;
  void setFloat(const std::string& name, float value) const;
  void setVec3(const std::string& name, glm::vec3 const& list) const;
  void setVec3(const std::string& name, float x, float y, float z) const;
  void setMatrix(const std::string& name, const glm::mat4x4& matrix, bool transpose = false) const;
};