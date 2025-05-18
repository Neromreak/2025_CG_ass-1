#include "application_solar.hpp"
#include "window_handler.hpp"

#include "utils.hpp"
#include "shader_loader.hpp"
#include "model_loader.hpp"

#include <glbinding/gl/gl.h>
// Use gl definitions from glbinding 
using namespace gl;

// Dont load gl bindings from glfw
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>

ApplicationSolar::ApplicationSolar(std::string const& resource_path)
 :Application{resource_path}
 ,planet_object{}
 ,m_view_transform{glm::translate(glm::fmat4{}, glm::fvec3{0.0f, 0.0f, 4.0f})}
 ,m_view_projection{utils::calculate_projection_matrix(initial_aspect_ratio)}
{
  initializeGeometry();
  initializeShaderPrograms();
}

ApplicationSolar::~ApplicationSolar() {
  glDeleteBuffers(1, &planet_object.vertex_BO);
  glDeleteBuffers(1, &planet_object.element_BO);
  glDeleteVertexArrays(1, &planet_object.vertex_AO);
}

void ApplicationSolar::render() const {
  // Bind shader to upload uniforms
  glUseProgram(m_shaders.at("planet").handle);

  // Add translation matrix with rotation and then translation
  glm::fmat4 model_matrix = glm::rotate(glm::fmat4{}, float(glfwGetTime()) * 2, glm::fvec3{1.0f, 1.0f, 1.0f});
  model_matrix = glm::translate(model_matrix, glm::fvec3{0.0f, 0.0f, -1.0f});

  glUniformMatrix4fv(m_shaders.at("planet").u_locs.at("ModelMatrix"),
                     1, GL_FALSE, glm::value_ptr(model_matrix));

  // Extra matrix for normal transformation to keep them orthogonal to surface
  glm::fmat4 normal_matrix = glm::inverseTranspose(glm::inverse(m_view_transform) * model_matrix);
  glUniformMatrix4fv(m_shaders.at("planet").u_locs.at("NormalMatrix"),
                     1, GL_FALSE, glm::value_ptr(normal_matrix));

  // Bind the VAO to draw
  glBindVertexArray(planet_object.vertex_AO);

  // Draw bound vertex array using bound shader
  glDrawElements(planet_object.draw_mode, planet_object.num_elements, model::INDEX.type, NULL);

  ///////////////////// TEST

  // Bind shader to upload uniforms
  glUseProgram(m_shaders.at("planet").handle);

  // Add translation matrix with rotation and then translation
  model_matrix = glm::rotate(glm::fmat4{}, float(glfwGetTime()) * 1, glm::fvec3{ 1.0f, 1.0f, 1.0f });
  model_matrix = glm::translate(model_matrix, glm::fvec3{ 0.0f, 0.0f, -1.0f });

  glUniformMatrix4fv(m_shaders.at("planet").u_locs.at("ModelMatrix"),
    1, GL_FALSE, glm::value_ptr(model_matrix));

  // Extra matrix for normal transformation to keep them orthogonal to surface
  normal_matrix = glm::inverseTranspose(glm::inverse(m_view_transform) * model_matrix);
  glUniformMatrix4fv(m_shaders.at("planet").u_locs.at("NormalMatrix"),
    1, GL_FALSE, glm::value_ptr(normal_matrix));

  // Bind the VAO to draw
  glBindVertexArray(planet_object.vertex_AO);

  // Draw bound vertex array using bound shader
  glDrawElements(planet_object.draw_mode, planet_object.num_elements, model::INDEX.type, NULL);
}

void ApplicationSolar::uploadView() {
  // Vertices are transformed in camera space, so camera transform must be inverted
  glm::fmat4 view_matrix = glm::inverse(m_view_transform);
  // Upload matrix to gpu
  glUniformMatrix4fv(m_shaders.at("planet").u_locs.at("ViewMatrix"),
                     1, GL_FALSE, glm::value_ptr(view_matrix));
}

void ApplicationSolar::uploadProjection() {
  // upload matrix to gpu
  glUniformMatrix4fv(m_shaders.at("planet").u_locs.at("ProjectionMatrix"),
                     1, GL_FALSE, glm::value_ptr(m_view_projection));
}

// Update uniform locations
void ApplicationSolar::uploadUniforms() { 
  // bind shader to which to upload unforms
  glUseProgram(m_shaders.at("planet").handle);
  // upload uniform values to new locations
  uploadView();
  uploadProjection();
}

///////////////////////////// Intialisation Functions /////////////////////////
// Load shader sources
void ApplicationSolar::initializeShaderPrograms() {
  // Store shader program objects in container
  m_shaders.emplace("planet", shader_program{{{GL_VERTEX_SHADER,m_resource_path + "shaders/simple.vert"},
                                           {GL_FRAGMENT_SHADER, m_resource_path + "shaders/simple.frag"}}});
  // Request uniform locations for shader program
  m_shaders.at("planet").u_locs["NormalMatrix"] = -1;
  m_shaders.at("planet").u_locs["ModelMatrix"] = -1;
  m_shaders.at("planet").u_locs["ViewMatrix"] = -1;
  m_shaders.at("planet").u_locs["ProjectionMatrix"] = -1;
}

// Load models
void ApplicationSolar::initializeGeometry()
{
  model planet_model = model_loader::obj(m_resource_path + "models/sphere.obj", model::NORMAL);

  // Generate vertex array object
  glGenVertexArrays(1, &planet_object.vertex_AO);
  // Bind the array for attaching buffers
  glBindVertexArray(planet_object.vertex_AO);

  // Generate generic buffer
  glGenBuffers(1, &planet_object.vertex_BO);
  // Bind this as an vertex array buffer containing all attributes
  glBindBuffer(GL_ARRAY_BUFFER, planet_object.vertex_BO);
  // Configure currently bound array buffer
  glBufferData(GL_ARRAY_BUFFER, sizeof(float) * planet_model.data.size(), planet_model.data.data(), GL_STATIC_DRAW);

  // Activate first attribute on GPU
  glEnableVertexAttribArray(0);
  // First attribute is 3 floats with no offset & stride
  glVertexAttribPointer(0, model::POSITION.components, model::POSITION.type, GL_FALSE, planet_model.vertex_bytes, planet_model.offsets[model::POSITION]);
  // Activate second attribute on GPU
  glEnableVertexAttribArray(1);
  // Second attribute is 3 floats with no offset & stride
  glVertexAttribPointer(1, model::NORMAL.components, model::NORMAL.type, GL_FALSE, planet_model.vertex_bytes, planet_model.offsets[model::NORMAL]);

  // Generate generic buffer
  glGenBuffers(1, &planet_object.element_BO);
  // Bind this as an vertex array buffer containing all attributes
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, planet_object.element_BO);
  // Configure currently bound array buffer
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, model::INDEX.size * planet_model.indices.size(), planet_model.indices.data(), GL_STATIC_DRAW);

  // Store type of primitive to draw
  planet_object.draw_mode = GL_TRIANGLES;
  // Transfer number of indices to model object 
  planet_object.num_elements = GLsizei(planet_model.indices.size());
}

///////////////////////////// Callback Functions for Window Events ////////////
// Handle key input
void ApplicationSolar::keyCallback(int key, int action, int mods) {
  if (key == GLFW_KEY_W  && (action == GLFW_PRESS || action == GLFW_REPEAT)) {
    m_view_transform = glm::translate(m_view_transform, glm::fvec3{0.0f, 0.0f, -0.1f});
    uploadView();
  }
  else if (key == GLFW_KEY_S  && (action == GLFW_PRESS || action == GLFW_REPEAT)) {
    m_view_transform = glm::translate(m_view_transform, glm::fvec3{0.0f, 0.0f, 0.1f});
    uploadView();
  }
}

// Handle delta mouse movement input
void ApplicationSolar::mouseCallback(double pos_x, double pos_y) {
  // Mouse handling
}

// Handle resizing
void ApplicationSolar::resizeCallback(unsigned width, unsigned height) {
  // Recalculate projection matrix for new aspect ration
  m_view_projection = utils::calculate_projection_matrix(float(width) / float(height));
  // Upload new projection matrix
  uploadProjection();
}


// .exe entry point
int main(int argc, char* argv[]) {
  Application::run<ApplicationSolar>(argc, argv, 3, 2);
}