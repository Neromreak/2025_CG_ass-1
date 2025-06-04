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


// Helper function for printing a glm::fmat4
void print_glm4matf(glm::fmat4 const& m)
{
  std::cout << "---------------------\n";
  std::cout << m[0][0] << " " << m[1][0] << " " << m[2][0] << " " << m[3][0] << "\n";
  std::cout << m[0][1] << " " << m[1][1] << " " << m[2][1] << " " << m[3][1] << "\n";
  std::cout << m[0][2] << " " << m[1][2] << " " << m[2][2] << " " << m[3][2] << "\n";
  std::cout << m[0][3] << " " << m[1][3] << " " << m[2][3] << " " << m[3][3] << "\n";
}