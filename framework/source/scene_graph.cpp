#include "scene_graph.hpp"


// Make singleton by having one instance
SceneGraph* SceneGraph::get_instance()
{
  if (!instance_)
  {
    instance_ = new SceneGraph();
  }
  return instance_;
}

// Getter Setter
std::string const& SceneGraph::get_name() const
{
  return name_;
}
void SceneGraph::set_name(std::string const& name_in)
{
  name_ = name_in;
}

Node* SceneGraph::get_root() const
{
  return root_;
}
void SceneGraph::set_root(Node* root_in)
{
  root_ = root_in;
}

SceneGraph* SceneGraph::instance_ = nullptr;