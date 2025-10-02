#pragma once

#include "Precompiled.h"
#include <string>
#include "TypeDef.h"

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