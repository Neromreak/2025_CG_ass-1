#include <string>
#include "node.hpp"

class SceneGraph
{
public:
  // Make singleton by having one instance
  static SceneGraph& getInstance()
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
  void set_root(Node const& root_in)
  {
    *root_ = root_in;
  }

private:
  static SceneGraph* instance_;
  std::string name_{"Scene Graph"};
  Node* root_;

};