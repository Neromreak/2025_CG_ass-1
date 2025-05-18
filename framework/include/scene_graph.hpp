#ifndef SCENE_GRAPH
#define SCENE_GRAPH

#include <string>
#include "node.hpp"


class SceneGraph
{
public:
  // Make singleton by having one instance
  static SceneGraph& get_instance()
  {
    if (!instance_)
    {
      instance_ = new SceneGraph();
    }
    return *instance_;
  }

  // Getter Setter
  std::string const& get_name() const
  {
    return name_;
  }
  void set_name(std::string const& name_in )
  {
    name_ = name_in;
  }

  Node& get_root() const
  {
    return *root_;
  }
  void set_root(Node* root_in)
  {
    root_ = root_in;
  }

private:
  static SceneGraph* instance_;
  std::string name_{"Scene Graph"};
  Node* root_;

  // Constructors (delete synthesized constructors and make default constructor private
  // ...because class is supposed to be singleton)
  SceneGraph() = default;
  SceneGraph(SceneGraph const&) = delete;
  SceneGraph& operator=(SceneGraph const&) = delete;

  ~SceneGraph() = default;
};

SceneGraph* SceneGraph::instance_ = nullptr;

#endif