#include "geometry_node.hpp"


// Constructors
GeometryNode::GeometryNode(std::string const& name, Node* parent):
  GeometryNode::GeometryNode(name, "no_path", parent, {}, -1, glm::fmat4{}, glm::fmat4{}, 0.0f, nullptr)
{ }
GeometryNode::GeometryNode(std::string const& name, Node* parent, model_object const* geometry):
  GeometryNode::GeometryNode(name, "no_path", parent, {}, -1, glm::fmat4{}, glm::fmat4{}, 0.0f, geometry)
{ }
GeometryNode::GeometryNode(std::string const& name, Node* parent, glm::fmat4 const& local_transform, glm::fmat4 const& world_transform, model_object const* geometry) :
  GeometryNode::GeometryNode(name, "no_path", parent, {}, -1, local_transform, world_transform, 0.0f, geometry)
{ }
GeometryNode::GeometryNode(std::string const& name, std::string const& path, Node* parent, std::list<Node*> const& children,
  int depth, glm::fmat4 const& local_transform, glm::fmat4 const& world_transform, float animation, model_object const* geometry) :
  Node::Node(name, path, parent, children, depth, local_transform, world_transform, animation),
  geometry_{ geometry }
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
  // Method called to traverse tree and render all nodes
  /*
  // DEBUG
  std::cout << get_name() << "---------------------\n";
  std::cout << transform[0][0] << " " << transform[1][0] << " " << transform[2][0] << " " << transform[3][0] << "\n";
  std::cout << transform[0][1] << " " << transform[1][1] << " " << transform[2][1] << " " << transform[3][1] << "\n";
  std::cout << transform[0][2] << " " << transform[1][2] << " " << transform[2][2] << " " << transform[3][2] << "\n";
  std::cout << transform[0][3] << " " << transform[1][3] << " " << transform[2][3] << " " << transform[3][3] << "\n";
  */
 
  // Inherit local transformation of parent
  glm::fmat4 new_transform = transform * get_local_transform();

  // Actual rendering:
  // Bind shader to upload uniforms
  glUseProgram(shaders->at("planet").handle);
  glUniformMatrix4fv(shaders->at("planet").u_locs.at("ModelMatrix"), 1, GL_FALSE, glm::value_ptr(new_transform));
  // Extra matrix for normal transformation to keep them orthogonal to surface
  glm::fmat4 normal_matrix = glm::inverseTranspose(glm::inverse(*view_transform) * new_transform);
  glUniformMatrix4fv(shaders->at("planet").u_locs.at("NormalMatrix"), 1, GL_FALSE, glm::value_ptr(normal_matrix));

  glm::vec4 frag_pos_v4 = new_transform * glm::vec4{ 0.0f, 0.0f, 0.0f, 1.0f };
  glm::vec3 frag_pos{ frag_pos_v4[0] / frag_pos_v4[3], frag_pos_v4[1] / frag_pos_v4[3], frag_pos_v4[2] / frag_pos_v4[3] };
  // shader_program shader_p = shaders->at("planet");
  glUniform3f(glGetUniformLocation(geometry_->vertex_AO, "frag_pos"), frag_pos[0], frag_pos[1], frag_pos[2]);
  if (get_name() == "Sun")
  {
    glUniform1i(glGetUniformLocation(geometry_->vertex_AO, "is_sun"), 1);
  }
  else
  {
    glUniform1i(glGetUniformLocation(geometry_->vertex_AO, "is_sun"), 0);
  }

  // Bind the VAO to draw
  glBindVertexArray(geometry_->vertex_AO);
  // Draw bound vertex array using bound shader
  glDrawElements(geometry_->draw_mode, geometry_->num_elements, model::INDEX.type, NULL);
  
  // Render children
  Node::render(shaders, view_transform, transform);
}