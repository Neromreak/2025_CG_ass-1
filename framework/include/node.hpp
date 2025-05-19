#ifndef NODE
#define NODE

#include <list>
#include <string>

#include <glm/gtc/matrix_transform.hpp>


class Node
{
public:
  // Constructors
  Node() = default;
  Node(std::string const& name, Node* parent);
  Node(std::string const& name, Node* parent, glm::fmat4 const& local_transform, glm::fmat4 const& world_transform);
  Node(std::string const& name, std::string const& path, Node* parent, std::list<Node*> const& children,
    int depth, glm::fmat4 const& local_transform, glm::fmat4 const& world_transform);
  
  ~Node();
  
  // Getter Setter
  std::string const& get_name() const;
  void set_name(std::string const&);
  std::string const& get_path() const;
  int get_depth() const;
  Node& get_parent() const;
  void set_parent(Node*);
  std::list<Node*> const& get_children() const;
  Node* get_children(std::string const&) const;
  glm::fmat4 const& get_local_transform() const;
  void set_local_transform(glm::fmat4 const&);
  glm::fmat4 const& get_world_transform() const;
  void set_world_transform(glm::fmat4 const&);
  void add_children(Node*);
  Node* remove_children(std::string const&);

private:
  std::string name_{"Default Node"};
  std::string path_;
  Node* parent_;
  std::list<Node*> children_;
  int depth_;
  glm::fmat4 local_transform_;
  glm::fmat4 world_transform_;
};

#endif