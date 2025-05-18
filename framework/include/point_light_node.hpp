#include "node.hpp"
#include <glm/gtc/matrix_transform.hpp>


class PointLightNode : public Node
{
public:
  // Getter Setter
  glm::vec3 const& get_color() const
  {
    return color_;
  }
  void set_color(glm::vec3 const& color_in)
  {
    color_ = color_in;
  }
  float get_intensity() const
  {
    return intensity_;
  }
  void set_intensity(float intensity_in)
  {
    intensity_ = intensity_in;
  }

private:
  glm::vec3 color_;
  float intensity_;
};