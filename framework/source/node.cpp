#include "node.hpp"

// Constructors
Node::Node(std::string const& name, Node* parent, std::list<Node*> const& children, glm::fmat4 const& local_transform,
  glm::fmat4 const& world_transform, float animation, model_object const* geometry_orbit) :
  name_{ name },
  parent_{ parent },
  children_{ children },
  local_transform_{ local_transform },
  world_transform_{ world_transform },
  animation_{ animation },
  geometry_orbit_{ geometry_orbit }
{
  if (parent != nullptr)
  {
    set_parent(parent);
  }
  for (Node* new_node : children)
  {
    add_children(new_node);
  }
  path_ = get_name();
  depth_ = 0;
  Node* parent_node = parent_;
  while (parent_node != nullptr)
  {
    path_ += " > " + parent_node->get_name();
    parent_node = &parent_node->get_parent();
    depth_++;
  }
}
Node::Node(std::string const& name, Node* parent, glm::fmat4 const& local_transform, glm::fmat4 const& world_transform,
  float animation, model_object const* geometry_orbit):
  Node::Node(name, parent, {}, local_transform, world_transform, animation, geometry_orbit)
{ }
Node::Node(std::string const& name, Node* parent) :
  Node::Node(name, parent, {}, glm::fmat4{}, glm::fmat4{}, 0.0f, nullptr)
{ }

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
float Node::get_animation() const
{
  return animation_;
}


// Methods
void Node::render(std::map<std::string, shader_program> const* shaders, glm::fmat4 const* view_transform, glm::fmat4 transform) const
{
  // Method called to traverse tree and render all nodes


  // Transformations:
  // Create translation matrix with rotation
  glm::fmat4 rotation_matrix = glm::rotate(glm::fmat4{}, float(glfwGetTime() * animation_), glm::fvec3{ 0.0f, 1.0f, 0.0f });
  // Add local transform
  glm::fmat4 new_local_transform = rotation_matrix * get_local_transform();
  // Inherit local transform of parent and add own local transform to it
  glm::fmat4 new_transform = transform * new_local_transform;
  

  // Render the orbit (Ass2):
  if (geometry_orbit_ != nullptr)
  {
    // Calculate size of orbit
    glm::fmat4 temp = get_local_transform();
    float orbit_radius = sqrt(pow(temp[3][0] / temp[3][3], 2) + pow(temp[3][1] / temp[3][3], 2) + pow(temp[3][2] / temp[3][3], 2));

    // Scale orbit accordingly
    glm::fmat4 orbit_transform = glm::scale(transform, orbit_radius * glm::vec3{1.0f, 1.0f, 1.0f});

    // Bind shader
    glUseProgram(shaders->at("vao").handle);

    // Bind the VAO to draw
    glBindVertexArray(geometry_orbit_->vertex_AO);

    // Upload transformation to ModelMatrix uniform of the shader
    glUniformMatrix4fv(shaders->at("vao").u_locs.at("ModelMatrix"), 1, GL_FALSE, glm::value_ptr(orbit_transform));

    // Draw bound vertex array using bound shader
    glDrawArrays(geometry_orbit_->draw_mode, 0, geometry_orbit_->num_elements);

    glBindVertexArray(0);
  }


  // Propagate rendering down to children if Node has children
  for (Node* children : children_)
  {
    children->render(shaders, view_transform, new_transform);
  }
}