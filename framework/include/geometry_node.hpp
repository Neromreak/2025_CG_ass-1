#include "node.hpp"
#include "structs.hpp"


class GeometryNode : public Node
{
public:
  // Getter Setter
  model_object const& get_model() const
  {
    return geometry_;
  }
  void set_model(model_object const& geometry_in)
  {
    geometry_ = geometry_in;
  }

private:
  model_object geometry_;
};