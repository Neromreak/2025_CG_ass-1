#include "node.hpp"
#include <glm/gtc/matrix_transform.hpp>


class PointLightNode : public Node
{
public:
  // Constructors
  PointLightNode() = default;
  PointLightNode(std::string const& name, Node* parent);
  PointLightNode(std::string const& name, Node* parent, glm::vec3 const& color, float intensity);
  PointLightNode(std::string const& name, Node* parent, glm::fmat4 const& local_transform, glm::fmat4 const& world_transform,
    float animation, glm::vec3 const& color, float intensity);
  PointLightNode(std::string const& name, Node* parent, std::list<Node*> const& children,
    glm::fmat4 const& local_transform, glm::fmat4 const& world_transform, float animation, glm::vec3 const& color, float intensity);

  // Getter Setter
  glm::vec3 const& get_color() const;
  void set_color(glm::vec3 const& color_in);
  float get_intensity() const;
  void set_intensity(float intensity_in);

  // Methods
  void render(std::map<std::string, shader_program> const* shaders, glm::fmat4 const* view_transform, glm::fmat4 transform) const override;


private:
  glm::vec3 color_;
  float intensity_;
};