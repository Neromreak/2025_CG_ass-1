#ifndef CAMERA_NODE
#define CAMERA_NODE

#include "node.hpp"

#include <glm/gtc/matrix_transform.hpp>


class CameraNode : public Node
{
public:
  // Constructors
  CameraNode() = default;
  CameraNode(std::string const& name, Node* parent);
  CameraNode(std::string const& name, Node* parent, glm::fmat4 const& projection_matrix);
  CameraNode(std::string const& name, Node* parent, glm::fmat4 const& local_transform, glm::fmat4 const& world_transform, glm::fmat4 const& projection_matrix);
  CameraNode(std::string const& name, Node* parent, std::list<Node*> const& children,
    glm::fmat4 const& local_transform, glm::fmat4 const& world_transform, float animation, glm::fmat4 const& projection_matrix);

  // Getter Setter
  bool is_perspective() const;
  bool is_enabled() const;
  void set_enabled(bool is_enabled_in);
  glm::fmat4 const& get_projection_matrix() const;
  void set_projection_matrix(glm::fmat4 projection_matrix_in);

  // Methods
  void render(std::map<std::string, shader_program> const* shaders, glm::fmat4 const* view_transform, glm::fmat4 transform) const override;


private:
  bool is_perspective_ = true;
  bool is_enabled_ = true;
  glm::fmat4 projection_matrix_;
};

#endif