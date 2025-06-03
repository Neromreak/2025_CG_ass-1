#include "point_light_node.hpp"


// Constructors
PointLightNode::PointLightNode(std::string const& name, Node* parent):
  PointLightNode::PointLightNode(name, parent, {}, glm::fmat4{}, glm::fmat4{}, 0.0f, glm::vec3{ 1, 0, 0 }, 1)
{ }
PointLightNode::PointLightNode(std::string const& name, Node* parent, glm::vec3 const& color, float intensity):
  PointLightNode::PointLightNode(name, parent, {}, glm::fmat4{}, glm::fmat4{}, 0.0f, color, intensity)
{ }
PointLightNode::PointLightNode(std::string const& name, Node* parent, glm::fmat4 const& local_transform, glm::fmat4 const& world_transform,
  float animation, glm::vec3 const& color, float intensity):
  PointLightNode::PointLightNode(name, parent, {}, local_transform, world_transform, animation, color, intensity)
{ }
PointLightNode::PointLightNode(std::string const& name, Node* parent, std::list<Node*> const& children,
  glm::fmat4 const& local_transform, glm::fmat4 const& world_transform, float animation, glm::vec3 const& color, float intensity):
  Node::Node(name, parent, children, local_transform, world_transform, animation, nullptr),
  color_{ color },
  intensity_{ intensity }
{ }

// Getter Setter
glm::vec3 const& PointLightNode::get_color() const
{
  return color_;
}
void PointLightNode::set_color(glm::vec3 const& color_in)
{
  color_ = color_in;
}
float PointLightNode::get_intensity() const
{
  return intensity_;
}
void PointLightNode::set_intensity(float intensity_in)
{
  intensity_ = intensity_in;
}

// Methods
void PointLightNode::render(std::map<std::string, shader_program> const* shaders, glm::fmat4 const* view_transform, glm::fmat4 transform) const
{
  // Method called to traverse tree and render all nodes
  Node::render(shaders, view_transform, transform);
}