#include "geometry_node.hpp"


// Constructors
GeometryNode::GeometryNode(std::string const& name, Node* parent):
  GeometryNode::GeometryNode(name, "no_path", parent, {}, -1, glm::fmat4{}, glm::fmat4{}, nullptr)
{ }
GeometryNode::GeometryNode(std::string const& name, Node* parent, model_object const* geometry):
  GeometryNode::GeometryNode(name, "no_path", parent, {}, -1, glm::fmat4{}, glm::fmat4{}, geometry)
{ }
GeometryNode::GeometryNode(std::string const& name, Node* parent, glm::fmat4 const& local_transform, glm::fmat4 const& world_transform, model_object const* geometry) :
  GeometryNode::GeometryNode(name, "no_path", parent, {}, -1, local_transform, world_transform, geometry)
{ }
GeometryNode::GeometryNode(std::string const& name, std::string const& path, Node* parent, std::list<Node*> const& children,
  int depth, glm::fmat4 const& local_transform, glm::fmat4 const& world_transform, model_object const* geometry) :
  Node::Node(name, path, parent, children, depth, local_transform, world_transform),
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
void GeometryNode::render(std::map<std::string, shader_program> const* shaders, glm::fmat4 const* view_transform) const
{
  // Method called to traverse tree and render all nodes
  std::cout << get_name() << "---------------------\n";
  std::cout << get_local_transform()[0][0] << " " << get_local_transform()[1][0] << " " << get_local_transform()[2][0] << " " << get_local_transform()[3][0] << "\n";
  std::cout << get_local_transform()[0][1] << " " << get_local_transform()[1][1] << " " << get_local_transform()[2][1] << " " << get_local_transform()[3][1] << "\n";
  std::cout << get_local_transform()[0][2] << " " << get_local_transform()[1][2] << " " << get_local_transform()[2][2] << " " << get_local_transform()[3][2] << "\n";
  std::cout << get_local_transform()[0][3] << " " << get_local_transform()[1][3] << " " << get_local_transform()[2][3] << " " << get_local_transform()[3][3] << "\n";
  // Propagate rendering down to children if Node has children
  for (Node* children : get_children())
  {
    children->render(shaders, view_transform);
  }

  // Actual rendering:
  // Bind shader to upload uniforms
  glUseProgram(shaders->at("planet").handle);

  // Add translation matrix with rotation and then translation
  glm::fmat4 model_matrix = glm::rotate(glm::fmat4{}, float(glfwGetTime()), glm::fvec3{0.0f, 1.0f, 0.0f});
  model_matrix = glm::translate(model_matrix, glm::fvec3{ 0.0f, 0.0f, get_local_transform()[3][2]});

  glUniformMatrix4fv(shaders->at("planet").u_locs.at("ModelMatrix"),
    1, GL_FALSE, glm::value_ptr(model_matrix));

  // Extra matrix for normal transformation to keep them orthogonal to surface
  glm::fmat4 normal_matrix = glm::inverseTranspose(glm::inverse(*view_transform) * model_matrix);
  glUniformMatrix4fv(shaders->at("planet").u_locs.at("NormalMatrix"),
    1, GL_FALSE, glm::value_ptr(normal_matrix));

  // Bind the VAO to draw
  glBindVertexArray(geometry_->vertex_AO);

  // Draw bound vertex array using bound shader
  glDrawElements(geometry_->draw_mode, geometry_->num_elements, model::INDEX.type, NULL);
}