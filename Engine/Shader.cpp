#include <string>
#include <sstream>
#include <fstream>
#include <iostream> 

#include <gl/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Shader.h"

void Shader::init(const char* vertexPath, const char* fragmentPath)
{
  // 1. retrieve the vertex/fragment source code from filePath
  std::string vertexCode;
  std::string fragmentCode;
  std::ifstream vShaderFile;
  std::ifstream fShaderFile;

  // ensure ifstream objects can throw exceptions:
  vShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
  fShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);

  // open files
  vShaderFile.open(vertexPath);
  fShaderFile.open(fragmentPath);

  std::stringstream vShaderStream, fShaderStream;

  // read file's buffer contents into streams
  vShaderStream << vShaderFile.rdbuf();
  fShaderStream << fShaderFile.rdbuf();

  // convert stream into string
  vertexCode = vShaderStream.str();
  fragmentCode = fShaderStream.str();

  const char* vShaderCode = vertexCode.c_str();
  const char* fShaderCode = fragmentCode.c_str();

  // 2. compile shaders
  int success;

  // vertex Shader
  GLuint vertex = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(vertex, 1, &vShaderCode, nullptr);
  glCompileShader(vertex);

  // print compile errors if any
  glGetShaderiv(vertex, GL_COMPILE_STATUS, &success);

  if (!success) {
    char infoLog[512];
    glGetShaderInfoLog(vertex, 512, nullptr, infoLog);
    std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << '\n';
  }

  GLuint fragment = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(fragment, 1, &fShaderCode, nullptr);
  glCompileShader(fragment);

  if (!success) {
    char infoLog[512];
    glGetShaderInfoLog(fragment, 512, nullptr, infoLog);
    std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << '\n';
  }

  m_id = glCreateProgram();
  glAttachShader(m_id, vertex);
  glAttachShader(m_id, fragment);
  glLinkProgram(m_id);

  // print linking errors if any
  glGetProgramiv(m_id, GL_LINK_STATUS, &success);
  if (!success) {
    char infoLog[512];
    glGetShaderInfoLog(fragment, 512, nullptr, infoLog);
    std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << '\n';
  }

  // delete shaders; they're linked into our program and no longer necessary
  glDeleteShader(vertex);
  glDeleteShader(fragment);
}

void Shader::destroy()
{
  glDeleteProgram(m_id);
}

void Shader::use() const {
  glUseProgram(m_id);
}

GLuint Shader::getId() const {
  return m_id;
}


void Shader::setBool(const std::string& name, bool const value) const {
  glUniform1i(glGetUniformLocation(m_id, name.c_str()), static_cast<int>(value));
}

void Shader::setInt(const std::string& name, int const value) const {
  glUniform1i(glGetUniformLocation(m_id, name.c_str()), value);
}

void Shader::setFloat(const std::string& name, float const value) const {
  glUniform1f(glGetUniformLocation(m_id, name.c_str()), value);
}

void Shader::setMatrix(const std::string& name, const glm::mat4x4& matrix, bool transpose) const {
  glUniformMatrix4fv(glGetUniformLocation(m_id, name.c_str()), 1, transpose, glm::value_ptr(matrix));
}

void Shader::setVec3(const std::string& name, glm::vec3 const& list) const {
  glUniform3f(glGetUniformLocation(m_id, name.c_str()), list[0], list[1], list[2]);
}

void Shader::setVec3(const std::string& name, float x, float y, float z) const {
  glUniform3f(glGetUniformLocation(m_id, name.c_str()), x, y, z);
}

void Shader::setVec4(const std::string& name, glm::vec4 const& list) const {
  glUniform4f(glGetUniformLocation(m_id, name.c_str()), list[0], list[1], list[2], list[3]);
}

void Shader::setVec4(const std::string& name, float a, float b, float c, float d) const {
  glUniform4f(glGetUniformLocation(m_id, name.c_str()), a, b, c, d);
}

void Shader::setIntArray(const std::string& name, int count, const int* value) const {
  glUniform1iv(glGetUniformLocation(m_id, name.c_str()), count, value);
}