#ifndef CAMERA_NODE
#define CAMERA_NODE

#include "node.hpp"

#include <glm/gtc/matrix_transform.hpp>


class CameraNode : public Node
{
public:
  // Getter Setter
  bool is_perspective() const
  {
    return is_perspective_;
  }
  bool is_enabled() const
  {
    return is_enabled_;
  }
  void set_enabled(bool is_enabled_in)
  {
    is_enabled_ = is_enabled_in;
  }
  glm::fmat4 const& get_projection_matrix() const 
  {
    return projection_matrix_;
  }
  void set_projection_matrix(glm::fmat4 projection_matrix_in)
  {
    projection_matrix_ = projection_matrix_in;
  }

private:
  bool is_perspective_ = true;
  bool is_enabled_ = true;
  glm::fmat4 projection_matrix_;
};

#endif