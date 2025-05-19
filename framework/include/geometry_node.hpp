#ifndef GEOMETRY_NODE
#define GEOMETRY_NODE

#include "node.hpp"
#include "structs.hpp"


class GeometryNode : public Node
{
public:
  // Constructors
  GeometryNode() = default;
  GeometryNode(std::string const& name, Node* parent):
    GeometryNode::GeometryNode(name, "no_path", parent, {}, -1, glm::fmat4{}, glm::fmat4{}, nullptr)
  { }
  GeometryNode(std::string const& name, Node* parent, model_object const* geometry):
    GeometryNode::GeometryNode(name, "no_path", parent, {}, -1, glm::fmat4{}, glm::fmat4{}, geometry)
  { }
  GeometryNode(std::string const& name, Node* parent, glm::fmat4 const& local_transform, glm::fmat4 const& world_transform, model_object const* geometry):
    GeometryNode::GeometryNode(name, "no_path", parent, {}, -1, local_transform, world_transform, geometry)
  { }
  GeometryNode(std::string const& name, std::string const& path, Node* parent, std::list<Node*> const& children,
    int depth, glm::fmat4 const& local_transform, glm::fmat4 const& world_transform, model_object const* geometry):
    Node::Node(name, path, parent, children, depth, local_transform, world_transform),
    geometry_{ geometry }
  { }

  // Getter Setter
  model_object const* get_model() const
  {
    return geometry_;
  }
  void set_model(model_object const* geometry_in)
  {
    geometry_ = geometry_in;
  }

private:
  model_object const* geometry_;
};

#endif