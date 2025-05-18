#include "node.hpp"

// Constructors


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
void Node::set_parent(Node const& parent_in)
{
  *parent_ = parent_in;
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
void Node::add_children(Node& child)
{
  children_.push_back(&child);
}
Node* Node::remove_children(std::string const& child_name)
{
  for (Node* node : children_)
  {
    if (node->get_name() == child_name)
    {
      children_.remove(node);
      return node;
    }
  }
  return nullptr;
}