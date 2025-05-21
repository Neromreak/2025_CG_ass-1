#include "application_solar.hpp"
#include "window_handler.hpp"

#include "utils.hpp"
#include "shader_loader.hpp"
#include "model_loader.hpp"
#include "scene_graph.hpp"
#include "geometry_node.hpp"
#include "camera_node.hpp"
#include "point_light_node.hpp"
#include "node.hpp"

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
 ,m_view_transform{glm::translate(glm::fmat4{}, glm::fvec3{0.0f, 0.0f, 30.0f})}
 ,m_view_projection{utils::calculate_projection_matrix(initial_aspect_ratio)}
{
  initializeGeometry();
  initializeShaderPrograms();
  initializeScene();
}

ApplicationSolar::~ApplicationSolar() {
  glDeleteBuffers(1, &planet_object.vertex_BO);
  glDeleteBuffers(1, &planet_object.element_BO);
  glDeleteVertexArrays(1, &planet_object.vertex_AO);
}

void ApplicationSolar::render() const {
  // Traverse the scene graph tree
  scene->get_root()->render(&m_shaders, &m_view_transform, scene->get_root()->get_world_transform());
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
void ApplicationSolar::initializeShaderPrograms()
{
  // Store shader program objects in container
  m_shaders.emplace("planet", shader_program{{{GL_VERTEX_SHADER,m_resource_path + "shaders/simple.vert"},
                                           {GL_FRAGMENT_SHADER, m_resource_path + "shaders/simple.frag"}}});
  // Request uniform locations for shader program
  m_shaders.at("planet").u_locs["NormalMatrix"] = -1;
  m_shaders.at("planet").u_locs["ModelMatrix"] = -1;
  m_shaders.at("planet").u_locs["ViewMatrix"] = -1;
  m_shaders.at("planet").u_locs["ProjectionMatrix"] = -1;
}

// Load models (the raw model files containing the vertices information)
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

// Create and fill scene with nodes (camera, objects, lights)
void ApplicationSolar::initializeScene()
{
  // Create scene graph and root
  scene = SceneGraph::get_instance();
  Node* root = new Node{ "root", nullptr, glm::fmat4{}, glm::fmat4{}, 0.0f };
  scene->set_root(root);
  
  // Add planet holders to scene root
  glm::fmat4 local_transform = glm::translate(glm::fmat4{}, glm::vec3{ 0.0f, 0.0f, 6.0f });
  Node* holder_mer = new Node{ "Mercury Holder", root, local_transform, glm::fmat4{}, 1.0f };
  
  local_transform = glm::translate(glm::fmat4{}, glm::vec3{ 0.0f, 0.0f, 12.0f });
  Node* holder_ven = new Node{ "Venus Holder", root, local_transform, glm::fmat4{}, 0.8f };

  local_transform = glm::translate(glm::fmat4{}, glm::vec3{ 0.0f, 0.0f, 18.0f });
  Node* holder_ear = new Node{ "Earth Holder", root, local_transform, glm::fmat4{}, 0.6f };
  
  local_transform = glm::translate(glm::fmat4{}, glm::vec3{0.0f, 0.0f, 2.0f});
  Node* holder_moo = new Node{ "Moon Holder", holder_ear, local_transform, glm::fmat4{}, 1.2f };

  local_transform = glm::translate(glm::fmat4{}, glm::vec3{ 0.0f, 0.0f, 24.0f });
  Node* holder_mar = new Node{ "Mars Holder", root, local_transform, glm::fmat4{}, 0.4f };

  local_transform = glm::translate(glm::fmat4{}, glm::vec3{ 0.0f, 0.0f, 34.0f });
  Node* holder_jup = new Node{ "Jupiter Holder", root, local_transform, glm::fmat4{}, 0.2f };

  local_transform = glm::translate(glm::fmat4{}, glm::vec3{ 0.0f, 0.0f, 44.0f });
  Node* holder_sat = new Node{ "Saturn Holder", root, local_transform, glm::fmat4{}, 0.1f };

  local_transform = glm::translate(glm::fmat4{}, glm::vec3{ 0.0f, 0.0f, 54.0f });
  Node* holder_ura = new Node{ "Uranus Holder", root, local_transform, glm::fmat4{}, 0.05f };

  local_transform = glm::translate(glm::fmat4{}, glm::vec3{ 0.0f, 0.0f, 64.0f });
  Node* holder_nep = new Node{ "Neptune Holder", root, local_transform, glm::fmat4{}, 0.03f };
  
  // Add planets to planet holders
  local_transform = glm::scale(glm::fmat4{}, glm::vec3{ 0.7f });
  GeometryNode* mer = new GeometryNode{ "Mercury", holder_mer, local_transform, glm::fmat4{}, &planet_object};

  local_transform = glm::scale(glm::fmat4{}, glm::vec3{ 0.9f });
  GeometryNode* ven = new GeometryNode{ "Venus", holder_ven, local_transform, glm::fmat4{}, &planet_object};

  local_transform = glm::scale(glm::fmat4{}, glm::vec3{ 1.0f });
  GeometryNode* ear = new GeometryNode{ "Earth", holder_ear, local_transform, glm::fmat4{}, &planet_object};
  local_transform = glm::scale(glm::fmat4{}, glm::vec3{ 0.5f });
  GeometryNode* moo = new GeometryNode{ "Moon", holder_moo, local_transform, glm::fmat4{}, &planet_object};

  local_transform = glm::scale(glm::fmat4{}, glm::vec3{ 0.9f });
  GeometryNode* mar = new GeometryNode{ "Mars", holder_mar, local_transform, glm::fmat4{}, &planet_object};

  local_transform = glm::scale(glm::fmat4{}, glm::vec3{ 2.0f });
  GeometryNode* jup = new GeometryNode{ "Jupiter", holder_jup, local_transform, glm::fmat4{}, &planet_object};

  local_transform = glm::scale(glm::fmat4{}, glm::vec3{ 1.8f });
  GeometryNode* sat = new GeometryNode{ "Saturn", holder_sat, local_transform, glm::fmat4{}, &planet_object};

  local_transform = glm::scale(glm::fmat4{}, glm::vec3{ 1.4f });
  GeometryNode* ura = new GeometryNode{ "Uranus", holder_ura, local_transform, glm::fmat4{}, &planet_object};

  local_transform = glm::scale(glm::fmat4{}, glm::vec3{ 1.3f });
  GeometryNode* nep = new GeometryNode{ "Neptune", holder_nep, local_transform, glm::fmat4{}, &planet_object};

  // Add lighting and sun
  local_transform = glm::translate(glm::fmat4{}, glm::vec3{ 0.0f, 0.0f, 0.0f });
  PointLightNode* light_sun = new PointLightNode{ "Sun light", root, local_transform, glm::fmat4{}, 0.0f, glm::vec3{}, 1.0f };
  local_transform = glm::scale(glm::fmat4{}, glm::vec3{3.0f});
  GeometryNode* sun = new GeometryNode{ "Sun", light_sun, local_transform, glm::fmat4{}, &planet_object};

  // Add camera
  CameraNode* cam_main = new CameraNode{ "Main Camera", root };
}

///////////////////////////// Callback Functions for Window Events ////////////
// Handle key input
void ApplicationSolar::keyCallback(int key, int action, int mods)
{
  if (key == GLFW_KEY_W  && (action == GLFW_PRESS || action == GLFW_REPEAT))
  {
    m_view_transform = glm::translate(m_view_transform, glm::fvec3{0.0f, 0.0f, -0.25f});
    uploadView();
  }
  if (key == GLFW_KEY_A  && (action == GLFW_PRESS || action == GLFW_REPEAT))
  {
    m_view_transform = glm::translate(m_view_transform, glm::fvec3{-0.25f, 0.0f, 0.0f});
    uploadView();
  }
  if (key == GLFW_KEY_S  && (action == GLFW_PRESS || action == GLFW_REPEAT))
  {
    m_view_transform = glm::translate(m_view_transform, glm::fvec3{0.0f, 0.0f, 0.25f});
    uploadView();
  }
  if (key == GLFW_KEY_D  && (action == GLFW_PRESS || action == GLFW_REPEAT))
  {
    m_view_transform = glm::translate(m_view_transform, glm::fvec3{0.25f, 0.0f, 0.0f});
    uploadView();
  }
  if (key == GLFW_KEY_Q  && (action == GLFW_PRESS || action == GLFW_REPEAT))
  {
    m_view_transform = glm::translate(m_view_transform, glm::fvec3{0.0f, -0.25f, 0.0f});
    uploadView();
  }
  if (key == GLFW_KEY_E  && (action == GLFW_PRESS || action == GLFW_REPEAT))
  {
    m_view_transform = glm::translate(m_view_transform, glm::fvec3{0.0f, 0.25f, 0.0f});
    uploadView();
  }
}

// Handle delta mouse movement input
void ApplicationSolar::mouseCallback(double pos_x, double pos_y) {
  // Mouse handling
  glm::fmat4 rotation_x = glm::rotate(glm::mat4{}, float(pos_x) * -0.0015f, glm::fvec3{ 0.0f, 1.0f, 0.0f });
  glm::fmat4 rotation_y = glm::rotate(glm::mat4{}, float(pos_y) * -0.0015f, glm::fvec3{ 1.0f, 0.0f, 0.0f });
  m_view_transform *= rotation_x * rotation_y;
  uploadView();
}

// Handle resizing
void ApplicationSolar::resizeCallback(unsigned width, unsigned height) {
  // Recalculate projection matrix for new aspect ration
  m_view_projection = utils::calculate_projection_matrix(float(width) / float(height));
  // Upload new projection matrix
  uploadProjection();
}


// .exe entry point
int main(int argc, char* argv[])
{
  std::cout << "Use WASD to move around. Use QE to go up and down. Use the mouse to move the camera.\n";

  // Start the render applicaton
  Application::run<ApplicationSolar>(argc, argv, 3, 2);
}