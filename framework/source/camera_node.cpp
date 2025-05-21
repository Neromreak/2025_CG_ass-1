#include "camera_node.hpp"


// Constructors
CameraNode::CameraNode(std::string const& name, Node* parent) :
  CameraNode::CameraNode(name, "no_path", parent, {}, -1, glm::fmat4{}, glm::fmat4{}, glm::fmat4{})
{
}
CameraNode::CameraNode(std::string const& name, Node* parent, glm::fmat4 const& projection_matrix) :
  CameraNode::CameraNode(name, "no_path", parent, {}, -1, glm::fmat4{}, glm::fmat4{}, projection_matrix)
{
}
CameraNode::CameraNode(std::string const& name, Node* parent, glm::fmat4 const& local_transform, glm::fmat4 const& world_transform, glm::fmat4 const& projection_matrix) :
  CameraNode::CameraNode(name, "no_path", parent, {}, -1, local_transform, world_transform, projection_matrix)
{
}
CameraNode::CameraNode(std::string const& name, std::string const& path, Node* parent, std::list<Node*> const& children,
  int depth, glm::fmat4 const& local_transform, glm::fmat4 const& world_transform, glm::fmat4 const& projection_matrix) :
  Node::Node(name, path, parent, children, depth, local_transform, world_transform),
  projection_matrix_{ projection_matrix }
{ }

// Getter Setter
bool CameraNode::is_perspective() const
{
  return is_perspective_;
}
bool CameraNode::is_enabled() const
{
  return is_enabled_;
}
void CameraNode::set_enabled(bool is_enabled_in)
{
  is_enabled_ = is_enabled_in;
}
glm::fmat4 const& CameraNode::get_projection_matrix() const
{
  return projection_matrix_;
}
void CameraNode::set_projection_matrix(glm::fmat4 projection_matrix_in)
{
  projection_matrix_ = projection_matrix_in;
}

// Methods
void CameraNode::render(std::map<std::string, shader_program> const* shaders, glm::fmat4 const* view_transform) const
{
  // Method called to traverse tree and render all nodes

  // Propagate rendering down to children if Node has children
  for (Node* children : get_children())
  {
    children->render(shaders, view_transform);
  }
}