#ifndef GEOMETRY_NODE
#define GEOMETRY_NODE

#include "node.hpp"


class GeometryNode : public Node
{
public:
  // Constructors
  GeometryNode() = default;
  GeometryNode(std::string const& name, Node* parent);
  GeometryNode(std::string const& name, Node* parent, model_object const* geometry, glm::vec3 const& color);
  GeometryNode(std::string const& name, Node* parent, glm::fmat4 const& local_transform, glm::fmat4 const& world_transform,
    model_object const* geometry, glm::vec3 const& color);
  GeometryNode(std::string const& name, Node* parent, std::list<Node*> const& children,
    glm::fmat4 const& local_transform, glm::fmat4 const& world_transform, float animation, model_object const* geometry,
    glm::vec3 const& color);

  // Getter Setter
  model_object const* get_model() const;
  void set_model(model_object const* geometry_in);

  // Methods
  void render(std::map<std::string, shader_program> const* shaders, glm::fmat4 const* view_transform, glm::fmat4 transform) const override;


private:
  model_object const* geometry_;
  glm::vec3 color_;
};

#endif