#include "geometry_node.hpp"


// Constructors
GeometryNode::GeometryNode(std::string const& name, Node* parent):
  GeometryNode::GeometryNode(name, parent, {}, glm::fmat4{}, glm::fmat4{}, 0.0f, nullptr, { 1.0f, 1.0f, 1.0f }, nullptr, nullptr, nullptr)
{ }
GeometryNode::GeometryNode(std::string const& name, Node* parent, model_object const* geometry, glm::vec3 const& color, texture_object const* texture):
  GeometryNode::GeometryNode(name, parent, {}, glm::fmat4{}, glm::fmat4{}, 0.0f, geometry, color, texture, nullptr, nullptr)
{ }
GeometryNode::GeometryNode(std::string const& name, Node* parent, glm::fmat4 const& local_transform, glm::fmat4 const& world_transform,
  model_object const* geometry, glm::vec3 const& color, texture_object const* texture) :
  GeometryNode::GeometryNode(name, parent, {}, local_transform, world_transform, 0.0f, geometry, color, texture, nullptr, nullptr)
{ }
GeometryNode::GeometryNode(std::string const& name, Node* parent, std::list<Node*> const& children,
  glm::fmat4 const& local_transform, glm::fmat4 const& world_transform, float animation, model_object const* geometry,
  glm::vec3 const& color, texture_object const* texture) :
  GeometryNode::GeometryNode(name, parent, children, local_transform, world_transform, animation, geometry, color, texture, nullptr, nullptr)
{ }
GeometryNode::GeometryNode(std::string const& name, Node* parent, std::list<Node*> const& children,
  glm::fmat4 const& local_transform, glm::fmat4 const& world_transform, float animation, model_object const* geometry,
  glm::vec3 const& color, texture_object const* texture, texture_object const* texture_spec, texture_object const* texture_normal) :
  Node::Node(name, parent, children, local_transform, world_transform, animation, nullptr),
  geometry_{ geometry },
  color_{ color },
  texture_{ texture },
  texture_spec_{ texture_spec },
  texture_normal_{ texture_normal }
{ }

// Getter Setter
model_object const* GeometryNode::get_model() const
{
  return geometry_;
}
void GeometryNode::set_model(model_object const* geometry_in)
{
  geometry_ = geometry_in;
}

// Methods
void GeometryNode::render(std::map<std::string, shader_program> const* shaders, glm::fmat4 const* view_transform, glm::fmat4 transform) const
{
  // Transformations:
  // Create translation matrix with rotation
  glm::fmat4 rotation_matrix = glm::rotate(glm::fmat4{}, float(glfwGetTime() * get_animation()), glm::fvec3{ 0.0f, 1.0f, 0.0f });
  // Add local transform
  glm::fmat4 new_local_transform = rotation_matrix * get_local_transform();
  // Inherit local transform of parent and add own local transform to it
  glm::fmat4 new_transform = transform * new_local_transform;


  // Actual rendering:
  if (get_name() == "Sun")
  {
    // Bind shader to upload uniforms
    glUseProgram(shaders->at("sun").handle);
    // Model Matrix
    glUniformMatrix4fv(shaders->at("sun").u_locs.at("ModelMatrix"), 1, GL_FALSE, glm::value_ptr(new_transform));
    // Normal Matrix
    // Extra matrix for normal transformation to keep them orthogonal to surface
    glm::fmat4 normal_matrix = glm::inverseTranspose(new_transform);
    glUniformMatrix4fv(shaders->at("sun").u_locs.at("NormalMatrix"), 1, GL_FALSE, glm::value_ptr(normal_matrix));
    // Camera Position
    glm::vec3 cam_pos{ (*view_transform)[3][0] / (*view_transform)[3][3], (*view_transform)[3][1] / (*view_transform)[3][3] , (*view_transform)[3][2] / (*view_transform)[3][3] };
    glUniform3f(shaders->at("sun").u_locs.at("CamPos"), cam_pos[0], cam_pos[1], cam_pos[2]);
    // Object scale (on one axis suffices as the planets are evenly scaled)
    float scale_x = glm::length(glm::vec3(get_local_transform()[0]));
    glUniform1f(shaders->at("sun").u_locs.at("Scale"), scale_x);

    // Texture 'color'
    // Select texture unit
    glActiveTexture(GL_TEXTURE0);
    // Bind texture object
    glBindTexture(texture_->target, texture_->handle);
    glUniform1i(shaders->at("sun").u_locs.at("TextureColor"), 0);
  }
  else
  {
    // Bind shader to upload uniforms
    glUseProgram(shaders->at("planet").handle);
    // Model Matrix
    glUniformMatrix4fv(shaders->at("planet").u_locs.at("ModelMatrix"), 1, GL_FALSE, glm::value_ptr(new_transform));
    // Normal Matrix
    // Extra matrix for normal transformation to keep them orthogonal to surface
    glm::fmat4 normal_matrix = glm::inverseTranspose(new_transform);
    glUniformMatrix4fv(shaders->at("planet").u_locs.at("NormalMatrix"), 1, GL_FALSE, glm::value_ptr(normal_matrix));
    // Object Color
    glUniform3fv(shaders->at("planet").u_locs.at("ObjColor"), 1, glm::value_ptr(color_));
    // Camera Position
    glm::vec3 cam_pos{ (*view_transform)[3][0] / (*view_transform)[3][3], (*view_transform)[3][1] / (*view_transform)[3][3] , (*view_transform)[3][2] / (*view_transform)[3][3] };
    glUniform3f(shaders->at("planet").u_locs.at("CamPos"), cam_pos[0], cam_pos[1], cam_pos[2]);
    // Object scale (on one axis suffices as the planets are evenly scaled)
    float scale_x = glm::length(glm::vec3(get_local_transform()[0]));
    glUniform1f(shaders->at("planet").u_locs.at("Scale"), scale_x);

    // Texture 'color'
    // Select texture unit
    glActiveTexture(GL_TEXTURE0);
    // Bind texture object
    glBindTexture(texture_->target, texture_->handle);
    glUniform1i(shaders->at("planet").u_locs.at("TextureColor"), 0 );

    // Texture specular
    if (texture_spec_ != nullptr)
    {
      // Select texture unit
      glActiveTexture(GL_TEXTURE1);
      // Bind texture object
      glBindTexture(texture_spec_->target, texture_spec_->handle);
      glUniform1i(shaders->at("planet").u_locs.at("TextureSpecular"), 1);
    }
    glUniform1b(shaders->at("planet").u_locs.at("TextureSpecularIsSet"), texture_spec_ != nullptr);
    
    // Texture normal
    if (texture_normal_ != nullptr)
    {
      // Select texture unit
      glActiveTexture(GL_TEXTURE2);
      // Bind texture object
      glBindTexture(texture_normal_->target, texture_normal_->handle);
      glUniform1i(shaders->at("planet").u_locs.at("TextureNormal"), 2);
    }
    glUniform1b(shaders->at("planet").u_locs.at("TextureNormalIsSet"), texture_normal_ != nullptr);
  }

  // Bind the VAO to draw
  glBindVertexArray(geometry_->vertex_AO);
  // Draw bound vertex array using bound shader
  glDrawElements(geometry_->draw_mode, geometry_->num_elements, model::INDEX.type, NULL);

  // Unbind VA
  glBindVertexArray(0);


  // Render children
  Node::render(shaders, view_transform, transform);
}