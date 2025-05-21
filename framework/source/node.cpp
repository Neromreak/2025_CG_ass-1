#include "node.hpp"

// Constructors
Node::Node(std::string const& name, std::string const& path, Node* parent, std::list<Node*> const& children,
  int depth, glm::fmat4 const& local_transform, glm::fmat4 const& world_transform) :
  name_{ name },
  path_{ path },
  parent_{ parent },
  children_{ children },
  depth_{ depth },
  local_transform_{ local_transform },
  world_transform_{ world_transform }
{
  if (parent != nullptr)
  {
    set_parent(parent);
  }
  for (Node* new_node : children)
  {
    add_children(new_node);
  }
}
Node::Node(std::string const& name, Node* parent, glm::fmat4 const& local_transform, glm::fmat4 const& world_transform):
  Node::Node(name, "no_path", parent, {}, -1, local_transform, world_transform)
{ }
Node::Node(std::string const& name, Node* parent) :
  Node::Node(name, "no_path", parent, {}, -1, glm::fmat4{}, glm::fmat4{})
{
}

Node::~Node()
{
  for (Node* node : children_)
  {
    delete node;
  }
}


// Getter Setter
std::string const& Node::get_name() const
{
  return name_;
}
void Node::set_name(std::string const& name_in)
{
  name_ = name_in;
}
std::string const& Node::get_path() const
{
  return path_;
}
int Node::get_depth() const
{
  return depth_;
}
Node& Node::get_parent() const
{
  return *parent_;
}
void Node::set_parent(Node* parent_in)
{
  if (parent_in != nullptr)
  {
    parent_ = parent_in;
    parent_->children_.push_back(this);
  }
}
std::list<Node*> const& Node::get_children() const
{
  return children_;
}
Node* Node::get_children(std::string const& child_name) const
{
  for (Node* node : children_)
  {
    if (node->get_name() == child_name)
    {
      return node;
    }
  }
  return nullptr;
}
glm::fmat4 const& Node::get_local_transform() const
{
  return local_transform_;
}
void Node::set_local_transform(glm::fmat4 const& mat_in)
{
  local_transform_ = mat_in;
}
glm::fmat4 const& Node::get_world_transform() const
{
  return world_transform_;
}
void Node::set_world_transform(glm::fmat4 const& mat_in)
{
  world_transform_ = mat_in;
}
void Node::add_children(Node* child)
{
  children_.push_back(child);
  child->parent_ = this;
}
Node* Node::remove_children(std::string const& child_name)
{
  for (Node* node : children_)
  {
    if (node->get_name() == child_name)
    {
      children_.remove(node);
      node->parent_ = nullptr;
      return node;
    }
  }
  return nullptr;
}


// Methods
void Node::render(std::map<std::string, shader_program> const* shaders, glm::fmat4 const* view_transform) const
{
  // Method called to traverse tree and render all nodes

  // Propagate rendering down to children if Node has children
  for (Node* children : children_)
  {
    children->render(shaders, view_transform);
  }
}