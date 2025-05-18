#ifndef STRUCTS_HPP
#define STRUCTS_HPP

#include <map>
#include <glbinding/gl/gl.h>
// Use gl definitions from glbinding 
using namespace gl;

// GPU representation of model
struct model_object {
  // Vertex array object
  GLuint vertex_AO = 0;
  // Vertex buffer object
  GLuint vertex_BO = 0;
  // Index buffer object
  GLuint element_BO = 0;
  // Primitive type to draw
  GLenum draw_mode = GL_NONE;
  // Indices number, if EBO exists
  GLsizei num_elements = 0;
};

// GPU representation of texture
struct texture_object {
  // Handle of texture object
  GLuint handle = 0;
  // Binding point
  GLenum target = GL_NONE;
};

// Shader handle and uniform storage
struct shader_program {
  shader_program(std::map<GLenum, std::string> paths)
   :shader_paths{paths}
   ,handle{0}
   {}

  // Paths to shader sources
  std::map<GLenum, std::string> shader_paths;
  // Object handle
  GLuint handle;
  // Uniform locations mapped to name
  std::map<std::string, GLint> u_locs{};
};
#endif