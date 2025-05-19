#include "node.hpp"
#include <glm/gtc/matrix_transform.hpp>


class PointLightNode : public Node
{
public:
  // Constructors
  PointLightNode() = default;
  PointLightNode(std::string const& name, Node* parent):
    PointLightNode::PointLightNode(name, "no_path", parent, {}, -1, glm::fmat4{}, glm::fmat4{}, glm::vec3{1, 0, 0}, 1)
  { }
  PointLightNode(std::string const& name, Node* parent, glm::vec3 const& color, float intensity) :
    PointLightNode::PointLightNode(name, "no_path", parent, {}, -1, glm::fmat4{}, glm::fmat4{}, color, intensity)
  {
  }
  PointLightNode(std::string const& name, Node* parent, glm::fmat4 const& local_transform, glm::fmat4 const& world_transform, glm::vec3 const& color, float intensity):
    PointLightNode::PointLightNode(name, "no_path", parent, {}, -1, local_transform, world_transform, color, intensity)
  { }
  PointLightNode(std::string const& name, std::string const& path, Node* parent, std::list<Node*> const& children,
    int depth, glm::fmat4 const& local_transform, glm::fmat4 const& world_transform, glm::vec3 const& color, float intensity):
    Node::Node(name, path, parent, children, depth, local_transform, world_transform),
    color_{ color },
    intensity_{ intensity }
  { }

  // Getter Setter
  glm::vec3 const& get_color() const
  {
    return color_;
  }
  void set_color(glm::vec3 const& color_in)
  {
    color_ = color_in;
  }
  float get_intensity() const
  {
    return intensity_;
  }
  void set_intensity(float intensity_in)
  {
    intensity_ = intensity_in;
  }

private:
  glm::vec3 color_;
  float intensity_;
};